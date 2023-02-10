/*
 TODO:
	- часто получаю нули в параметрах, увеличить задержку или отправлять предыдущее значение
	- фиксировать в eeprom данные расхода за день, неделю, месяц
	- фиксировать максимальное напряжение + время
	- фиксировать минимальное напряжение + время
	- фиксировать максимальный ток + время
	- определять перегрузку, если ток выше Х более Y сек
	- если есть перегрузка, то отключать нагрузки по приоритетам, отправлять udp get запрос
	- 
 */
#include "driver/uart.h"

#define FW_VER_NUM "2.0"

	/*
	* SDM Task Delay
	* MQTT Send Interval
	* Значения превышения по току
	* время срабатывания (мсек)
	* время отпускания сек

	sensors_param.cfgdes -  12
		cfgdes[0] - задержка чтения данных с SDM
		cfgdes[1] - время отправки данных с SDM по mqtt
		cfgdes[2] - превышение по току	
		cfgdes[3] - время определения перегрузки, мсек, control_current_delay
		cfgdes[4] - время определения отсутствия перегрузки, сек, control_load_on_delay

	*/
	#define SENS sensors_param
	#define SENSCFG SENS.cfgdes

#define DELAYED_START				60   //sec
#define UART_READ_TIMEOUT			1000  // влияет на результаты чтения из юсарт
#define CUT_OFF_INCORRECT_VALUE		1	// если ток превышает 100А, напряжение 400В (или 0В), мощность 25 кВт, то текущему значению присваивается предыдущее
#define UART_BUFFER_RAED_DELAY 		30
#define SDM_PAUSE_TASK 				50 // msec

#define CHECK_ERROR_COUNT			100

#define RESET_LOAD_GPIO				2
#define GPIO_OFF					0
#define GPIO_ON						1

#define SDM_ADDR					0x0001
#define SDM_VOLTAGE 				0x0000
#define SDM_CURRENT 				0x0006
#define SDM_POWER   				0x000C
#define SDM_ENERGY  				(uint16_t)0x0156
#define SDM_ENERGY_RESETTABLE  		(uint16_t)0x0180
#define SDM_NO_COMMAND				0xFFFF

#define BUF_SIZE (1024)      

#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL) 
#define high_byte(val) (uint8_t) ( val >> 8 )
#define low_byte(val) (uint8_t) ( val & 0xFF )

#define pauseTask(delay)  (vTaskDelay(delay / portTICK_PERIOD_MS))


typedef struct SDMCommand_request {
    uint8_t addr;
    uint8_t func_code;		// input "04" or holding register "03"
    uint8_t start_addr[2];	//start address register for request - hugh byte, low byte
    uint8_t count_reg[2];	// count of registers for request -  high byte, low byte
    uint8_t crc[2];			// crc - low byte, high byte
} SDMCommand_request_t;

#define REQUEST_SIZE sizeof(SDMCommand_request_t)


typedef struct SDMCommand_response {
    uint8_t addr;
    uint8_t func_code;		// input "04" or holding register "03"
    uint8_t byte_cnt;		// bytes count in response
    uint8_t resp_data[4];   // response data: reg1_hi, reg1_low, reg2_hi, reg2_low
    uint8_t crc[2];			// crc - low byte, high byte
} SDMCommand_response_t;

#define RESPONSE_SIZE sizeof(SDMCommand_response_t)
#define RESPONSE_DATA_SIZE 4


#define MQTT_SEND_INTERVAL 	10 // sec
#define VOLTAGE_MQTT_TOPIC_PARAM	"pmv"
#define CURRENT_MQTT_TOPIC_PARAM	"pmc"
#define POWER_MQTT_TOPIC_PARAM		"pmw"
#define ENERGY_MQTT_TOPIC_PARAM		"pmwh"
#define mqtt_send_interval_sec 		SENSCFG[1]
#define OVERLOAD_MQTT_TOPIC_PARAM	"overload"
TimerHandle_t mqtt_send_timer;	
void vMqttSendTimerCallback( TimerHandle_t xTimer );



float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float energy_resettable = 0;

uint8_t delayed_counter = DELAYED_START;

static TimerHandle_t system_start_timer;
//static volatile os_timer_t system_start_timer; 
void vSystemStartTimerCallback( TimerHandle_t xTimer );

// uart0
void send_buffer(const uint8_t *buffer, uint8_t len);
uint8_t read_buffer(uint8_t *buffer, uint8_t cnt);

uint8_t sdm_enabled = 0;
void read_electro_task( void * pvParameters );
void read_voltage();
void read_current();
void read_power();
void read_energy();
void read_energy_resettable();

uint16_t sdm_crc(uint8_t *data, uint8_t sz);
// sdm < -- > uart
void sdm_send (uint8_t addr, uint8_t fcode, uint32_t cmd);
float sdm_read(uint8_t addr, uint8_t fcode);

float sdm_voltage(uint8_t addr);
float sdm_current(uint8_t addr);
float sdm_power(uint8_t addr);
float sdm_energy(uint8_t addr);
float sdm_energy_resettable(uint8_t addr);

#ifdef DEBUG
void userlog(const char *fmt, ...) {
	char *str = (char *) malloc(100);
	memset(str, 100, 0);
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(str, 100, fmt, args);
    va_end(args);
	uart_write_bytes(UART_NUM_1, str, len);
	free(str);
	str = NULL;
}
#endif

static void uart_init() {
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 100, NULL, 0);	
    
#ifdef DEBUG	
	uart_param_config(UART_NUM_1, &uart_config);
	uart_driver_install(UART_NUM_1, BUF_SIZE * 2, BUF_SIZE * 2, 100, NULL);	
	os_install_putc1(userlog);
#endif	
}

void mqttSendInt(const char *topic, uint32_t value){
	char payload[20];
	os_sprintf(payload,"%d", value);
	MQTT_Publish(topic, payload, os_strlen(payload), 2, 0, 0);
}

void mqttSendFloat(const char *topic, float value, uint8_t divider){
	char payload[20];
	if (divider == 0) {
		os_sprintf(payload,"%d", (int)value);
	} else {
		os_sprintf(payload,"%d.%d", (int)value, (int)(value * divider) % divider);
	}
	MQTT_Publish(topic, payload, os_strlen(payload), 2, 0, 0);
}

void get_config() { 
	uint8_t needSave = 0;
	if (sdm_task_delay == 0) {
		sdm_task_delay = SDM_PAUSE_TASK_MS;
		needSave = 1;
	}
	
	if (mqtt_send_interval_sec < 2) {
		mqtt_send_interval_sec = sensors_param.mqttts;
		needSave = 1;
	}

	uint16_t prev_treshhold = overload_treshold;
	if (overload_treshold == 0 || overload_treshold > 300) {
		overload_treshold = OVERLOAD_TRESHOLD;
		needSave = 1;
	}

	if ( prev_treshhold != overload_treshold ) {
		// поменялось значение в настройках, надо обновить valdes[0]
		prev_treshhold = overload_treshold;
		valdes[0] = overload_treshold;
		mqttSendInt("valdes1", valdes[0]);
	} else {
		// значение не менялось, но могло поменяться в valdes[0]
		uint16_t tmp_current_treshold = valdes[0];  // получили по mqtt или через get, но здесь может быть и предыдущее значение
		if ( tmp_current_treshold > 0 && tmp_current_treshold != overload_treshold ) {  
			// значение в valdes[0] отличается от текущего и в опциях
			overload_treshold = tmp_current_treshold;
			needSave = 1;
		}
	}

	if (overload_detect_delay == 0) {
		overload_detect_delay = OVERLOAD_DETECT_DELAY_MS;
		needSave = 1;
	}

	if (overload_time == 0) {
		overload_time = OVERLOAD_TIME;
		needSave = 1;
	}

	if ( needSave == 1) {
		SAVEOPT;
	}
}

void vSystemStartTimerCallback( TimerHandle_t xTimer ){
#ifdef DEBUG
	userlog("\n%s\n", __func__);
#endif	
	xTaskCreate(read_electro_task, "read_electro_task", 2048, NULL, 5, NULL); 

#ifdef MQTTD
	mqtt_send_timer = xTimerCreate("mqtt send timer", pdMS_TO_TICKS( mqtt_send_interval_sec * 1000 ), pdTRUE, 0, vMqttSendTimerCallback);
	xTimerStart( mqtt_send_timer, 0);
#endif

}

void sdm_send (uint8_t addr, uint8_t fcode, uint32_t reg) {
	SDMCommand_request_t sdm;
	sdm.addr = addr;
	sdm.func_code = fcode;
	sdm.start_addr[0] = high_byte(reg);
	sdm.start_addr[1] = low_byte(reg);
  	sdm.count_reg[0] = 0;  //high
	sdm.count_reg[1] = 2;  //low

	uint8_t *bytes = (uint8_t*)&sdm;
	uint16_t crc = sdm_crc(bytes, sizeof(sdm) - 2 );	
	sdm.crc[0] = low_byte(crc);
	sdm.crc[1] = high_byte(crc);
	send_buffer(bytes, sizeof(sdm));
	pauseTask(UART_BUFFER_RAED_DELAY);
}

float sdm_read(uint8_t addr, uint8_t fcode) {
	float res = 0;
	//SDMCommand_response_t sdm;
	uint8_t *buf = (uint8_t *) malloc(RESPONSE_SIZE);
	//uint8_t *buf = (uint8_t*)&sdm;
	uint8_t len = read_buffer(buf, RESPONSE_SIZE);
	// validate
	if ( len ==  RESPONSE_SIZE && buf[0] == addr && buf[1] == fcode && buf[2] == RESPONSE_DATA_SIZE ) 
	{
		// check crc
		uint16_t crc = sdm_crc(buf, RESPONSE_SIZE - 2 );	
		if ( buf[6] | (buf[7] << 8) == crc) {
          ((uint8_t*)&res)[2]= buf[4];
          ((uint8_t*)&res)[3]= buf[3];
          ((uint8_t*)&res)[1]= buf[5];
          ((uint8_t*)&res)[0]= buf[6];
		}
		#ifdef DEBUG
		else {
			userlog("FAIL: crc invalid \n");
		}
		#endif
	}
	vTaskDelay(100);

	#ifdef DEBUG
	else {
		userlog("FAIL: response data invalid \n");
		if ( len != RESPONSE_SIZE ) userlog("FAIL: len != RESPONSE_SIZE: %d %d\n", RESPONSE_SIZE, len);
		if ( buf[0] != addr ) userlog("FAIL: address mismatch: %d %d\n", addr, buf[0]);
		if ( buf[1] != fcode ) userlog("FAIL: fcode mismatch: %d %d\n", fcode, buf[1]);
		if ( buf[2] != RESPONSE_DATA_SIZE ) userlog("FAIL: response data size mismatch: %d %d\n", RESPONSE_DATA_SIZE, buf[2]);
	}
	#endif

	free(buf);
	buf = NULL;
	return res;
}

float sdm_voltage(uint8_t addr) {
	#ifdef DEBUG
		userlog("[%d]  %s:     ", millis(), __func__);
		uint32_t duration = millis();
	#endif
	
	sdm_send(addr, 4, SDM_VOLTAGE);
	float value = sdm_read( addr, 4);

	#ifdef DEBUG 
		duration = millis() - duration;	
		userlog("[%d]  voltage: %d.%d   duration: %d\n", millis(),  (int)value,  ( (int) (value*100) % 100), duration);
	#endif

	return value;
}

float sdm_current(uint8_t addr) {
	#ifdef DEBUG
		userlog("[%d]  %s:     ", millis(), __func__);
		uint32_t duration = millis();
	#endif

	sdm_send(addr, 4, SDM_CURRENT);
	float value = sdm_read( addr, 4);
	
	#ifdef DEBUG 
		duration = millis() - duration;
		userlog("[%d]  current: %d.%d   duration: %d\n", millis(),  (int)value,  ( (int) (value*100) % 100), duration);
	#endif

	return value;	
}

float sdm_power(uint8_t addr) {
	#ifdef DEBUG
		userlog("[%d]  %s:     ", millis(), __func__);
		uint32_t duration = millis();
	#endif

	sdm_send(addr, 4, SDM_POWER);
	float value = sdm_read( addr, 4);

	#ifdef DEBUG 
		duration = millis() - duration;
		userlog("[%d]  power: %d.%d   duration: %d\n", millis(),  (int)value,  ( (int) (value*100) % 100), duration);
	#endif

	return value;		
}

float sdm_energy(uint8_t addr) {
	#ifdef DEBUG
		userlog("[%d]  %s:     ", millis(), __func__);
		uint32_t duration = millis();
	#endif

	sdm_send(addr, 4, SDM_ENERGY);
	float value = sdm_read( addr, 4);

	#ifdef DEBUG 
		duration = millis() - duration;
		userlog("[%d]  energy: %d.%d   duration: %d\n", millis(),  (int)value,  ( (int) (value*100) % 100), duration);
	#endif

	return value;			
}

float sdm_energy_resettable(uint8_t addr) {
	#ifdef DEBUG
		userlog("[%d]  %s:     ", millis(), __func__);
		uint32_t duration = millis();
	#endif

	sdm_send(addr, 4, SDM_ENERGY_RESETTABLE);
	float value = sdm_read( addr, 4);

	#ifdef DEBUG 
		duration = millis() - duration;
		userlog("[%d]  energy resettable: %d.%d   duration: %d\n", millis(),  (int)value,  ( (int) (value*100) % 100), duration);
	#endif

	return value;			
}

void read_voltage(){
	if ( !sdm_enabled ) return;	
	float v = sdm_voltage(SDM_ADDR);
	#ifdef CUT_OFF_INCORRECT_VALUE
		voltage = ( v == 0 || v > 400) ? voltage : v;
	#else
		voltage = ( v == 0 ) ? voltage : v;
	#endif	
	pauseTask(SDM_PAUSE_TASK );
}

void read_current(){
	if ( !sdm_enabled ) return;
	float v = sdm_current(SDM_ADDR);
	#ifdef CUT_OFF_INCORRECT_VALUE
		current = ( v == 0 || v > 100) ? current : v;
	#else
		current = ( v == 0) ? current : v;
	#endif	
	pauseTask( SDM_PAUSE_TASK );
}

void read_power(){
	if ( !sdm_enabled ) return;
	float v = sdm_power(SDM_ADDR);

	#ifdef CUT_OFF_INCORRECT_VALUE
		power = ( v == 0 || v > 25000) ? power : v;
	#else	
		power = ( v == 0) ? power : v;
	#endif

	pauseTask( SDM_PAUSE_TASK );
}

void read_energy(){
	if ( !sdm_enabled ) return;
	float v  = sdm_energy(SDM_ADDR);
	#ifdef CUT_OFF_INCORRECT_VALUE
		energy = ( v == 0) ? energy : v;
	#endif	
	pauseTask( SDM_PAUSE_TASK );
}

void read_energy_resettable(){
	if ( !sdm_enabled ) return;
	float v = sdm_energy_resettable(SDM_ADDR);
	#ifdef CUT_OFF_INCORRECT_VALUE
		energy_resettable = ( v == 0) ? energy_resettable : v;
	#endif		
	pauseTask( SDM_PAUSE_TASK );
}

void read_electro_params_c3_v1_c3_p1__e120() {
		for ( uint8_t i=1;i<=120;i++) {
			int m = i % 4;
			if ( i == 120 ) {
				read_energy();
				read_energy_resettable();
				continue;
			}
			if ( m != 0) {
				read_current();
			} else {
				int d = i / 4;
				int mm = d % 2;
				
				if ( mm == 1 ) {
					read_voltage();
				} else {
					read_power();
				}				
			}			
		}
}

void read_electro_params_c20vp__er_10sec() {
	// c20vp - ток 20 раз подряд, 1 раз, 1 раз
	// er_60sec - расход 1 раз в 10 сек
		static uint32_t ts;
		
		for ( uint8_t i=1;i<21;i++) read_current();
		read_voltage();
		read_power();

		if ( millis() - ts> 10 * 1000 ) {
			read_energy();
			read_energy_resettable();
			ts = millis();
		}
}

void read_electro_task( void * pvParameters ){
	for(;;){
		if ( !sdm_enabled ) { 
			vTaskDelete(NULL);
			return;
		}
		
		//read_electro_params_c3_v1_c3_p1__e120();  // старый алгоритм чтения
		read_electro_params_c20vp__er_10sec();     //  новый алгоритм чтения
		pauseTask(SDM_PAUSE_TASK);
	}
	vTaskDelete(NULL);
}

uint16_t sdm_crc(uint8_t *data, uint8_t sz) {
	uint16_t _crc, _flag;
	_crc = 0xFFFF;
	for (uint8_t i = 0; i < sz; i++) {
    	_crc = _crc ^ data[i];
    	for (uint8_t j = 8; j; j--) {
    		_flag = _crc & 0x0001;
    		_crc >>= 1;
    		if (_flag)
        	_crc ^= 0xA001;
    	}
  	}
  	return _crc;	
}

void send_buffer(const uint8_t *buffer, uint8_t len){
	uart_write_bytes(UART_NUM_0, (const char *) buffer, len);
	vTaskDelay(10);
}

uint8_t read_buffer(uint8_t *buffer, uint8_t cnt){
	int8_t result = 0;
	result = uart_read_bytes(UART_NUM_0, buffer, cnt, UART_READ_TIMEOUT / portTICK_RATE_MS );
	
	if 	( result < 0 ) { result = 0;	}
	return result;
}

void vMqttSendTimerCallback( TimerHandle_t xTimer ) {
	mqttSendFloat(VOLTAGE_MQTT_TOPIC_PARAM, voltage, 10);
	mqttSendFloat(CURRENT_MQTT_TOPIC_PARAM, current, 100);
	mqttSendFloat(POWER_MQTT_TOPIC_PARAM, power, 0);
	mqttSendFloat(ENERGY_MQTT_TOPIC_PARAM, energy, 100);
}	



void show_countdown(uint8_t cnt) {
	if ( cnt > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", cnt);
	}
}

void startfunc(){
	// выполняется один раз при старте модуля.
	uart_init();	  
	get_config();
	 // запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	system_start_timer = xTimerCreate("system start timer", pdMS_TO_TICKS( DELAYED_START * 1000 ), pdFALSE, 0, vSystemStartTimerCallback);
	BaseType_t b = xTimerStart( system_start_timer, 0);	
}

void timerfunc(uint32_t  timersrc) {
	// выполнение кода каждую 1 секунду
	get_config();

	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
	
	if ( delayed_counter > 0 ) { 
		delayed_counter--;
	}
	pauseTask(1000);
}

void webfunc(char *pbuf) {
	show_countdown( delayed_counter );
	os_sprintf(HTTPBUFF,"<br><b>Напряжение:</b> %d.%d В", 	(uint8_t)voltage, 		(uint8_t)(voltage*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Сила тока:</b> %d.%d А", 	(uint8_t)current, 		(uint8_t)(current*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Мощность:</b> %d.%d Вт", 	(uint16_t)power,   		(uint8_t)(power*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Расход1:</b> %d.%d кВт*ч", 	(uint32_t)energy, 		(uint8_t)(energy*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Расход2:</b> %d.%d кВт*ч", 	(uint32_t)energy_resettable, 		(uint8_t)(energy_resettable*100) % 100);
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}