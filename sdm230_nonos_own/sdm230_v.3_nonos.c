	#include "../moduls/uart_register.h"
	#include "../moduls/uart.h"
	#include "../moduls/uart.c"

	#define FW_VER "3.16.3"
	
	/*
	Глобальные переменные: 2
	Количество настроек:
	SDM Task Delay, MQTT Send Interval, Отсчека по току Ах10, Отсечка по мощности Вт, Время срабатывания мсек,Задержка после перегрузки сек, Расход вчера x100, Расход сегодня x100, Cчетчик на 00 сегодня, Счетчик на 07 сегодня, Счетчик на 23 сегодня, Счетчик на 00 вчера, Счетчик на 07 вчера, Счетчик на 23 вчера

	* SDM Task Delay
	* MQTT Send Interval
	* Отсчека по току (Ах10)
	* Отсечка по мощности (Вт)
	* Время срабатывания (мсек)
	* время отпускания сек

	sensors_param.cfgdes -  12
		cfgdes[0] - задержка чтения данных с SDM
		cfgdes[1] - время отправки данных с SDM по mqtt
		cfgdes[2] - превышение по току			x10 А
		cfgdes[3] - превышение по мощности      Ватт
		cfgdes[4] - время определения перегрузки, мсек, control_current_delay
		cfgdes[5] - время определения отсутствия перегрузки, сек, control_load_on_delay

		cfgdes[6] - расход вчера  x100
		cfgdes[7] - расход сегодня  x100
		// округляем
		cfgdes[8] - показания счетчика на 00:00 сегодня  
		cfgdes[9] - показания счетчика на 7:00 сегодня   
		cfgdes[10] - показания счетчика на 23:00 сегодня  
		cfgdes[11] - показания счетчика на 00:00 вчера   
		cfgdes[12] - показания счетчика на 07:00 вчера   
		cfgdes[13] - показания счетчика на 23:00 вчера   


		расход сегодня = текущие показания - показания счетчика на 00:00 сегодня
		расход ночью = оказания счетчика на 7:00 сегодня - показания счетчика на 23:00 вчера
		расход днем = показания счетчика на 23:00 сегодня - показания счетчика на 7:00 сегодня

		расход вчера = в 23:59:59 присвоить значения расход сегодня


	*/
	#define SENS sensors_param
	#define SENSCFG SENS.cfgdes

	#define ELECTRO_C20_V1_P1__E10						// читаем 20 раз подряд значение тока, затем 1 раз напряжение, затем 1 раз мощность, и каждое 10е чтение получаем значение потраченной энергии

	#define OVERLOAD_DETECT_DELAY_MS 			100		// мсек, интервал определения перегрузки
	#define OVERLOAD_TIME 						180		// сек,  интервал длительности перегрузки
	#define CURRENT_OVERLOAD_TRESHOLD 			200		// A*10
	#define POWER_OVERLOAD_TRESHOLD 			5000	// Ватт
	#define DELAYED_START						60   	// sec

	#define UART_READ_TIMEOUT					1000  // влияет на результаты чтения из юсарт

	#define CUT_OFF_INCORRECT_VALUE			// если ток превышает 100А, напряжение 400В (или 0В), мощность 25 кВт, то текущему значению присваивается предыдущее
	#define SDM_PAUSE_TASK_MS 					100

	#define CHECK_ERROR_COUNT			100

	#define RESET_LOAD_GPIO				2
	#define GPIO_OFF				0
	#define GPIO_ON				1

	#define SDM_ADDR					0x0001
	#define SDM_NO_COMMAND				0xFFFF
	#define SDM_VOLTAGE 				0x0000
	#define SDM_CURRENT 				0x0006
	#define SDM_POWER   				0x000C
	#define SDM_ENERGY  				(uint16_t)0x0156
	#define SDM_ENERGY_RESETTABLE  		(uint16_t)0x0180

	#define high_byte(val) (uint8_t) ( val >> 8 )
	#define low_byte(val) (uint8_t) ( val & 0xFF )

	typedef struct {
		float value;
		uint32_t dt;
	} dt_value_t;

	typedef struct {
		dt_value_t voltage_min;
		dt_value_t voltage_max;
		dt_value_t current_max;
		int32_t energy_today; // вычисленное значение
		int32_t energy_yesterday; // вычисленное значение
		int32_t energy_00;		// запомним значение счетчика в 00:00
		int32_t energy_07;		// запомним значение счетчика в 07:00
		int32_t energy_23;		// запомним значения счетчика в 23:00
		int32_t energy_00_prev;		// запомним значение счетчика в 00:00
		int32_t energy_07_prev;		// запомним значение счетчика в 07:00
		int32_t energy_23_prev;		// запомним значения счетчика в 23:00


		//float energy_23_07;
		//float energy_07_23;
		uint32_t crc32;
	} rtc_data_t;

	rtc_data_t rtc_data;

	//float energy_yesterday = 0;
	#define ENERGY_YESTERDAY SENSCFG[6]   // x100
	//float energy_today = 0;
	#define ENERGY_TODAY SENSCFG[7]
	#define ENERGY_00 SENSCFG[8]		// max 21_474_836.47
	#define ENERGY_07 SENSCFG[9]
	#define ENERGY_23 SENSCFG[10]
	#define ENERGY_00_Y SENSCFG[11]
	#define ENERGY_07_Y SENSCFG[12]
	#define ENERGY_23_Y SENSCFG[13]

	int32_t energy_23_07 = 0;
	int32_t energy_07_23 = 0;

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

	#define VOLTAGE_TOPIC_L		"l_pmv"
	#define CURRENT_TOPIC_L		"l_pmc"
	#define POWER_TOPIC_L			"l_pmw"
	#define ENERGY_TOPIC_L		"l_pmwh"
	#define ENERGY_TODAY_TOPIC_L		"l_pmwht"
	#define ENERGY_YESTERDAY_TOPIC_L		"l_pmwhy"
	#define OVERLOAD_TOPIC_L		"l_overload"

	#define MQTT_SEND_INTERVAL 10 // sec
	#define VOLTAGE_TOPIC		"pmv"
	#define CURRENT_TOPIC		"pmc"
	#define POWER_TOPIC			"pmw"
	#define ENERGY_TOPIC		"pmwh"
	#define ENERGY_TODAY_TOPIC		"pmwht"
	#define ENERGY_YESTERDAY_TOPIC		"pmwhy"
	#define OVERLOAD_TOPIC		"overload"
	#define MQTT_PAYLOAD_BUF 20
	#define mqtt_send_interval_sec SENSCFG[1]

	os_timer_t mqtt_send_timer;	
	void mqtt_send_cb();


	#define sdm_task_delay SENSCFG[0]

	uint32_t command = SDM_NO_COMMAND;

	volatile dt_value_t volt_min;
	volatile dt_value_t volt_max;
	volatile dt_value_t curr_max;

	volatile float voltage = 0;
	volatile float voltage_prev = 0;
	volatile float current = 0;
	volatile float power = 0;
	volatile float energy = 0;
	volatile float energy_resettable = 0;

	uint8_t delayed_counter = DELAYED_START;

	uint8_t startAfterPowerUp = 0;

	os_timer_t read_electro_timer;
	os_timer_t system_start_timer;
	os_timer_t overload_detect_timer;
	os_timer_t overload_reset_timer;


	uint8_t overload = 0;										// флаг наличия перегрузки
	#define current_overload_treshold SENSCFG[2]					// valdes[0]
	#define power_overload_treshold SENSCFG[3]					// valdes[1]
	#define overload_time SENSCFG[5] //= OVERLOAD_TIME;
	#define overload_detect_delay SENSCFG[4] //= OVERLOAD_DETECT_DELAY_MS;

	uint16_t error_count = 0;
	
	uint32_t getVoltageInt(){return (uint32_t)(voltage * 10);}
	uint32_t getCurrentInt(){return (uint32_t)(current * 100);}
	uint32_t getPowerInt(){return (uint32_t)(power * 100);}
	uint32_t getEnergyInt(){return (uint32_t)(energy * 100);}
	//uint32_t getEnergyTodayInt(){return (uint32_t)(energy_today * 100);}
	uint32_t getEnergyTodayInt(){return (uint32_t)(ENERGY_TODAY);}
	//uint32_t getEnergyYesterdayInt(){return (uint32_t)(energy_yesterday * 100);}
	uint32_t getEnergyYesterdayInt(){return (uint32_t)(ENERGY_YESTERDAY);}
	
#define ADDLISTSENS {200, LSENSFL1 | LS_MODE_VOLT | LSENS32BIT | LSENSFUNS, "Voltage", VOLTAGE_TOPIC_L, getVoltageInt, NULL},\
					{201, LSENSFL2 | LS_MODE_CURRENT | LSENS32BIT | LSENSFUNS, "Current", CURRENT_TOPIC_L, getCurrentInt, NULL},\
					{202, LSENSFL2 | LS_MODE_WATT | LSENS32BIT | LSENSFUNS, "Power", POWER_TOPIC_L, getPowerInt, NULL},\
					{203, LSENSFL2 | LS_MODE_WATTH | LSENS32BIT | LSENSFUNS, "Energy", ENERGY_TOPIC_L, getEnergyInt, NULL},\
					{204, LSENSFL2 | LS_MODE_WATTH | LSENS32BIT | LSENSFUNS, "EnergyT", ENERGY_TODAY_TOPIC_L, getEnergyTodayInt, NULL},\
					{205, LSENSFL2 | LS_MODE_WATTH | LSENS32BIT | LSENSFUNS, "EnergyY", ENERGY_YESTERDAY_TOPIC_L, getEnergyYesterdayInt, NULL},\
					{206, LSENSFL0, "Overload", OVERLOAD_TOPIC_L, &overload, NULL},\

	void system_start_cb( );
	void read_electro_cb();	
	void overload_detect_cb();	
	void overload_reset_cb();	

	// uart0
	void send_buffer(uint8_t *buffer, uint8_t len);
	void read_buffer();
	
	void sdm_send (uint8_t addr, uint8_t fcode, uint32_t reg);
	float sdm_read(uint8_t addr, uint8_t *buffer, uint8_t cnt);

	uint16_t sdm_crc(uint8_t *data, uint8_t sz);

	void request_voltage(uint8_t addr);
	void request_current(uint8_t addr);
	void request_power(uint8_t addr);
	void request_energy(uint8_t addr);
	void request_energy_resettable(uint8_t addr);
	
	#define millis() (uint32_t) (micros() / 1000ULL) 

uint32_t getTimeLocSeconds(){
	return time_loc.hour * 60 * 60 + time_loc.min * 60 + time_loc.sec;
}

void ICACHE_FLASH_ATTR send_buffer(uint8_t *buffer, uint8_t len){
	uart0_tx_buffer(buffer, len);
}

void read_buffer(){	
	static char rx_buf[125];
	static uint8_t i = 0;

	uint32_t ts = micros();

	WRITE_PERI_REG(UART_INT_CLR(UART0),UART_RXFIFO_FULL_INT_CLR);
	while ( READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S) 
			&& ( micros() - ts < UART_READ_TIMEOUT*1000)) 
	{
		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		uint8_t read = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		rx_buf[i] = read; // buffer[i++] = read;
		i++;
		ts = micros();
		if ( RESPONSE_SIZE == i) {
		
			// что то прочитали
			float v;
			v = sdm_read(SDM_ADDR, rx_buf, i);

			switch ( command ) {
				case SDM_VOLTAGE:
					#ifdef CUT_OFF_INCORRECT_VALUE
						voltage = ( v == 0 || v > 400) ? voltage : v;
					#else
						voltage = ( v == 0 ) ? voltage : v;
					#endif
					if ( volt_min.value == 0) {
						volt_min.value = voltage;
						volt_min.dt = getTimeLocSeconds();
					}
					if ( voltage > 0 && voltage < volt_min.value) {
						volt_min.value = voltage;
						volt_min.dt = getTimeLocSeconds();
					}
					if ( voltage > volt_max.value) {
						volt_max.value = voltage;
						volt_max.dt = getTimeLocSeconds();
					}
					break;
				case SDM_CURRENT:
					#ifdef CUT_OFF_INCORRECT_VALUE
						current = ( v == 0 || v > 100) ? current : v;
					#else
						current = ( v == 0) ? current : v;		
					#endif
					if ( current > curr_max.value) {
						curr_max.value = current;
						curr_max.dt = getTimeLocSeconds();
					}
					break;
				case SDM_POWER:
					#ifdef CUT_OFF_INCORRECT_VALUE
						power = ( v == 0 || v > 25000) ? power : v;	
					#else
						power = ( v == 0) ? power : v;
					#endif					
					break;
				case SDM_ENERGY:
					energy = ( v == 0) ? energy : v;
					break;
				case SDM_ENERGY_RESETTABLE:
					energy_resettable = ( v == 0) ? energy_resettable : v;
					break;					
				default:
					break;
			}
			command = SDM_NO_COMMAND;
			i = 0;		
			break;
		}	
	}
}


void ICACHE_FLASH_ATTR mqttSend(const char *topic, int32_t val){
    char payload[MQTT_PAYLOAD_BUF];
	memset(payload, 0, MQTT_PAYLOAD_BUF);
	os_sprintf(payload, "%d", val);
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 2, 0, 0);
}

void ICACHE_FLASH_ATTR mqttSendFloat(const char *topic, float val, int divider){
    char payload[MQTT_PAYLOAD_BUF];
	memset(payload, 0, MQTT_PAYLOAD_BUF);
	if (divider==0){
		os_sprintf(payload, "%d", (int)val);
	} else if (divider==10) {
		os_sprintf(payload, "%d.%d", (int)val, (int)(val*divider) % divider);
	} else if (divider==100) {
		os_sprintf(payload, "%d.%02d", (int)val, (int)(val*divider) % divider);
	} else if (divider==1000) {
		os_sprintf(payload, "%d.%03d", (int)val, (int)(val*divider) % divider);
	}
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 2, 0, 0);
}

void ICACHE_FLASH_ATTR get_config() {

	//-----------------------------------------------------------
	// корректировка неверно введенных значений
	uint8_t needSave = 0;
	if (sdm_task_delay == 0) {
		sdm_task_delay = SDM_PAUSE_TASK_MS;
		needSave = 1;
	}
	
	if (mqtt_send_interval_sec < 2) {
		mqtt_send_interval_sec = sensors_param.mqttts;
		needSave = 1;
	}
	//----------------------------------------------------------
	
	uint16_t prev_treshhold = current_overload_treshold;
	if (current_overload_treshold == 0 || current_overload_treshold > 300) {
		current_overload_treshold = CURRENT_OVERLOAD_TRESHOLD;
		needSave = 1;
	}

	if ( prev_treshhold != current_overload_treshold ) {
		// поменялось значение в настройках, надо обновить valdes[0]
		prev_treshhold = current_overload_treshold;
		valdes[0] = current_overload_treshold;
		//mqttSend("valdes1", valdes[0]);
	} else {
		// значение не менялось, но могло поменяться в valdes[0]
		uint16_t tmp_treshold = valdes[0];  // получили по mqtt или через get, но здесь может быть и предыдущее значение
		if ( tmp_treshold > 0 && tmp_treshold != current_overload_treshold ) {  
			// значение в valdes[0] отличается от текущего и в опциях
			current_overload_treshold = tmp_treshold;
			needSave = 1;
		}
	}

	//----------------------------------------------------------
	
	prev_treshhold = power_overload_treshold;
	if (power_overload_treshold == 0 || power_overload_treshold > 7000) {
		power_overload_treshold = POWER_OVERLOAD_TRESHOLD;
		needSave = 1;
	}

	if ( prev_treshhold != power_overload_treshold ) {
		// поменялось значение в настройках, надо обновить valdes[1]
		prev_treshhold = power_overload_treshold;
		valdes[1] = power_overload_treshold;
		//mqttSend("valdes2", valdes[1]);
	} else {
		// значение не менялось, но могло поменяться в valdes[1]
		uint16_t tmp_treshold = valdes[1];  // получили по mqtt или через get, но здесь может быть и предыдущее значение
		if ( tmp_treshold > 0 && tmp_treshold != power_overload_treshold ) {  
			// значение в valdes[1] отличается от текущего и в опциях
			power_overload_treshold = tmp_treshold;
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


uint32_t ICACHE_FLASH_ATTR calcCRC32(const uint8_t *data, uint16_t sz) {
  // Обрабатываем все данные, кроме последних четырёх байтов,
  // где и будет храниться проверочная сумма.
  size_t length = sz-4;
 
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
	uint32_t i;
    for (i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

void updateRTCData(){
	if ( rtc_data.energy_today == 0) rtc_data.energy_today = ENERGY_TODAY;
	if ( rtc_data.energy_yesterday == 0) rtc_data.energy_yesterday = ENERGY_YESTERDAY;
	if ( rtc_data.energy_00 == 0) rtc_data.energy_00 = ENERGY_00;
	if ( rtc_data.energy_07 == 0) rtc_data.energy_07 = ENERGY_07;
	if ( rtc_data.energy_23 == 0) rtc_data.energy_23 = ENERGY_23;
	if ( rtc_data.energy_00_prev == 0) rtc_data.energy_00_prev = ENERGY_00_Y;
	if ( rtc_data.energy_07_prev == 0) rtc_data.energy_07_prev = ENERGY_07_Y;
	if ( rtc_data.energy_23_prev == 0) rtc_data.energy_23_prev = ENERGY_23_Y;
}

void saveToRTC(){
	rtc_data.crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));
}

void ICACHE_FLASH_ATTR startfunc(){

	overload = 0;
	volt_min.value = 0;
	volt_min.dt = 0;
	volt_max.value = 0;
	volt_max.dt = 0;
	curr_max.value = 0;
	curr_max.dt = 0;

	// выполняется один раз при старте модуля.
	uart_init(BIT_RATE_9600);	  
	ETS_UART_INTR_ATTACH(read_buffer, NULL);

	get_config();

	// читаем rtc
	system_rtc_mem_read(70, &rtc_data, sizeof(rtc_data_t));
	// проверяем crc
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	if ( crc32 != rtc_data.crc32 ) {
		// кривые данные (т.е. пропадало питание), обнулим
		os_memset(&rtc_data, 0, sizeof(rtc_data_t));

		startAfterPowerUp = 1;

		//присвоим rtc_data значения из cfgdes
		updateRTCData();
		saveToRTC();
	} else {
		startAfterPowerUp = 0;
	}

	// если значения сохраненных данных меньше чем данные из rtc
	if (ENERGY_YESTERDAY < rtc_data.energy_yesterday) ENERGY_YESTERDAY = rtc_data.energy_yesterday;
	if (ENERGY_TODAY < rtc_data.energy_today) ENERGY_TODAY = rtc_data.energy_today;
	if (ENERGY_00 < rtc_data.energy_00) ENERGY_00 = rtc_data.energy_00;
	if (ENERGY_07 < rtc_data.energy_07) ENERGY_07 = rtc_data.energy_07;
	if (ENERGY_23 < rtc_data.energy_23) ENERGY_23 = rtc_data.energy_23;
	if (ENERGY_00_Y < rtc_data.energy_00_prev) ENERGY_00_Y = rtc_data.energy_00_prev;
	if (ENERGY_07_Y < rtc_data.energy_07_prev) ENERGY_07_Y = rtc_data.energy_07_prev;
	if (ENERGY_23_Y < rtc_data.energy_23_prev) ENERGY_23_Y = rtc_data.energy_23_prev;

	// запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	os_timer_disarm(&system_start_timer);
	os_timer_setfn(&system_start_timer, (os_timer_func_t *)system_start_cb, NULL);
	os_timer_arm(&system_start_timer, DELAYED_START * 1000, 0);
}

void ICACHE_FLASH_ATTR reset_energy_rate(int32_t value){
	ENERGY_00 = value;
	ENERGY_07 = value;
	ENERGY_23 = value;
	ENERGY_00_Y = value;
	ENERGY_07_Y = value;
	ENERGY_23_Y = value;
	ENERGY_YESTERDAY = 0;
	ENERGY_TODAY = 0;
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	if ( safemode ) return;

	//if(timersrc%5==0){
		get_config();
	//}
	
	if ( delayed_counter > 0 ) { 
		delayed_counter--;	
		return; // не будем далее выполнять код, пока счетчик отложенного запуска не будет 0
	}	

	// проверяем на зависание чтения (последнее время чтение данных зависает примерно через 4 дня и данные не изменяются)
	// будем проверять по напряжению, если напряжение было одинаково 100 сек подряд, значит могло зависнуть
	if ( voltage != voltage_prev ) {
		// все хорошо, запомним предыдущее показание
		voltage_prev = voltage;
		error_count = 0;
	} else {
		// показания одинаковы, увеличиваем ошибку
		error_count++;

		/*
			если кол-во превысило лимит и время не равно
			0:00:00-59 или
			0:07:00-59 или
			0:23:00-59 
			отправляем по mqtt
			ребутим
		*/
	}

	// вычисляем расход
	if ( energy < 1.0f ) return; // еще не получили показания счетчика

	int32_t _energy = (int32_t) (energy * 100);
	//if ( _energy < 1.0f ) _energy = rtc_data.energy_today;

	ENERGY_TODAY = _energy - ENERGY_00;
	if (  ENERGY_TODAY < 0 ) ENERGY_TODAY = 0;

	if ( time_loc.hour < 7)
	{
		ENERGY_07 = _energy; 
	} 

	if ( time_loc.hour < 23)
	{
		ENERGY_23 = _energy; 
	}

	if ( time_loc.hour == 23 && time_loc.min == 59 && time_loc.sec == 59 )
	{
		// запомним предыдущие значения
		ENERGY_00_Y = ENERGY_00;
		ENERGY_07_Y = ENERGY_07;
		ENERGY_23_Y = ENERGY_23;
		ENERGY_YESTERDAY = ENERGY_TODAY;

		// обнулить суточные данные
		ENERGY_00 = _energy;
		ENERGY_07 = ENERGY_00;
		ENERGY_23 = ENERGY_00;
		ENERGY_TODAY = 0;
		
		rtc_data.voltage_min.value = 0;
		rtc_data.voltage_min.dt = 0;		
		rtc_data.voltage_max.value = 0;
		rtc_data.voltage_max.dt = 0;		
		rtc_data.current_max.value = 0;
		rtc_data.current_max.dt = 0;

	} 

	if ( GPIO_ALL_GET( 6 ) == 1 )
	{
		reset_energy_rate(_energy);
		GPIO_ALL(6, 0);
		SAVEOPT;
	}

	// пишем данные в rtc
	rtc_data.energy_today = ENERGY_TODAY;
	rtc_data.energy_yesterday = ENERGY_YESTERDAY;
	rtc_data.energy_00 = ENERGY_00;
	rtc_data.energy_07 = ENERGY_07;
	rtc_data.energy_23 = ENERGY_23;
	rtc_data.energy_00_prev = ENERGY_00_Y;
	rtc_data.energy_07_prev = ENERGY_07_Y;
	rtc_data.energy_23_prev = ENERGY_23_Y;

	rtc_data.voltage_min.value = volt_min.value;
	rtc_data.voltage_min.dt = volt_min.dt;		
	rtc_data.voltage_max.value = volt_max.value;
	rtc_data.voltage_max.dt = volt_max.dt;		
	rtc_data.current_max.value = curr_max.value;
	rtc_data.current_max.dt = curr_max.dt;
	saveToRTC();

	if(timersrc%3600==0){
		// выполнение кода каждый час
		SAVEOPT;
	}

}

void ICACHE_FLASH_ATTR system_start_cb( ){

	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, sdm_task_delay, 0); // будет рестартовать сам себя

	os_timer_disarm(&overload_detect_timer);
	os_timer_setfn(&overload_detect_timer, (os_timer_func_t *)overload_detect_cb, NULL);
	os_timer_arm(&overload_detect_timer, overload_detect_delay, 1);


	//mqtt_client = (MQTT_Client*) &mqttClient;
	os_timer_disarm(&mqtt_send_timer);
	os_timer_setfn(&mqtt_send_timer, (os_timer_func_t *)mqtt_send_cb, NULL);
	os_timer_arm(&mqtt_send_timer, mqtt_send_interval_sec * 1000, 1);

	command = SDM_NO_COMMAND;
}

void ICACHE_FLASH_ATTR sdm_send (uint8_t addr, uint8_t fcode, uint32_t reg) {
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
	os_delay_us(20);
}

float sdm_read(uint8_t addr, uint8_t *buffer, uint8_t cnt) {
	uint8_t fcode = 4;
	uint8_t i;
	uint8_t data[RESPONSE_DATA_SIZE];
	float value = 0;
	if ( cnt ==  RESPONSE_SIZE && buffer[0] == addr && buffer[1] == fcode && buffer[2] ==  RESPONSE_DATA_SIZE ) {
		uint16_t crc = sdm_crc(buffer, RESPONSE_SIZE - 2 );
		if ( buffer[6] | (buffer[7] << 8) == crc) {
          ((uint8_t*)&value)[2]= buffer[4];
          ((uint8_t*)&value)[3]= buffer[3];
          ((uint8_t*)&value)[1]= buffer[5];
          ((uint8_t*)&value)[0]= buffer[6];
		}		
	}
	return value;
}

uint16_t sdm_crc(uint8_t *data, uint8_t sz) {
	uint16_t _crc, _flag;
	_crc = 0xFFFF;
	uint8_t i,j;
	for (i = 0; i < sz; i++) {
    	_crc = _crc ^ data[i];
    	for (j = 8; j; j--) {
    		_flag = _crc & 0x0001;
    		_crc >>= 1;
    		if (_flag)
        	_crc ^= 0xA001;
    	}
  	}
  	return _crc;	
}

void ICACHE_FLASH_ATTR request_voltage(uint8_t addr) {
	sdm_send(addr, 4, SDM_VOLTAGE);
}

void ICACHE_FLASH_ATTR request_current(uint8_t addr) {
	sdm_send(addr, 4, SDM_CURRENT);
}

void ICACHE_FLASH_ATTR request_power(uint8_t addr) {
	sdm_send(addr, 4, SDM_POWER);
}

void ICACHE_FLASH_ATTR request_energy(uint8_t addr) {
	sdm_send(addr, 4, SDM_ENERGY);
}

void ICACHE_FLASH_ATTR request_energy_resettable(uint8_t addr) {
	sdm_send(addr, 4, SDM_ENERGY_RESETTABLE);
}

void ICACHE_FLASH_ATTR read_electro_params_c3_v1_c3_p1__e120(uint8_t counter) {
		int m = counter % 4;
		if ( counter == 119 ) {
			command = SDM_ENERGY;
			request_energy(SDM_ADDR);
		} else if ( counter == 120) {
			command = SDM_ENERGY_RESETTABLE;
			request_energy_resettable(SDM_ADDR);
		} else if ( m != 0) {
			command = SDM_CURRENT;
			request_current(SDM_ADDR);
		} else {
			int d = counter / 4;
			int mm = d % 2;
			if ( mm == 1 ) {
				command = SDM_VOLTAGE;
				request_voltage(SDM_ADDR);
			} else {
				command = SDM_POWER;
				request_power(SDM_ADDR);
			}				
		}
}

void ICACHE_FLASH_ATTR read_electro_params_c20vp__er_10sec(uint8_t counter) {
	// c20vp - ток 20 раз подряд, 1 раз, 1 раз
	// er_60sec - расход 1 раз в 10 сек
	// 20 * 50 msec = 1000 msec, => 20 раз/сек
	// 20 * 100 msec = 2000 msec, => 10 раз/сек
	static uint32_t ts = 0;
	static uint32_t ts2 = 0;

	if ( millis() - ts> 10 * 1000 ) {  // раз в 10 сек
		command = SDM_ENERGY;
		request_energy(SDM_ADDR);
		ts = millis();
	} else if ( millis() - ts2> 11 * 1000 ) { // раз в 11 сек
		command = SDM_ENERGY_RESETTABLE;
		request_energy_resettable(SDM_ADDR);
		ts2 = millis();
	} else if ( counter < 21 ) {
		command = SDM_CURRENT;
		request_current(SDM_ADDR);
	} else if ( counter == 21 ) {
		command = SDM_VOLTAGE;
		request_voltage(SDM_ADDR);		
	} else if ( counter == 22) {
		command = SDM_POWER;
		request_power(SDM_ADDR);		
	}
}

void ICACHE_FLASH_ATTR read_electro_cb()
{
	static uint8_t el_cnt = 0;
	if ( command == SDM_NO_COMMAND) {	
		// можно писать в uart
		el_cnt++;  // увеличим счетчик

		system_soft_wdt_feed();
		#ifdef ELECTRO_C20_V1_P1__E10
			if ( el_cnt > 22 ) el_cnt = 1;		
			read_electro_params_c20vp__er_10sec(el_cnt);
		#else
			if ( el_cnt > 120 ) el_cnt = 1;
			read_electro_params_c3_v1_c3_p1__e120(el_cnt);
		#endif
		os_delay_us(500);	
	}
	os_timer_disarm(&read_electro_timer);
	os_timer_setfn(&read_electro_timer, (os_timer_func_t *)read_electro_cb, NULL);
	os_timer_arm(&read_electro_timer, sdm_task_delay, 0);		
}


void ICACHE_FLASH_ATTR mqtt_send_cb() {
	mqttSendFloat(VOLTAGE_TOPIC, voltage, 10);
	mqttSendFloat(CURRENT_TOPIC, current, 10);
	mqttSendFloat(POWER_TOPIC, power, 0);
	mqttSendFloat(ENERGY_TOPIC, energy, 100);
	mqttSend(OVERLOAD_TOPIC, overload);	
	
	mqttSendFloat(ENERGY_TODAY_TOPIC, (float)ENERGY_TODAY / 100.0f, 100);	
	mqttSendFloat(ENERGY_YESTERDAY_TOPIC, (float)ENERGY_YESTERDAY / 100.0f, 100);	
}	


void ICACHE_FLASH_ATTR overload_detect_cb()
{
	//  отрабатывает каждые 50 мсек
		
	// превышение по току, если в настройках отсечка по току более 10А
	if ( current_overload_treshold > 100 && 
	     current >= ((float)current_overload_treshold / 10.0f)) 
	{
		overload = 1;
	}
		
	// или превышение по мощности, если в настройках отсечка по мощности более 1000 Вт
	if ( power_overload_treshold > 1000 && 
	     power >= power_overload_treshold) 
	{
		overload = 2;
	}

	if ( overload > 0) {
		// есть перегрузка
		GPIO_ALL( RESET_LOAD_GPIO, GPIO_ON);
		// запустим единичный overload_reset_timer, который обнулит флаг перегрузки и выставит gpio2 в 0
		os_timer_disarm(&overload_reset_timer);
		os_timer_setfn(&overload_reset_timer, (os_timer_func_t *)overload_reset_cb, NULL);
		os_timer_arm(&overload_reset_timer, overload_time * 1000, 0);
	}
	// overload_reset_timer сбросит флаг перегрузки через Х сек после сработки перегрузки
	// если перегрузка не прекратилась, таймер перезапускается, т.е. в итоге флаг сбросится таймером только после Х сек после последнего детекта перегрузки
}

void overload_reset_cb()
{
	overload = 0;
	GPIO_ALL( RESET_LOAD_GPIO, GPIO_OFF);
}

void webfunc(char *pbuf) {

	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", delayed_counter);
	}
	
	os_sprintf(HTTPBUFF, 
		"<table width='100%%' cellpadding='2' cellspacing='2' cols='2'>"
			"<tr>"
				"<td align='left'>Отсечка по току: <b>%d.%d</b> A</td>"
				"<td align='right'>Перегрузка: <span style='color: %s;'><b>%s</b></span></td>"
			"</tr>"
			"<tr>"
				"<td align='left'>Отсечка по мощности: <b>%d.%03d</b> кВт</td>"
				"<td align='right'>GPIO2: <span style='color: %s;'><b>%s</b></span></td>"
			"</tr>"
		"</table>"
		, (uint16_t)current_overload_treshold / 10
		, (uint16_t)current_overload_treshold % 10
		, overload > 0 ? "red" : "green"
		, overload > 0 ? "ДА" : "НЕТ"
		, (uint16_t)power_overload_treshold / 1000
		, (uint16_t)(power_overload_treshold % 1000) / 100		
		, GPIO_ALL_GET( RESET_LOAD_GPIO ) > 0 ? "red" : "blue"
		, GPIO_ALL_GET( RESET_LOAD_GPIO ) > 0 ? "ВКЛ" : "ВЫКЛ" 		
	);


	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
							"<tr>"
							"<td>Напряжение: <b>%d</b> <small>В</small></td>"
							"<td>Сила тока: <b>%d.%02d</b> <small>А</small></td>"
							"<td>Мощность: <b>%d</b> <small>Вт</small></td>"
							"</tr>"
						  "</table>"
						  , (int)voltage
						  , (uint16_t)current
						  , (uint16_t)(current*100) % 100
						  , (uint16_t)power
				);
	
	os_sprintf(HTTPBUFF, 
		"<table width='100%%' cellpadding='2' cellspacing='2' cols='2'>"
			"<tr>"
				"<td align='left'>Напряжение (min): <b>%d</b> В</td>"
				"<td align='right'>Время: <b>%02d:%02d</b></td>"
			"</tr>"
			"<tr>"
				"<td align='left'>Напряжение (max): <b>%d</b> В</td>"
				"<td align='right'>Время: <b>%02d:%02d</b></td>"
			"</tr>"
			"<tr>"
				"<td align='left'>Сила тока (max): <b>%d.%d</b> A</td>"
				"<td align='right'>Время: <b>%02d:%02d</b></td>"
			"</tr>"
			"</table>"
			, (int)rtc_data.voltage_min.value
			, rtc_data.voltage_min.dt / 3600
			, (rtc_data.voltage_min.dt % 3600) / 60
			, (int)rtc_data.voltage_max.value
			, rtc_data.voltage_max.dt / 3600
			, (rtc_data.voltage_max.dt % 3600) / 60
			, (uint16_t)rtc_data.current_max.value
			, (uint16_t)(rtc_data.current_max.value*100) % 100
			, rtc_data.current_max.dt / 3600
			, (rtc_data.current_max.dt % 3600) / 60
	);
	
	//=====================================
	os_sprintf(HTTPBUFF, "<style>"
						 ".tn {float: left; width: 70px; text-align: right; margin-right: 10px;}"
						 "</style>");

	os_sprintf(HTTPBUFF, "<div>");
	os_sprintf(HTTPBUFF, "<div><b>Расход</b></div>");
	os_sprintf(HTTPBUFF, "<div><b class='tn'>Всего:</b> %d.%02d кВт*ч</div>"
						, (uint32_t)energy
						, (uint32_t)(energy*100) % 100
	);

	os_sprintf(HTTPBUFF, "<div><b class='tn'>вчера:</b> %d.%02d кВт*ч</div>"
						, (int32_t)(ENERGY_YESTERDAY / 100)
						, (int32_t)(ENERGY_YESTERDAY % 100)
	);
	os_sprintf(HTTPBUFF, "<div><b class='tn'>сегодня:</b> %d.%02d кВт*ч</div>"
						, (int32_t)(ENERGY_TODAY / 100)
						, (int32_t)(ENERGY_TODAY % 100)
	);
	os_sprintf(HTTPBUFF, "<div><b class='tn'>день:</b> %d.%02d кВт*ч</div>"
					   , (int32_t)( (ENERGY_23 - ENERGY_07) / 100)
					   , (int32_t)( (ENERGY_23 - ENERGY_07) % 100)
	);
	os_sprintf(HTTPBUFF, "<div><b class='tn'>ночь:</b> %d.%02d кВт*ч</div>"
					   , (int32_t)( (ENERGY_07 - ENERGY_23_Y) / 100)
					   , (int32_t)( (ENERGY_07 - ENERGY_23_Y) % 100)
	);
	os_sprintf(HTTPBUFF, "</div>");

	//=========================================
	os_sprintf(HTTPBUFF, "<p><small>Ошибки чтения: %d</small></p>", error_count);

	os_sprintf(HTTPBUFF,"<p><small>Версия прошивки: %s</small></p>", FW_VER); 
	os_sprintf(HTTPBUFF,"<p style='color: red;'><small><b>Прежде чем менять данные, перезагрузи модуль!!!</b></small></p>"); 

 // когда перегрузка закончилась, показать таймер отчета до включения устройства
}