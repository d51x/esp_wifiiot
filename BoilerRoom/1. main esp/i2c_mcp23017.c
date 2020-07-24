#define FW_VER "2.42"

/*
	valdes[0] - ssr mode
	valdes[1] - duty
	valdes[2] - voltage * 10
	valdes[3] - current * 10
	valdes[4] - power
	valdes[5] - energy
	valdes[6] - uptime
	
	valdes[7] - pump status
	
*/

#define CHECK_VALDES_INTERVAL 200

#define ESP_GPIO_BOILER_FULL_POWER	15
#define ESP_GPIO_BOILER_HALF_POWER	16

#define ESP_GPIO_PUMP				12
#define ESP_GPIO_VENT				13
#define ESP_GPIO_HOTCAB				14

#define GPIO_LCD_BACKLIGHT			199

#define BOILER_VIA_POWERREG			1

//**************************** MCP23017 ****************************************
/*
VirtualGPIO	MCP_GPIO	MCP_PORT_BIT
200			00			PORTA 0
201			01			PORTA 1
202			02			PORTA 2
203			03			PORTA 3
204			04			PORTA 4
205			05			PORTA 5
206			06			PORTA 6
207			07			PORTA 7
208			08			PORTB 0
209			09			PORTB 1
210			10			PORTB 2
211			11			PORTB 3
212			12			PORTB 4
213			13			PORTB 5
214			14			PORTB 6
215			15			PORTB 7
*/

#define MCP23017_GPIO0   1 << 0     //0x0001
#define MCP23017_GPIO1   1 << 1     //0x0002
#define MCP23017_GPIO2   1 << 2     //0x0004
#define MCP23017_GPIO3   1 << 3     //0x0008
#define MCP23017_GPIO4   1 << 4     //0x0010
#define MCP23017_GPIO5   1 << 5     //0x0020
#define MCP23017_GPIO6   1 << 6     //0x0040
#define MCP23017_GPIO7   1 << 7     //0x0080
#define MCP23017_GPIO8   1 << 8     //0x0100
#define MCP23017_GPIO9   1 << 9     //0x0200
#define MCP23017_GPIO10  1 << 10    //0x0400
#define MCP23017_GPIO11  1 << 11    //0x0800
#define MCP23017_GPIO12  1 << 12    //0x1000
#define MCP23017_GPIO13  1 << 13    //0x2000
#define MCP23017_GPIO14  1 << 14    //0x4000
#define MCP23017_GPIO15  1 << 15    //0x8000

#define MCP_GPIO_SET(val, gpio)   ((val) |= gpio )
#define MCP_GPIO_CLEAR(val, gpio)   ((val) &= ~gpio) 


#define B(bit_no)         (1 << (bit_no))
#define BIT_CLEAR(reg, bit_no)   (reg) &= ~B(bit_no)
#define BIT_SET(reg, bit_no)   (reg) |= B(bit_no)
#define BIT_CHECK(reg, bit_no)   ( (reg) & B(bit_no) )
#define BIT_TRIGGER(reg, bit_no)   (reg) ^= B(bit_no)

#define MCP23017_LED_RED 		MCP23017_GPIO3	//203	// 03, PORTA - bit 4
#define MCP23017_IOT_LED_RED 	203

#define MCP23017_LED_YELLOW 	MCP23017_GPIO7	//207	// 04, PORTA - bit 8 
#define MCP23017_IOT_LED_YELLOW 	207

#define MCP23017_LED_BLUE		MCP23017_GPIO4	//204	// 04, PORTA - bit 5
#define MCP23017_IOT_LED_BLUE 	204

#define MCP23017_LED_WHITE		MCP23017_GPIO6	//206	// 06, PORTA - bit 7
#define MCP23017_IOT_LED_WHITE 	206

#define MCP23017_LED_GREEN		MCP23017_GPIO5	//205	// 05, PORTA - bit 6
#define MCP23017_IOT_LED_GREEN 	205



#define MCP23017_BTN_RED		MCP23017_GPIO11	//211	// 11, PORTB - bit 4
#define MCP23017_IOT_BTN_RED 	211

#define MCP23017_BTN_BLUE		MCP23017_GPIO10	//210	// 10, PORTB - bit 3
#define MCP23017_IOT_BTN_BLUE 	210

#define MCP23017_BTN_WHITE1		MCP23017_GPIO9	//209	// 09, PORTB - bit 2
#define MCP23017_IOT_BTN_WHITE1 	209

#define MCP23017_BTN_WHITE2		MCP23017_GPIO8	//208 // 08, PORTB - bit 1
#define MCP23017_IOT_BTN_WHITE2 	208

//#define MCP23017_ENCODER_BTN	215	// 15, PORTB - bit 16
//#define MCP23017_ENCODER_LEFT	213 // 13, PORTB - bit 14 ???????
//#define MCP23017_ENCODER_RIGHT	214 // 14, PORTB - bit 15 ???????
			
#define MCP23017_INTB_PIN 4     // pin esp			
			
#define MCP23017_ENCODER_CLK_PIN    MCP23017_GPIO13
#define MCP23017_ENCODER_DT_PIN     MCP23017_GPIO14
#define MCP23017_ENCODER_BTN_PIN    MCP23017_GPIO15

#define MCP23017_ENCODER_BTN_INTR_TYPE GPIO_PIN_INTR_POSEDGE
#define MCP23017_BTN_INTR_TYPE GPIO_PIN_INTR_NEGEDGE

#define MCP23017_DEBOUNCE_TIME 200 // msec

#define IODIRA      0x00    // регистр, указыващий направления портов output/input
#define IODIRB      0x01
#define IPOLA       0x02    // инверсия ног
#define IPOLB       0x03
#define GPINTENA    0x04    // прерывания на ногах
#define GPINTENB    0x05
#define DEFVALA     0x06    // дефолтные значения ног, прерывание сработает, если на ноге сигнал отличается от дефолтного
#define DEFVALB     0x07
#define INTCONA     0x08    // условия сработки прерывания на ногах
#define INTCONB     0x09
#define IOCONA      0x0A    // конфигурационный регистр
#define IOCONB      0x0B
#define GPPUA       0x0C    // подтяжка ног 100к
#define GPPUB       0x0D
#define INTFA       0x0E    // регистр флагов прерываний, покажет на какой ноге было прерывание
#define INTFB       0x0F
#define INTCAPA     0x10    // покажет что было на ноге в момент прерывания на этой ноге
#define INTCAPB     0x11
#define GPIOA       0x12    // состояние ног, когда было прерывание на ноге может уже быть другое значение и надо читать INTCAP, если работаем с прерываниями
#define GPIOB       0x13
#define OLATA       0x14    
#define OLATB       0x15

//*********************** POWER REG ************************************			
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

//*************************** LCD2004 **********************************
#define LCD_UPDATE_TIME  5 // sec
#define PAGES_COUNT 2
#define page1_line1 " Boiler %3s Heat %3s"
//#define page1_line1 "Boiler %s %6d.%1d°"
#define page1_line2 "Pump  %4s Power %3s"
//#define page1_line2 "Pump   %s Power %s"
//#define page1_line3 "Hotcab %s %6d.%1d°"
#define page1_line3 "%7d.%1d° %6d.%1d°"
#define page1_line4_1 "Hum %2d.%1d%%  Air %2d.%1d°"
#define page1_line4_2 "%02d:%02d %02d.%02d.%02d %2d.%1d°"

#define str_page_header_2 "***  Power Load  ***"
#define str_page_voltage "Voltage %3d.%1dV"
#define str_current_power "%7d.%1dA %8dW" // " 10.6A  1534W"

#define page2_line1 "Boiler %3s %3s %2d.%1d°"
#define page2_line2 "Relay %2d°   Pipe %2d°"
#define page2_line3 "Energy %8d.%1dkWh"
#define page2_line4 "%3d.%1dV %3d.%1dA %2d.%1dkW"


uint8_t page = 0;
uint8_t show_min = 0;
uint8_t refresh_time = 5;

void ICACHE_FLASH_ATTR update_LCD(uint8_t page);

//************************** ADC *****************************************
#define ADC_divider_A 0,296
#define ADC_divider_mA 296
#define ADC_zero 128
#define MIN_CUR_PUMP 300

//#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL)
#define millis() (uint32_t) (micros() / 1000ULL)

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

enum Gpio_State {
	OFF,
	ON
};

typedef union  {
    float f;
    struct {
        uint8_t b[4];
    };
} PowerRegEnergyData;

typedef struct PowerRegData {
    uint8_t mode;
    uint8_t duty;
	uint8_t prev_duty;
    uint32_t uptime;
    PowerRegEnergyData v;
    PowerRegEnergyData c;
    PowerRegEnergyData p;
    PowerRegEnergyData e;
} PowerRegData_t;

PowerRegData_t powerRegData;
uint8_t i2c_busy = 0;


typedef void (*func_cb)(void *arg);

typedef enum {
	ENCODER_ROTATE_ZERO,
	ENCODER_ROTATE_LEFT,    
    ENCODER_ROTATE_RIGHT	
} encoder_direction_t;


typedef struct mcp23017_encoder {
    uint16_t mcp_pin_dt;
    uint16_t mcp_pin_clk;
    uint16_t mcp_pin_btn;
    uint8_t intr_a_pin;
    uint8_t intr_b_pin;
    encoder_direction_t direction;
    int32_t position;
    func_cb left;
    func_cb right;
    func_cb press;
    func_cb release;
	uint32_t debounce;
} mcp23017_encoder_t;

mcp23017_encoder_t mcp23017_encoder;

uint8_t display_backlight = ON;

void turn_on_display() {
	if ( !display_backlight ) {
		display_backlight = 1;
		GPIO_ALL(GPIO_LCD_BACKLIGHT, display_backlight);
	}
	// TODO: start timer for 30 sec to turn off display
}

void turn_off_display(){
	display_backlight = 0;
	GPIO_ALL(GPIO_LCD_BACKLIGHT, display_backlight);
	
}


void turn_on_boiler() {
	// TODO: напрямую через MCPwrite_reg16
	GPIO_ALL(MCP23017_IOT_LED_RED, ON);
	
	
	if ( BOILER_VIA_POWERREG == ON ) {
		sensors_param.cfgdes[1] = powerRegData.prev_duty;
		//check_valdes_cb();
	} else {
		GPIO_ALL(ESP_GPIO_BOILER_FULL_POWER, ON);
	}
	
		
}

void turn_off_boiler() {
	GPIO_ALL(MCP23017_IOT_LED_RED, OFF);
	
	
	if ( BOILER_VIA_POWERREG == ON ) {
		if ( powerRegData.duty > 0 ) powerRegData.prev_duty = powerRegData.duty;
		sensors_param.cfgdes[1] = OFF;
		// //check_valdes_cb();
	} else {
		GPIO_ALL(ESP_GPIO_BOILER_FULL_POWER, OFF);
	}
	
		
}

void turn_on_pump() {
	GPIO_ALL(MCP23017_IOT_LED_BLUE, ON);
	GPIO_ALL(ESP_GPIO_PUMP, ON);
	
	//uint16_t _val = MCPread_reg16(0, GPIOA);
	//MCP_GPIO_SET(_val, MCP23017_LED_BLUE);
	//MCPwrite_reg16(0, GPIOA, _val);		
}

void turn_off_pump(){
	GPIO_ALL(MCP23017_IOT_LED_BLUE, OFF);
	GPIO_ALL(ESP_GPIO_PUMP, OFF);
	
	//uint16_t _val = MCPread_reg16(0, GPIOA);
	//MCP_GPIO_CLEAR(_val, MCP23017_LED_BLUE);
	//MCPwrite_reg16(0, GPIOA, _val);	
}


void turn_on_vent(){
	GPIO_ALL(MCP23017_IOT_LED_WHITE, ON);
	GPIO_ALL(ESP_GPIO_VENT, ON);	
	
	//uint16_t _val = MCPread_reg16(0, GPIOA);
	//MCP_GPIO_SET(_val, MCP23017_LED_WHITE);
	//MCPwrite_reg16(0, GPIOA, _val);	
}

void turn_off_vent(){
	GPIO_ALL(MCP23017_IOT_LED_WHITE, OFF);
	GPIO_ALL(ESP_GPIO_VENT, OFF);	
	
	//uint16_t _val = MCPread_reg16(0, GPIOA);
	//MCP_GPIO_CLEAR(_val, MCP23017_LED_WHITE);
	//MCPwrite_reg16(0, GPIOA, _val);
}

void turn_on_hotcab(){
	GPIO_ALL(MCP23017_IOT_LED_GREEN, ON);
	GPIO_ALL(ESP_GPIO_HOTCAB, ON);	
	
	//uint16_t _val = MCPread_reg16(0, OLATA);
	//MCP_GPIO_SET(_val, MCP23017_LED_GREEN);
	//_val = _val | MCP23017_LED_GREEN;
	//MCPwrite_reg16(0, OLATA, _val);
}

void turn_off_hotcab(){
	GPIO_ALL(MCP23017_IOT_LED_GREEN, OFF);
	GPIO_ALL(ESP_GPIO_HOTCAB, OFF);	
	
	//uint16_t _val = MCPread_reg16(0, OLATA);
	//_val = _val & ~MCP23017_LED_GREEN;
	//MCP_GPIO_CLEAR(_val, MCP23017_LED_GREEN);
	//MCPwrite_reg16(0, OLATA, _val);
}

void encoder_turn_left(void *arg){
	if ( page > 0 ) 
		page--;
	else 
		page = PAGES_COUNT-1;
	
	update_LCD(page);
    turn_on_display();	
}

void encoder_turn_right(void *arg){
	if ( page < PAGES_COUNT-1 ) 
		page++;
	else 
		page = 0;
	update_LCD(page);
    turn_on_display();
}

void encoder_button_push(void *arg){
	if ( display_backlight ) {
		turn_off_display();
	} else {
		turn_on_display();
	}
}

void encoder_button_release(void *arg){

}

void encoder_rotate_isr_handler(uint16_t val) {
    uint8_t level_clk = val & mcp23017_encoder.mcp_pin_clk ? 1 : 0;
    uint8_t level_dt = val & mcp23017_encoder.mcp_pin_dt ? 1 : 0;

	static uint32_t t = 0;
    static uint8_t prev_dt = 0;
    if ( !level_dt && prev_dt 
			&& (millis()-t >= mcp23017_encoder.debounce) 
		) {
        if ( level_clk != level_dt ) {
            mcp23017_encoder.direction = ENCODER_ROTATE_RIGHT;
            mcp23017_encoder.position++;
            mcp23017_encoder.right(NULL);
        } else {
            mcp23017_encoder.direction = ENCODER_ROTATE_LEFT;
            mcp23017_encoder.position--;
            mcp23017_encoder.left(NULL);
        }
		t = millis();
    }
    prev_dt = level_dt; 
}


void encoder_push_isr_handler(uint16_t val) {
    uint8_t level = val & mcp23017_encoder.mcp_pin_btn ? 1 : 0;
    
    if ( 
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 0) ||
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_POSEDGE  && level == 1)
        ) 
    {
        mcp23017_encoder.press(NULL);
    }

    if ( 
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 1) ||
            ( MCP23017_ENCODER_BTN_INTR_TYPE == GPIO_PIN_INTR_POSEDGE  && level == 0)
        ) 
    {
        mcp23017_encoder.release(NULL);
    }

}

void mcp23017_btn_red_isr_handler(uint16_t val) {
	uint8_t level = val & MCP23017_BTN_RED ? 1 : 0;
	if ( MCP23017_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 0 ) {
		
		if ( powerRegData.duty > 0 ) {
			turn_off_boiler();
		} else {
			turn_on_boiler();
		}
	}
}

void mcp23017_btn_blue_isr_handler(uint16_t val) {
	uint8_t level = val & MCP23017_BTN_BLUE ? 1 : 0;
	uint8_t st = GPIO_ALL_GET(ESP_GPIO_PUMP);
	
	if ( MCP23017_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 0 ) {
		// press 
		if ( st == ON ) {
			turn_off_pump();
		} else {
			turn_on_pump();
		}		
	} else {
		// release
	}
	
	

}

void mcp23017_btn_white1_isr_handler(uint16_t val) {
	uint8_t st = GPIO_ALL_GET(ESP_GPIO_VENT);
	uint8_t level = val & MCP23017_BTN_WHITE1 ? 1 : 0;
	
	if ( MCP23017_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 0 ) {
		if ( st == ON ) {
			turn_off_vent();
		} else {
			turn_on_vent();
		}
	}
}

void mcp23017_btn_white2_isr_handler(uint16_t val) {
	// включить / выключить греющий кабель
	// включить / выключить индикацию
	// изменить valdesX
	
	// TODO: включить / выключить термостат,
	uint8_t st = GPIO_ALL_GET(ESP_GPIO_HOTCAB);
	uint8_t level = val & MCP23017_BTN_WHITE2 ? 1 : 0;
	
	if ( MCP23017_BTN_INTR_TYPE == GPIO_PIN_INTR_NEGEDGE  && level == 0 ) {
		if ( st == ON ) {
			turn_off_hotcab();
		} else {
			turn_on_hotcab();
		}	
	}
}

void mcp23017_intr_handler()
{
	i2c_busy = 1;
    uint32_t gpio_st = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
    ETS_GPIO_INTR_DISABLE();

    if  ( 
            (mcp23017_encoder.intr_b_pin != 255 && gpio_st & BIT( mcp23017_encoder.intr_b_pin )) 
            || ( mcp23017_encoder.intr_a_pin != 255 && gpio_st & BIT( mcp23017_encoder.intr_a_pin ))
        )
    {
        uint16_t _int = MCPread_reg16(0, INTFA); // считываем данные с mcp23017
        uint16_t _cap = MCPread_reg16(0, INTCAPA); // считываем данные с mcp23017 // чтение снимка ножек при прерывании сбрасывает прерывание
        if ( _int & mcp23017_encoder.mcp_pin_dt ||  _int & mcp23017_encoder.mcp_pin_clk )  
        {
            encoder_rotate_isr_handler(_cap);
        }
		else
        if ( _int & mcp23017_encoder.mcp_pin_btn  ) {
            encoder_push_isr_handler( _cap );
        }
		else
		if (  _int & MCP23017_BTN_RED  ) {
			mcp23017_btn_red_isr_handler( _cap );
		}
		else
		if (  _int & MCP23017_BTN_BLUE  ) {
			mcp23017_btn_blue_isr_handler( _cap );
		}		
		else
		if (  _int & MCP23017_BTN_WHITE1  ) {
			mcp23017_btn_white1_isr_handler( _cap );
		}		
		else
		if ( _int & MCP23017_BTN_WHITE2 ) {
			mcp23017_btn_white2_isr_handler( _cap );
		}		
    }

    GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_st);
    ETS_GPIO_INTR_ENABLE();
	i2c_busy = 0;
}


void ICACHE_FLASH_ATTR mcp23017_encoder_init() {
    mcp23017_encoder.mcp_pin_clk = MCP23017_ENCODER_CLK_PIN;
    mcp23017_encoder.mcp_pin_dt = MCP23017_ENCODER_DT_PIN;
    mcp23017_encoder.mcp_pin_btn = MCP23017_ENCODER_BTN_PIN;
    mcp23017_encoder.intr_a_pin = 255; //MCP23017_INTA_PIN;
    mcp23017_encoder.intr_b_pin = MCP23017_INTB_PIN;

    mcp23017_encoder.direction = ENCODER_ROTATE_ZERO;
    mcp23017_encoder.position = 0;

    mcp23017_encoder.left = encoder_turn_left;
    mcp23017_encoder.right = encoder_turn_right;
    mcp23017_encoder.press = encoder_button_push;
    mcp23017_encoder.release = encoder_button_release;

    // запретить gpio5 на output, т.е. сделать INPUT
    //gpio_output_set(0, 0, 0, (1 << MCP23017_INTB));  //GPIO_DIS_OUTPUT(gpio_no) 
    if ( mcp23017_encoder.intr_b_pin != 255 )
        GPIO_DIS_OUTPUT( mcp23017_encoder.intr_b_pin);


    if ( mcp23017_encoder.intr_a_pin != 255 )
        GPIO_DIS_OUTPUT( mcp23017_encoder.intr_a_pin);

    MCPwrite_reg16(0, GPINTENA, mcp23017_encoder.mcp_pin_clk | mcp23017_encoder.mcp_pin_dt | mcp23017_encoder.mcp_pin_btn | MCP23017_BTN_RED | MCP23017_BTN_BLUE | MCP23017_BTN_WHITE1 | MCP23017_BTN_WHITE2); 
    MCPwrite_reg16(0, INTCONA, 0);

    // условия срабатывания прерываний, если на пинах значение отличается от  заданного ниже (DEFVAL  = 1 )
    MCPwrite_reg16(0, DEFVALA, 0b1111111111111111);

    ETS_GPIO_INTR_DISABLE();  
    ETS_GPIO_INTR_ATTACH(mcp23017_intr_handler,NULL);
    
    if ( mcp23017_encoder.intr_b_pin != 255 )
        gpio_pin_intr_state_set(GPIO_ID_PIN(mcp23017_encoder.intr_b_pin),GPIO_PIN_INTR_NEGEDGE);
    

    if ( mcp23017_encoder.intr_a_pin != 255 )
        gpio_pin_intr_state_set(GPIO_ID_PIN(mcp23017_encoder.intr_a_pin),GPIO_PIN_INTR_NEGEDGE);

    ETS_GPIO_INTR_ENABLE();    
}

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
    if ( i2c_busy ) return;
    i2c_busy = 1;
    uint8_t val  = _power_reg_read_gpio_mode();
    if ( val != 255) powerRegData.mode = val;
    i2c_busy = 0;
}

void ICACHE_FLASH_ATTR power_reg_get_duty() {
    if ( i2c_busy ) return;
    i2c_busy = 1;
    uint8_t val = _power_reg_read_duty();
    if ( val != 255 ) powerRegData.duty = val;
    i2c_busy = 0;
}

void ICACHE_FLASH_ATTR power_reg_get_energy() {
    if ( i2c_busy ) return;
    i2c_busy = 1;
    _power_reg_read_energy();
    i2c_busy = 0;

    if ( powerRegData.v.f < 0 || powerRegData.v.f > 400 ) powerRegData.v.f = 0.0;
    if ( powerRegData.c.f < 0 || powerRegData.c.f > 40 ) powerRegData.c.f = 0.0;
    if ( powerRegData.p.f < 0 || powerRegData.p.f > 15000 ) powerRegData.p.f = 0.0;
    if ( powerRegData.e.f < 0 ) powerRegData.e.f = 0.0;

    valdes[2] = powerRegData.v.f * 10;
    valdes[3] = powerRegData.c.f * 10;
    valdes[4] = (uint32_t)powerRegData.p.f;
    valdes[5] = (uint32_t)powerRegData.e.f;
}

void ICACHE_FLASH_ATTR power_reg_write_mode(uint8_t _mode) {
    i2c_write_cmd_data(POWER_REG_ADDR, POWER_REG_CMD_SET_GPIO_MODE, _mode);
}

bool ICACHE_FLASH_ATTR power_reg_set_mode(uint8_t _mode) {
    //os_timer_disarm(&power_reg_mode_timer);
    if ( i2c_busy ) {
        return false;
        // start onetime timer
        //os_timer_setfn(&power_reg_mode_timer, (os_timer_func_t *)power_reg_set_mode, _mode);
        //os_timer_arm(&power_reg_mode_timer, 125, 0);
    } else {
        i2c_busy = 1;
        power_reg_write_mode( _mode );
        i2c_busy = 0;
        return true;
    }
}

void ICACHE_FLASH_ATTR power_reg_write_duty(uint8_t _duty) {
    i2c_write_cmd_data(POWER_REG_ADDR, POWER_REG_CMD_SET_DUTY, _duty);
}

bool ICACHE_FLASH_ATTR power_reg_set_duty(uint8_t _duty) {
    //os_timer_disarm(&power_reg_duty_timer);
    if ( i2c_busy ) {
        return false;
        // start onetime timer
        //os_timer_setfn(&power_reg_duty_timer, (os_timer_func_t *)power_reg_set_duty, _duty);
        //os_timer_arm(&power_reg_duty_timer, 125, 0);
    } else {
        i2c_busy = 1;
        power_reg_write_duty( _duty );
        i2c_busy = 0;
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
    if ( i2c_busy ) return;
    i2c_busy = 1;
    uint32_t uptime = power_reg_read_uptime();
    i2c_busy = 0;
    
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
	
	mcp23017_encoder.debounce =  sensors_param.cfgdes[2] > 1000 ? 1000 : sensors_param.cfgdes[2];
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
		
		//if ( BOILER_VIA_POWERREG == 1 ) {
			//GPIO_ALL(MCP23017_IOT_LED_RED, powerRegData.duty );
			//GPIO_ALL(MCP23017_IOT_LED_YELLOW, powerRegData.mode);
		//}		
    }


}


void ICACHE_FLASH_ATTR handle_leds() {
	uint8_t  val = 0;
	
	val = GPIO_ALL_GET(ESP_GPIO_HOTCAB);
	GPIO_ALL(MCP23017_IOT_LED_GREEN, val);

	val = GPIO_ALL_GET(ESP_GPIO_VENT);
	GPIO_ALL(MCP23017_IOT_LED_WHITE, val);

	val = GPIO_ALL_GET(ESP_GPIO_PUMP);
	GPIO_ALL(MCP23017_IOT_LED_BLUE, val);
	
	if ( BOILER_VIA_POWERREG == 1 ) {
		GPIO_ALL(MCP23017_IOT_LED_RED, powerRegData.duty > 0 );	
		GPIO_ALL(MCP23017_IOT_LED_YELLOW, powerRegData.mode);				
	} else {
		val = GPIO_ALL_GET(ESP_GPIO_BOILER_FULL_POWER);
		GPIO_ALL(MCP23017_IOT_LED_RED, val);		
		
		val = GPIO_ALL_GET(ESP_GPIO_BOILER_HALF_POWER);
		GPIO_ALL(MCP23017_IOT_LED_YELLOW, val);				
	}
}


void ICACHE_FLASH_ATTR update_LCD(uint8_t page) {
	char lcd_line_text[40] = "";
	uint8_t   lcd_line = 0;
	
	int16_t adc0 = 0;
	int16_t adc1 = 0;
	int16_t adc2 = 0;
	int16_t adc3 = 0;

	adc0 = abs(ADCdata[1] - 128)*296;
	adc1 = abs(ADCdata[3] - 128)*296;
	adc2 = abs(ADCdata[0] - 128)*296;
	adc3 = abs(ADCdata[2] - 128)*296;
	int16_t temp = 0;
	
	//if ( timersrc % refresh_time==0) show_min = !show_min;
	uint8_t boiler_state = 	( BOILER_VIA_POWERREG && powerRegData.duty > 0 ) || ( !BOILER_VIA_POWERREG && (GPIO_ALL_GET(ESP_GPIO_BOILER_FULL_POWER) || GPIO_ALL_GET(ESP_GPIO_BOILER_HALF_POWER)));					
	uint8_t boiler_status = powerRegData.c.f > 1 && boiler_state;
			
	switch (page) {
		case 0: 
			// ***************** page 1, line 1 ******************************
			os_memset(lcd_line_text, 0, 40);
			uint8_t pump_status = ( adc1 >= MIN_CUR_PUMP && GPIO_ALL_GET(ESP_GPIO_PUMP) );
			valdes[7] = pump_status;
			os_sprintf(lcd_line_text, page1_line2
												, pump_status ? "ON " : "OFF"
												, (GPIO_ALL_GET(ESP_GPIO_PUMP) ) ? "ON " : "OFF"
												);
			LCD_print(lcd_line, lcd_line_text);	
			
			// ***************** page 1, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 1;
			


			os_sprintf(lcd_line_text, page1_line1
										, boiler_state ? "ON " : "OFF"
										, boiler_status ? "ON " : "OFF"
										);
			LCD_print(lcd_line, lcd_line_text);	
			
			// ***************** page 1, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 2;

			os_sprintf(lcd_line_text, page1_line3
												, (int)(data1wire[2] / 100)
												, (int)(data1wire[2] % 100)/10
												, (int)(data1wire[3] / 100)
												, (int)(data1wire[3] % 100)/10
												);
			LCD_print(lcd_line, lcd_line_text);	
			
			
			// ***************** page 1, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 3;
			if (show_min == 0) {
				uint16_t hum = sht_h;
				os_sprintf(lcd_line_text, page1_line4_1, (int)(sht_h / 10), (int)(sht_h % 10), (int)(sht_t / 10), (int)(sht_t % 10)); 
			} else {
				os_sprintf(lcd_line_text, page1_line4_2, time_loc.hour, time_loc.min, time_loc.day ,time_loc.month, time_loc.year, (int)(sht_t / 10), (int)(sht_t % 10)); 
			}
			LCD_print(lcd_line, lcd_line_text);				
			break;
		case 1:
		/*
		#define page2_line1 "Boiler %3s %3s %2d.%1d°"
#define page2_line2 "Relay %2d°   Pipe  %2d°"
#define page2_line3 "Energy %8d.%1dkWh"
#define page2_line4 "%3d.%1dV %3d.%1dA %2d.1dkW"

		*/
			// ***************** page 2, line 1 ******************************
			lcd_line = 0;
			os_memset(lcd_line_text, 0, 40);	
			os_sprintf(lcd_line_text, page2_line1
									, boiler_state ? "ON " : "OFF"
									, boiler_status ? "ON " : "OFF"
									, (int)(data1wire[2] / 100)
									, (int)(data1wire[2] % 100)/10
									);	
			LCD_print(lcd_line, lcd_line_text);	
			
			// ***************** page 2, line 2 ******************************
			lcd_line = 1;
			os_memset(lcd_line_text, 0, 40);	
			os_sprintf(lcd_line_text, page2_line2
									, (int)(data1wire[3] / 100)
									, (int)(data1wire[2] / 100)
									);	
			LCD_print(lcd_line, lcd_line_text);
			
			// ***************** page 2, line 3 ******************************
			lcd_line = 2;
			os_memset(lcd_line_text, 0, 40);	
			os_sprintf(lcd_line_text, page2_line3
											, (int)(powerRegData.e.f / 1000)
											, (int)(powerRegData.e.f * 1000) % 1000
											);	
			LCD_print(lcd_line, lcd_line_text);			

			// ***************** page 2, line 4 ******************************
			lcd_line = 3;
			os_memset(lcd_line_text, 0, 40);	
			os_sprintf(lcd_line_text, page2_line4
											, (int)(powerRegData.v.f)
											, (int)(powerRegData.v.f * 10) % 10
											, (int)(powerRegData.c.f )
											, (int)(powerRegData.c.f * 10) % 10
											, (int)(powerRegData.p.f / 1000)
											, (int)(powerRegData.p.f)  % 1000
											);	
			LCD_print(lcd_line, lcd_line_text);	
			break;
		default: break;
	}
}


void ICACHE_FLASH_ATTR startfunc(){
    load_options();
	powerRegData.prev_duty = powerRegData.duty;
	
    power_reg_set_mode( powerRegData.mode );
    delay(10);
    power_reg_set_duty( powerRegData.duty ); 
    delay(10);

	mcp23017_encoder_init();   
	handle_leds();
	
    #if mqtte || mqttjsone
        mqtt_client = (MQTT_Client*) &mqttClient;
	    os_timer_disarm(&mqtt_send_timer);
	    os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	    os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);        
    #endif
 
 	os_timer_disarm(&check_valdes_timer);
	os_timer_setfn(&check_valdes_timer, (os_timer_func_t *)check_valdes_cb, NULL);
	os_timer_arm(&check_valdes_timer, CHECK_VALDES_INTERVAL, 1);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {


	mcp23017_encoder.debounce =  sensors_param.cfgdes[2] > 1000 ? 1000 : sensors_param.cfgdes[2];
	if ( !i2c_busy ) {
		i2c_busy = 1;
		handle_leds();
		
		if ( timersrc % LCD_UPDATE_TIME == 0 )
			update_LCD(page);
		
		i2c_busy = 0;
	}
	
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
    os_sprintf(HTTPBUFF,"<br>pos: %d", mcp23017_encoder.position );
    os_sprintf(HTTPBUFF,"<br>Версия: %s", FW_VER );
}