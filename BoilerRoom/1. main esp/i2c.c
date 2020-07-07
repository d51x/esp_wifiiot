#define FW_VER "1.46"

#define POWER_REG_ADDR 0x10
#define WRITE_BIT   0
#define READ_BIT    1

#define POWER_REG_CMD_GET_GPIO_MODE 0x00
#define POWER_REG_CMD_SET_GPIO_MODE 0x01
#define POWER_REG_CMD_GET_DUTY 0x02
#define POWER_REG_CMD_SET_DUTY 0x03
#define POWER_REG_CMD_GET_ENERGY 0x04
#define POWER_REG_CMD_GET_UPTIME 0x05


#define POWER_REG_DATA_SIZE 16

#if mqtte || mqttjsone
MQTT_Client* mqtt_client;
#define MQTT_SEND_INTERVAL 10 // sec
#define MQTT_PAYLOAD_BUF 20
static char payload[MQTT_PAYLOAD_BUF];
#define MQTT_TOPIC_VOLTAGE	"pmv"
#define MQTT_TOPIC_CURRENT	"pmc"
#define MQTT_TOPIC_POWER	"pmw"
#define MQTT_TOPIC_ENERGY	"pmwh"
uint32_t mqtt_send_interval_sec = MQTT_SEND_INTERVAL;
static volatile os_timer_t mqtt_send_timer;	
void ICACHE_FLASH_ATTR mqtt_send_cb();
#endif

static volatile os_timer_t check_valdes_timer;	
void ICACHE_FLASH_ATTR check_valdes_cb();

typedef union  {
    float f;
    struct {
        uint8_t b[4];
    };
} PowerRegEnergyData;

typedef struct PowerRegData {
    uint8_t mode;
    uint8_t duty;
    uint32_t uptime;
    PowerRegEnergyData v;
    PowerRegEnergyData c;
    PowerRegEnergyData p;
    PowerRegEnergyData e;
} PowerRegData_t;

PowerRegData_t powerRegData;
uint8_t power_reg_busy = 0;





bool ICACHE_FLASH_ATTR i2c_write_cmd(uint8_t addr, uint8_t cmd) {
    i2c_start();
    i2c_writeByte( addr << 1 | WRITE_BIT );  // 0 - write bit
    if(i2c_getAck()){return false;} // slave not ack
    i2c_writeByte( cmd ); 
    if(i2c_getAck()){return false;} // slave not ack        
    i2c_stop();
    return true;
}

bool ICACHE_FLASH_ATTR i2c_write_cmd_data(uint8_t addr, uint8_t cmd, uint8_t data) {
    i2c_start();
    i2c_writeByte( addr << 1 | WRITE_BIT );  // 0 - write bit
    if(i2c_getAck()){return false;} // slave not ack
    i2c_writeByte( cmd ); 
    if(i2c_getAck()){return false;} // slave not ack        

            i2c_writeByte( data ); 
            if(i2c_getAck()){return false;}

    
    i2c_stop();
    return true;
}

uint8_t ICACHE_FLASH_ATTR _power_reg_read_gpio_mode() {
    if ( !i2c_write_cmd(POWER_REG_ADDR, POWER_REG_CMD_GET_GPIO_MODE) ) {return 255;    }
    delay(10);

    i2c_start();
    i2c_writeByte( POWER_REG_ADDR << 1 | READ_BIT );
    if(i2c_getAck()){ i2c_stop(); return 255;} // slave not ack

    uint8_t value = i2c_readByte(); 
    i2c_setAck(0);  // 0 -ACK, 1 - NACK (before stop)    
    uint8_t crc = i2c_readByte();
    i2c_setAck(1);  // 0 -ACK, 1 - NACK (before stop)    
    i2c_stop();
    if ( (POWER_REG_ADDR + POWER_REG_CMD_GET_GPIO_MODE + value)&0xFF != crc ) return 255;
    return value;
}

uint8_t ICACHE_FLASH_ATTR _power_reg_read_duty() {
    if ( !i2c_write_cmd(POWER_REG_ADDR, POWER_REG_CMD_GET_DUTY) ) return 255;
    delay(10);

    i2c_start();
    i2c_writeByte( POWER_REG_ADDR << 1 | READ_BIT );
    if(i2c_getAck()){ i2c_stop(); return 255;} // slave not ack

    uint8_t value = i2c_readByte(); 
    i2c_setAck(0);  // 0 -ACK, 1 - NACK (before stop)    
    uint8_t crc = i2c_readByte();
    i2c_setAck(1);  // 0 -ACK, 1 - NACK (before stop)    
    i2c_stop();
    if ( (POWER_REG_ADDR + POWER_REG_CMD_GET_DUTY + value)&0xFF != crc ) return 255;
    return value;
}

void ICACHE_FLASH_ATTR _power_reg_read_energy() {
    if ( !i2c_write_cmd(POWER_REG_ADDR, POWER_REG_CMD_GET_ENERGY) ) return;
    delay(10);

    uint8_t buf[16];
    memset(buf, 0, 16);
    uint8_t _crc = POWER_REG_ADDR + POWER_REG_CMD_GET_ENERGY;

    i2c_start();
    i2c_writeByte( POWER_REG_ADDR << 1 | READ_BIT );
    if(i2c_getAck()){ i2c_stop(); return;} // slave not ack

    uint8_t i = 0;
    for (i=0;i<POWER_REG_DATA_SIZE;i++){
        buf[i] =  i2c_readByte(); 
        _crc += buf[i];
        i2c_setAck( i == (POWER_REG_DATA_SIZE - 1));  // 0 -ACK, 1 - NACK (before stop)    
        //i2c_setAck( 0 );  // 0 -ACK, 1 - NACK (before stop)    
    }
    //uint8_t crc = i2c_readByte();
    //i2c_setAck( 1 );
    i2c_stop();

    //if ( _crc != crc ) memset(buf, 0, 16);
    memcpy(&powerRegData.v.b[0], &buf[0], 4);
    memcpy(&powerRegData.c.b[0], &buf[4], 4);
    memcpy(&powerRegData.p.b[0], &buf[8], 4);
    memcpy(&powerRegData.e.b[0], &buf[12], 4);

    
}


void ICACHE_FLASH_ATTR power_reg_get_mode() {
    if ( power_reg_busy ) return;
    power_reg_busy = 1;
    uint8_t val  = _power_reg_read_gpio_mode();
    if ( val != 255) powerRegData.mode = val;
    power_reg_busy = 0;
}

void ICACHE_FLASH_ATTR power_reg_get_duty() {
    if ( power_reg_busy ) return;
    power_reg_busy = 1;
    uint8_t val = _power_reg_read_duty();
    if ( val != 255 ) powerRegData.duty = val;
    power_reg_busy = 0;
}

void ICACHE_FLASH_ATTR power_reg_get_energy() {
    if ( power_reg_busy ) return;
    power_reg_busy = 1;
    _power_reg_read_energy();
    power_reg_busy = 0;

    if ( powerRegData.v.f < 0 || powerRegData.v.f > 400 ) powerRegData.v.f = 0.0;
    if ( powerRegData.c.f < 0 || powerRegData.c.f > 40 ) powerRegData.c.f = 0.0;
    if ( powerRegData.p.f < 0 || powerRegData.p.f > 15000 ) powerRegData.p.f = 0.0;
    if ( powerRegData.e.f < 0 ) powerRegData.e.f = 0.0;

    valdes[2] = powerRegData.v.f * 10;
    valdes[3] = powerRegData.c.f * 10;
    valdes[4] = (uint32_t)powerRegData.p.f;
    valdes[5] = (uint32_t)powerRegData.p.f;
}

void ICACHE_FLASH_ATTR power_reg_write_mode(uint8_t _mode) {
    i2c_write_cmd_data(POWER_REG_ADDR, POWER_REG_CMD_SET_GPIO_MODE, _mode);
}

bool ICACHE_FLASH_ATTR power_reg_set_mode(uint8_t _mode) {
    //os_timer_disarm(&power_reg_mode_timer);
    if ( power_reg_busy ) {
        return false;
        // start onetime timer
        //os_timer_setfn(&power_reg_mode_timer, (os_timer_func_t *)power_reg_set_mode, _mode);
        //os_timer_arm(&power_reg_mode_timer, 125, 0);
    } else {
        power_reg_busy = 1;
        power_reg_write_mode( _mode );
        power_reg_busy = 0;
        return true;
    }
}

void ICACHE_FLASH_ATTR power_reg_write_duty(uint8_t _duty) {
    i2c_write_cmd_data(POWER_REG_ADDR, POWER_REG_CMD_SET_DUTY, _duty);
}

bool ICACHE_FLASH_ATTR power_reg_set_duty(uint8_t _duty) {
    //os_timer_disarm(&power_reg_duty_timer);
    if ( power_reg_busy ) {
        return false;
        // start onetime timer
        //os_timer_setfn(&power_reg_duty_timer, (os_timer_func_t *)power_reg_set_duty, _duty);
        //os_timer_arm(&power_reg_duty_timer, 125, 0);
    } else {
        power_reg_busy = 1;
        power_reg_write_duty( _duty );
        power_reg_busy = 0;
        return true;
    }
}

uint32_t ICACHE_FLASH_ATTR power_reg_read_uptime() {
    if ( !i2c_write_cmd(POWER_REG_ADDR, POWER_REG_CMD_GET_UPTIME) ) {return 0;    }
    delay(10);

    uint32_t val = 0;
    i2c_start();
    i2c_writeByte( POWER_REG_ADDR << 1 | READ_BIT );
    if(i2c_getAck()){ i2c_stop(); return 0;} // slave not ack

    val = i2c_readByte(); 
    i2c_setAck(0);  // 0 -ACK, 1 - NACK (before stop)    

    val |= i2c_readByte() << 8; 
    i2c_setAck(0);  // 0 -ACK, 1 - NACK (before stop)    

    val |= i2c_readByte() << 16; 
    i2c_setAck(0);  // 0 -ACK, 1 - NACK (before stop)    

    val |= i2c_readByte() << 24; 
    i2c_setAck(1);  // 0 -ACK, 1 - NACK (before stop)    

    i2c_stop();
    return val;
}

void ICACHE_FLASH_ATTR power_reg_get_uptime() {
    if ( power_reg_busy ) return;
    power_reg_busy = 1;
    uint32_t uptime = power_reg_read_uptime();
    power_reg_busy = 0;
    
    if ( powerRegData.uptime == 0 || (powerRegData.uptime > 0 && uptime > 0) ) {
        powerRegData.uptime = uptime;
        valdes[6] = uptime;
    }
    
}


void ICACHE_FLASH_ATTR load_options() {
    #if mqtte || mqttjsone    
        mqtt_send_interval_sec = sensors_param.mqttts;
    #endif
    powerRegData.mode = sensors_param.cfgdes[0] > 1 ? 1 : sensors_param.cfgdes[0];
    valdes[0] = powerRegData.mode;

    powerRegData.duty = sensors_param.cfgdes[1] > 100 ? 100 : sensors_param.cfgdes[1];
    valdes[1] = powerRegData.duty;
}

#if mqtte || mqttjsone
void ICACHE_FLASH_ATTR mqtt_send_cb(){
    if ( sensors_param.mqtten != 1 ) return;
    char payload[MQTT_PAYLOAD_BUF];

    os_memset(payload, 0, MQTT_PAYLOAD_BUF);
    os_sprintf(payload,"%d.%d", (uint16_t)powerRegData.v.f, 		(uint16_t)(powerRegData.v.f * 10) % 10);
	system_soft_wdt_feed();
	MQTT_Publish(mqtt_client, MQTT_TOPIC_VOLTAGE, payload, os_strlen(payload), 2, 0, 1);

    os_memset(payload, 0, MQTT_PAYLOAD_BUF);
    os_sprintf(payload,"%d.%d", (uint16_t)powerRegData.c.f, 		(uint16_t)(powerRegData.c.f * 10) % 10);
	system_soft_wdt_feed();
	MQTT_Publish(mqtt_client, MQTT_TOPIC_CURRENT, payload, os_strlen(payload), 2, 0, 1);

    os_memset(payload, 0, MQTT_PAYLOAD_BUF);
    os_sprintf(payload,"%d", (uint16_t)powerRegData.p.f);
	system_soft_wdt_feed();
	MQTT_Publish(mqtt_client, MQTT_TOPIC_POWER, payload, os_strlen(payload), 2, 0, 1);

    os_memset(payload, 0, MQTT_PAYLOAD_BUF);
    os_sprintf(payload,"%d", (uint32_t)powerRegData.e.f);
	system_soft_wdt_feed();
	MQTT_Publish(mqtt_client, MQTT_TOPIC_ENERGY, payload, os_strlen(payload), 2, 0, 1);

}
#endif

void ICACHE_FLASH_ATTR check_valdes_cb(){
    // запись данных только при изменении
    static uint8_t not_sent_mode = 255;  // выставляется сюда значение только при изменении, потом его пишем по i2c, если оно не 255, если занято, то оставляем текущее знаение, если записали, то в 255 ставим обратно
    static uint8_t not_sent_duty = 255;

    uint8_t need_save = 0;
    uint8_t val = 0;
    uint8_t val2 = 0;

    //=============================================
    val = sensors_param.cfgdes[0] > 0 ? 1 : 0;
    if ( val != powerRegData.mode ) {
        powerRegData.mode = val;
        valdes[0] = val;
        not_sent_mode = val;
        #if mqtte || mqttjsone
            if ( sensors_param.mqtten && mqtt_client != NULL) {
                os_memset(payload, 0, 20);
                os_sprintf(payload,"%d", val);
                MQTT_Publish(mqtt_client, "valuedes0", payload, os_strlen(payload), 2, 0, 1);
                os_delay_us(20);
            }
        #endif
    }

    val = ( valdes[0] > 1 ) ? 1 : valdes[0];
    if ( val != powerRegData.mode ) {
        powerRegData.mode = val;
        not_sent_mode = val;
        sensors_param.cfgdes[0] = powerRegData.mode;
        need_save = 1;
    }


    //===================================================
    val = sensors_param.cfgdes[1] > 100 ? 100 : sensors_param.cfgdes[1];
    if ( val != powerRegData.duty) {
        powerRegData.duty = val;
        not_sent_duty = val;
        #if mqtte || mqttjsone
            if ( sensors_param.mqtten && mqtt_client != NULL) {
                os_memset(payload, 0, 20);
                os_sprintf(payload,"%d", val);
                MQTT_Publish(mqtt_client, "valuedes1", payload, os_strlen(payload), 2, 0, 1);
                os_delay_us(20);
            }
        #endif        
    }

    val = valdes[1] > 100 ? 100 : valdes[1];
    if ( val != powerRegData.duty ) {
        powerRegData.duty = val;
        not_sent_duty = val;
        sensors_param.cfgdes[1] = powerRegData.duty;
        need_save = 1;
    }

    if ( not_sent_mode < 255 ) {
        //if ( power_reg_set_mode( powerRegData.mode ) ) {
        if ( power_reg_set_mode( not_sent_mode ) ) {
            delay(10);
            not_sent_mode = 255;
        }
    }
        
    if ( not_sent_duty < 255 ) {
        //if ( power_reg_set_duty( powerRegData.duty ) ) {
        if ( power_reg_set_duty( not_sent_duty ) ) {
            delay(10);            
            not_sent_duty = 255;
        }
    }

    if ( need_save ) {
        SAVEOPT;
    }

    


}

void ICACHE_FLASH_ATTR startfunc(){
    load_options();
    power_reg_set_mode( powerRegData.mode );
    delay(10);
    power_reg_set_duty( powerRegData.duty ); 
    delay(10);

    #if mqtte || mqttjsone
        mqtt_client = (MQTT_Client*) &mqttClient;
	    os_timer_disarm(&mqtt_send_timer);
	    os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	    os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);        
    #endif
 
 	os_timer_disarm(&check_valdes_timer);
	os_timer_setfn(&check_valdes_timer, (os_timer_func_t *)check_valdes_cb, NULL);
	os_timer_arm(&check_valdes_timer, 50, 1);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {





    if (timersrc%5==0){
        
        //power_reg_get_mode();
        //delay(50);
        
        //power_reg_get_duty();
        //delay(50);
        
        power_reg_get_energy();
        delay(10);
        
        power_reg_get_uptime();
        delay(10);
    }

}

void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<hr>");
    os_sprintf(HTTPBUFF,"<br><b>Power Regulator:</b>");
    os_sprintf(HTTPBUFF,"<br><b>Mode: </b>%d (%s), &nbsp; <b>Duty: </b> %d"
                        , powerRegData.mode
                        , powerRegData.mode ? "duty" : "gpio"
                        , powerRegData.duty );
    
        os_sprintf(HTTPBUFF,"<br><b>valdes0: </b>%d, &nbsp; <b>valdes1: </b> %d"
                        , valdes[0]
                        , valdes[1] );

    os_sprintf(HTTPBUFF,"<br><b>Voltage: </b>%d.%d V, &nbsp; <b>Current: </b>%d.%d A"
                        , (uint32_t) powerRegData.v.f, (uint32_t)(powerRegData.v.f * 10) % 10 
                        , (uint32_t) powerRegData.c.f, (uint32_t)(powerRegData.c.f * 10) % 10);

    os_sprintf(HTTPBUFF,"<br><b>Power: </b>%d W, &nbsp; <b>Energy: </b>%d Wh"
                        , (uint32_t) powerRegData.p.f
                        , (uint32_t) powerRegData.e.f);

    os_sprintf(HTTPBUFF,"<br>Uptime: %d", powerRegData.uptime );
    os_sprintf(HTTPBUFF,"<br>Версия: %s", FW_VER );
}