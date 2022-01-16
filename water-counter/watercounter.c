/*
версия 2.хх - добавляю работу с eeprom (24c64), убрал rtc mem

// Количество настроек: Счетчик1 GPIO,Расход1 общий,Расход1 сегодня,Расход1 вчера,Счетчик2 GPIO,Расход2 общий,Расход2 сегодня,Расход2 вчера,Последняя промывка,Счетчик1-промывка,Счетчик2-промывка-до,Счетчик2-промывка-после,Объем до промывки,Дней до промывки,Автоотключение промывки (мин),Сбросить все

*/
os_timer_t gpio_timer;

#define millis() (unsigned long) (micros() / 1000ULL)
#define B(bit_no)         (1 << (bit_no))
#define BIT_CLEAR(reg, bit_no)   (reg) &= ~B(bit_no)
#define BIT_SET(reg, bit_no)   (reg) |= B(bit_no)
#define BIT_CHECK(reg, bit_no)   ( ((reg) & B(bit_no)) > 0 )
#define BIT_TRIGGER(reg, bit_no)   (reg) ^= B(bit_no)

#define PCF_LED_ON(num) (BIT_CLEAR(pcf_data, num))
#define PCF_LED_OFF(num) (BIT_SET(pcf_data, num))

#define GET_TS()( sntp_get_current_timestamp() < TIMESTAMP_DEFAULT ? 0 : (sntp_get_current_timestamp() - sntp_get_timezone()*3600 + sensors_param.utc * 3600))

#define TIMESTAMP_DEFAULT 1614081600
#define MAX_COUNTER_VALUE 9999999

#define GPIO_STATE_CLOSE 0
#define COUNTER_IMPL 10
#define AUTO_WASH_START_DELTA 	30			// порог расхода счетчика2 при выключенной промывки для автоопределения начала промывки
#define AUTO_WASH_END_DELTA 	20			// порог расхода счетчика1 при включенной промывки для автоопределения зазвершения промывки
#define CLEAN_WATER_PERCENT 70		// порог в %%, после которого сигнализируется красным светодиодом о приближении времени промывки
#define CLEAN_WATER_VOLUME_DEFAULT 	10000 	// 10 кубов, объем чистой воды до следующей промывки		// можно выставлять через интерпретер

#define STATE_NORMA 0
#define STATE_WASH  1

#define WASH_FERRUM_FREE 0
#define	WASH_SOFTENER 	 1

#define WRITE_BIT   0
#define READ_BIT    1

#define LED_GREEN 0
#define LED_RED 1

#define BUTTON1	4
#define BUTTON2	5

uint32_t ts1 = 0;
uint32_t ts2 = 0;

static uint32_t pt1 = 0;
static uint8_t press_flag = 0;

static uint32_t pt2 = 0;
static uint8_t press_flag2 = 0;


static uint32_t btn2_pressed = 0;
	
#define TOPIC_WASH_STATE 	"washstate"
#define TOPIC_WASH_START 	"washstart"
#define TOPIC_WASH_END 		"washend"
#define TOPIC_WASH_LITRES 	"washlitres"

#define EEPROM_ADDR							0x50

#define EEPROM_MAGIC_ADDR					16 	
#define EEPROM_WATERCNT1_ADDR				20	
#define EEPROM_WATERCNT1_Y_ADDR				24	
#define EEPROM_WATERCNT1_T_ADDR				28	
#define EEPROM_WATERCNT2_ADDR				32	
#define EEPROM_WATERCNT2_Y_ADDR				36	
#define EEPROM_WATERCNT2_T_ADDR				40	
#define EEPROM_WASH_START_DT_ADDR			44			// дата и время начала промывки
#define EEPROM_WASH_END_DT_ADDR				48			// дата и время завершения промывки
#define EEPROM_WATERCNT2_CHANGE_TS_ADDR		52			// дата и время изменения показаний счетчика 2
#define EEPROM_WATERCNT1_WASH_START_ADDR	56			// показания счетчика 1 при начале промывки
#define EEPROM_WATERCNT2_WASH_START_ADDR	60			// показания счетчика 2 при начале промывки
#define EEPROM_WATERCNT2_WASH_END_ADDR		64			// показания счетчика 2 при завершении промывки
#define EEPROM_WATERCNT2_WASH_SWITCH_ADDR	68			// показания счетчика 2 при переключении на промывку умягчителя
#define EEPROM_WASH_COUNT_ADDR				72			// кол-во совершенных промывок
#define EEPROM_WASH_DURATION_ADDR			76			// длительность последней промывки
#define EEPROM_WASH_STATE_ADDR				80			// текущее состояние
#define EEPROM_WASH_TYPE_ADDR				84			// тип промывки
#define EEPROM_CLEAN_WATER_ADDR				88			// расход чистой воды после промывки
#define EEPROM_WASH_VOLUME_ADDR				92			// уставка объема воды до промывки

#define  WATERCNT1_GPIO 		sensors_param.cfgdes[0]		// счетчик 1 gpio
#define  WATERCNT1 				sensors_param.cfgdes[1]		// счетчик 1 общий расход
#define  WATERCNT1_T 			sensors_param.cfgdes[2]		// счетчик 1 расход сегодня
#define  WATERCNT1_Y 			sensors_param.cfgdes[3]		// счетчик 1 расход вчера
uint32_t watercnt1_change_ts = TIMESTAMP_DEFAULT;		// счетчик 1 таймстамп последнего изменения показаний

#define  WATERCNT2_GPIO 		sensors_param.cfgdes[4]		// счетчик 2 gpio
#define  WATERCNT2 				sensors_param.cfgdes[5]		// счетчик 2 общий расход
#define  WATERCNT2_T 			sensors_param.cfgdes[6]		// счетчик 2 расход сегодня
#define  WATERCNT2_Y 			sensors_param.cfgdes[7]		// счетчик 2 расход вчера
uint32_t watercnt2_change_ts = TIMESTAMP_DEFAULT;		// счетчик 2 таймстамп последнего изменения показаний

#define  WASH_START_TS 			sensors_param.cfgdes[8]		// таймстамп начала промывки
uint32_t wash_end_ts = TIMESTAMP_DEFAULT;				// таймстамп окончания промывки

#define  WASH_CNT1_START 		sensors_param.cfgdes[9]		// показания счетчика 1 на начало промывки
uint32_t wash_cnt1_end = 0;								// показания счетчика 1 на окончание промывки
uint32_t wash_cnt1_switch = 0;							// показания счетчика 1 при переключении на промывку умягчителя	

#define  WASH_CNT2_START 		sensors_param.cfgdes[10]	// показания счетчика 2 на начало промывки
#define  WASH_CNT2_END 			sensors_param.cfgdes[11]	// показания счетчика 2 на окончание промывки
uint32_t wash_cnt2_switch = 0;							// показания счетчика 2 при переключении на промывку умягчителя	

#define  CLEAN_WATER_VOLUME 	sensors_param.cfgdes[12]	//10000 	// 10 кубов, объем чистой воды до следующей промывки		// можно выставлять через интерпретер

#define  WASH_AFTER_DAYS 		sensors_param.cfgdes[13]	//14  	// новая промывка через Х дней после прошедшей
#define  WASH_AUTO_END 			sensors_param.cfgdes[14]	//30  	// автовыключение промывки
#define  RESET_ALL 				sensors_param.cfgdes[15]	//флаг сброса



uint32_t clean_water;		// объем чистой воды после последней промывки (литры)
uint16_t percent;



uint32_t wash_state = STATE_NORMA;
uint32_t wash_type = WASH_FERRUM_FREE;

uint32_t wash_count = 0;			// кол-во промывок
uint32_t wash_duration = 0;

uint8_t reset = 0;

#define PASSED_DAY_AFTER_WASH() ( (GET_TS()  - wash_end_ts ) / (3600*24) )

#define		RTC_MAGIC		0x55aaaa55

// #define ADDLISTSENS {200,LSENSFL3|LSENS32BIT,"WaterCnt1","watercnt1",&WATERCNT1,NULL}, \
// 					{201,LSENSFL3|LSENS32BIT,"WaterCnt1Y","watercnt1y",&WATERCNT1_Y,NULL}, \
// 					{202,LSENSFL3|LSENS32BIT,"WaterCnt1T","watercnt1t",&WATERCNT1_T,NULL}, \
// 					{203,LSENSFL3|LSENS32BIT,"WaterCnt2","watercnt2",&WATERCNT2,NULL}, \
// 					{204,LSENSFL3|LSENS32BIT,"WaterCnt2Y","watercnt2y",&WATERCNT2_Y,NULL}, \
// 					{205,LSENSFL3|LSENS32BIT,"WaterCnt2T","watercnt2t",&WATERCNT2_T,NULL}, \
// 					{206,LSENSFL0|LSENS32BIT,"WashState","washstate",&wash_state,NULL}, \
// 					{207,LSENSFL0|LSENS32BIT,"WashTime","washtime",&wash_duration,NULL}, \
// 					{208,LSENSFL0|LSENS32BIT,"WashCnt","washcnt",&wash_count,NULL}, \
// 					{209,LSENSFL0|LSENS32BIT,"WashStart","washstart",&wash_start_dt,NULL}, \
// 					{210,LSENSFL0|LSENS32BIT,"WashEnd","washend",&WASH_START_TS,NULL}, \
// 					{211,LSENSFL0,"WashResrc","washrsrc",&percent,NULL}, 


#define ADDLISTSENS {200,LSENSFL3|LSENS32BIT,"WaterCnt1","watercnt1",&WATERCNT1,NULL}, \
					{201,LSENSFL3|LSENS32BIT,"WaterCnt1Y","watercnt1y",&WATERCNT1_Y,NULL}, \
					{202,LSENSFL3|LSENS32BIT,"WaterCnt1T","watercnt1t",&WATERCNT1_T,NULL}, \
					{203,LSENSFL3|LSENS32BIT,"WaterCnt2","watercnt2",&WATERCNT2,NULL}, \
					{204,LSENSFL3|LSENS32BIT,"WaterCnt2Y","watercnt2y",&WATERCNT2_Y,NULL}, \
					{205,LSENSFL3|LSENS32BIT,"WaterCnt2T","watercnt2t",&WATERCNT2_T,NULL}, \
					{206,LSENSFL0|LSENS32BIT,"WashState","washstate",&wash_state,NULL}, \
					{207,LSENSFL0|LSENS32BIT,"WashTime","washtime",&wash_duration,NULL}, \
					{208,LSENSFL0|LSENS32BIT,"WashCnt","washcnt",&wash_count,NULL}, \
					{209,LSENSFL0|LSENS32BIT,"WashStart","washstart",&WASH_START_TS,NULL}, \
					{210,LSENSFL0|LSENS32BIT,"WashEnd","washend",&wash_end_ts,NULL}, \
					{211,LSENSFL0,"WashResrc","washrsrc",&percent,NULL}, 

uint8_t pcf_data;
	
void ICACHE_FLASH_ATTR reset_all();

void ICACHE_FLASH_ATTR blink_one_led(uint8_t led, uint16_t before, uint16_t on_delay, uint16_t off_delay)
{
	// частота запуска 100 мсек
	static uint32_t blink = 0;
	uint16_t cnt = (before + on_delay + off_delay) / 100;
	
	if ( (blink % cnt >= (before / 100)) && (blink % cnt < ( (before + on_delay) / 100)))
		PCF_LED_ON(led);
	else
		PCF_LED_OFF(led);

	blink++;
	if ( blink > (cnt-1) ) blink = 0;
}

void ICACHE_FLASH_ATTR blink_one_led_twice(uint8_t led, uint16_t before, uint16_t on1_delay, uint16_t off1_delay, uint16_t on2_delay, uint16_t off2_delay )
{
	// частота запуска 100 мсек
	static uint32_t blink = 0;
	uint16_t cnt = (before + on1_delay + off1_delay + on2_delay + off2_delay) / 100;
	
	if ( ( blink % cnt >= (before / 100) && blink % cnt < ( ( before + on1_delay ) / 100) )// первая вспышка
		 || 								
	     ( blink % cnt >= ( (before + on1_delay + off1_delay ) / 100) &&  blink % cnt < ( (before + on1_delay + off1_delay + on2_delay) / 100) )     // вторая вспышка
		)
		PCF_LED_ON(led);
	else
		PCF_LED_OFF(led);

	blink++;
	if ( blink > (cnt-1) ) blink = 0;
}

void ICACHE_FLASH_ATTR mqtt_send_wash_start()
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	os_sprintf(payload,"%d", wash_state);
	MQTT_Publish(&mqttClient, TOPIC_WASH_STATE, payload, os_strlen(payload), 0, 0, 0);
	
	os_sprintf(payload,"%d", WASH_START_TS);
	MQTT_Publish(&mqttClient, TOPIC_WASH_START, payload, os_strlen(payload), 0, 0, 0);
		
	#endif	
}

void ICACHE_FLASH_ATTR mqtt_send_wash_end()
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	
	os_sprintf(payload,"%d", (WASH_CNT2_END > WASH_CNT2_START /*&& wash_cnt2_switch >= WASH_CNT2_START*/) ? (WASH_CNT2_END - WASH_CNT2_START) : 0);
	MQTT_Publish(&mqttClient, TOPIC_WASH_LITRES, payload, os_strlen(payload), 0, 0, 0);
			
	os_sprintf(payload,"%d", WASH_START_TS);
	MQTT_Publish(&mqttClient, TOPIC_WASH_END, payload, os_strlen(payload), 0, 0, 0);
	
	os_sprintf(payload,"%d", wash_state);
	MQTT_Publish(&mqttClient, TOPIC_WASH_STATE, payload, os_strlen(payload), 0, 0, 0);	
	
	#endif		
}

uint32_t ICACHE_FLASH_ATTR read_eeprom(uint16_t addr)
{
	uint32_t val;
	read_24cxx(EEPROM_ADDR, addr, (uint8_t*)&val, 4);
	if ( val == 0xFFFFFFFF ) val = 0;
	return val;
}

void ICACHE_FLASH_ATTR write_eeprom(uint16_t addr, uint32_t val)
{
	uint32_t v = read_eeprom(addr);
	if ( v != val)	
	{
		write_24cxx(EEPROM_ADDR, addr, (uint8_t*)&val, 4);
		os_delay_us(10);
	}
}


void ICACHE_FLASH_ATTR save_eeprom()
{
	write_eeprom(EEPROM_WASH_STATE_ADDR, 		wash_state);
	write_eeprom(EEPROM_WASH_TYPE_ADDR,			wash_type);
	
	write_eeprom(EEPROM_WATERCNT1_ADDR, 		WATERCNT1);
	write_eeprom(EEPROM_WATERCNT1_Y_ADDR, 		WATERCNT1_Y);
	write_eeprom(EEPROM_WATERCNT1_T_ADDR, 		WATERCNT1_T);
	
	write_eeprom(EEPROM_WATERCNT2_ADDR, 		WATERCNT2);
	write_eeprom(EEPROM_WATERCNT2_Y_ADDR, 		WATERCNT2_Y);
	write_eeprom(EEPROM_WATERCNT2_T_ADDR, 		WATERCNT2_T);
	
	write_eeprom(EEPROM_WASH_START_DT_ADDR, 	WASH_START_TS);
	write_eeprom(EEPROM_WASH_END_DT_ADDR, 		wash_end_ts);

	write_eeprom(EEPROM_WATERCNT2_CHANGE_TS_ADDR, watercnt2_change_ts);

	write_eeprom(EEPROM_WATERCNT1_WASH_START_ADDR, WASH_CNT1_START);
	write_eeprom(EEPROM_WATERCNT2_WASH_START_ADDR, WASH_CNT2_START);
	write_eeprom(EEPROM_WATERCNT2_WASH_END_ADDR, WASH_CNT2_END);
	write_eeprom(EEPROM_WATERCNT2_WASH_SWITCH_ADDR, wash_cnt2_switch);
	
	write_eeprom(EEPROM_WASH_COUNT_ADDR, wash_count);
	write_eeprom(EEPROM_WASH_DURATION_ADDR, wash_duration);
	
	write_eeprom(EEPROM_CLEAN_WATER_ADDR, clean_water);
	write_eeprom(EEPROM_WASH_VOLUME_ADDR, CLEAN_WATER_VOLUME);
}


void ICACHE_FLASH_ATTR save_options()
{
	save_eeprom();
	SAVEOPT;
}

void ICACHE_FLASH_ATTR do_wash_start(uint16_t counter_offset)
{
	if ( wash_state == STATE_WASH ) return;
	
	wash_state = STATE_WASH;
	wash_type = WASH_FERRUM_FREE;

	WASH_START_TS = GET_TS();

	wash_cnt2_switch = WASH_CNT2_START;
	WASH_CNT1_START = WATERCNT1;		// фиксируем показания счетчика 1 на начало промывки
	WASH_CNT2_START = WATERCNT2 -  counter_offset;		// фиксируем показания счетчика 2 на начало промывки
	
	watercnt2_change_ts = GET_TS();

	wash_duration = 0;
	wash_count++;
	
	save_options();	
	mqtt_send_wash_start();
}

void ICACHE_FLASH_ATTR do_wash_end(uint16_t counter_offset)
{
	if ( wash_state == STATE_NORMA ) return;
	
	wash_state = STATE_NORMA;
	wash_type = WASH_FERRUM_FREE;
	
	WASH_START_TS = TIMESTAMP_DEFAULT;
	wash_end_ts = GET_TS();			// фиксируем дату и время завершения промывки
	
	//WASH_CNT2_START = WATERCNT2;
	WASH_CNT2_END = WATERCNT2;		// фиксируем показания счетчика 2 на окончание промывки

	save_options();	
	mqtt_send_wash_end();
}

void ICACHE_FLASH_ATTR read_gpio_cb() 
{
	// счетик 1
	if ( WATERCNT1_GPIO < 16 && digitalRead(WATERCNT1_GPIO) == GPIO_STATE_CLOSE )
	{
		// геркон замкнут (замыкание начинается на значении бликом к 1, а размыкание на значении после 3)
		// Сами счетчик замыкаются при переходе с 0 на 9 и размыкание происходит при переходе с 2 на 3.
		if ( ts1 == 0 )
		{
			WATERCNT1 += COUNTER_IMPL;
			WATERCNT1_T += COUNTER_IMPL;
			watercnt1_change_ts = GET_TS();
		}
		
		ts1 = millis();
		
	} 
	else 
	{
		ts1 = 0;
	}
	
	// счетик 2
	if ( WATERCNT2_GPIO < 16 &&  digitalRead(WATERCNT2_GPIO) == GPIO_STATE_CLOSE )
	{
		// геркон замкнут (замыкание начинается на значении бликом к 1, а размыкание на значении после 3)
		if ( ts2 == 0 )
		{
			WATERCNT2 += COUNTER_IMPL;
			WATERCNT2_T += COUNTER_IMPL;
			watercnt2_change_ts = GET_TS();
		}
		
		ts2 = millis();
		
	} 
	else 
	{
		ts2 = 0;
	}	
	
	pcf_data = pcfgpior8(0x20);
	
	// грязный хак: перед тем как прочитать состояние гпио как входа, надо этот пин пометить как вход (1) записью в pcf
	//if ( BIT_CHECK(data, 4) > 0)
	if ( GPIO_ALL_GET(224) == 0)
	{
		// нажали
		if ( pt1 == 0 ) 
			pt1 = millis(); // фиксируем время нажатия
	} 
	else 
	{
		// отпустили
		pt1 = 0;
		press_flag = 0;
	}
	
	// грязный хак: перед тем как прочитать состояние гпио как входа, надо этот пин пометить как вход (1) записью в pcf
	BIT_SET( pcf_data, 4); BIT_SET( pcf_data, 5);
	pcfgpiow8(0x20, pcf_data);
	
	if ( GPIO_ALL_GET(225) == 0)
	{
		// нажали
		if ( pt2 == 0 ) 
			pt2 = millis(); // фиксируем время нажатия
	} 
	else 
	{
		// отпустили
		pt2 = 0;
		press_flag2 = 0;
	}
	
	if ( press_flag == 0 && millis() - pt1 >= 5000 && pt1 > 0) {
		// кнопка нажата более 5 сек
		if ( wash_state == STATE_NORMA ) 
		{
			// включаем режим промывки
			do_wash_start(0);
			btn2_pressed = 1;
		}
		else
		{
			// отключаем режим промывки
			do_wash_end(0);
			btn2_pressed = 1;
		}	
		press_flag = 1;		
	} 


	if ( press_flag2 == 0 && millis() - pt2 >= 5000 && pt2 > 0) {
		// кнопка нажата более 5 сек
		if ( wash_state == STATE_NORMA ) 
		{
			// обычный режим, например, для сохранения во флеш при нажатии или для сброса показаний
			btn2_pressed = 1;
			save_options();

			reset++; 
			if ( reset > 2 ) reset = 0;

			if ( reset == 2 ) {
				reset = 0;
				reset_all();
			}

		} 
		else 
		{
			btn2_pressed = 1;
			// режим промывки
			if ( wash_type == WASH_FERRUM_FREE ) 
			{
				// включаем режим промывки умягчителя
				wash_type = WASH_SOFTENER;
				// зафиксировать показания счетчика 2 промежуточные (узнаем, сколько воды было потрачено на промывку обезжелезивателя
				wash_cnt2_switch = WATERCNT2;
			} 
			else 
			{
				// включаем режим промывки обезжелезивателя
				wash_type = WASH_FERRUM_FREE;
			}
		}
		press_flag2 = 1;	
	} 
	
	// индикации и другие действия по статусу промывки
	if ( wash_state == STATE_WASH )
	{
		PCF_LED_OFF(LED_RED);
		
		if ( btn2_pressed > 0 ) 
		{
			blink_one_led_twice(LED_RED, 0, 100, 200, 100, 200 );
			btn2_pressed += 100;
			if ( btn2_pressed > 700 ) btn2_pressed = 0;
			goto pcf;
		}
		
		// режим промывки включен, мигаем светодиодом,  сюда попадаем каждые 100 мсек
		blink_one_led(LED_GREEN, 0, 100, 100);
		
	} 
	else 
	{
		// режим промывки выключен
		PCF_LED_OFF(LED_RED);	//BIT_SET( pcf_data, LED_RED);  // погасить красный светодиод
		
		if ( btn2_pressed > 0 ) 
		{
			blink_one_led_twice(LED_RED, 0, 100, 200, 100, 200 );
			btn2_pressed += 100;
			if ( btn2_pressed > 700 ) btn2_pressed = 0;
			goto pcf;
		}
		
		// в зависимости от потребленного расхода чистой воды после промывки показываем индикацию
		// если расход после промывки Б 70%, то включаем зеленый или редко мигаем BIT_CLEAR(data, LED_GREEN);
		clean_water = ( WASH_CNT1_START > 0) ? WATERCNT1 - WASH_CNT1_START : 0;
		
		if ( clean_water >= CLEAN_WATER_VOLUME )
		{
			// превышение на 100%
			PCF_LED_OFF(LED_GREEN);
			blink_one_led(LED_RED, 0, 100, 100);			
		}
		else
		{
			if ( (clean_water*100)/CLEAN_WATER_VOLUME > CLEAN_WATER_PERCENT )
			{
				// превысили 70%
				PCF_LED_OFF(LED_GREEN);
				PCF_LED_OFF(LED_RED);			
				
				blink_one_led(LED_GREEN, 0, 100, 2900);
				blink_one_led_twice(LED_RED, 900, 100, 200, 100, 1700 );					
			} 
			else 
			{
				// расход чистой воды менее 70%
				// индикация: зеленая вспышка на 100 мсек, далее 1900 мсек погашено
				PCF_LED_OFF(LED_RED);			
				blink_one_led(LED_GREEN, 0, 100, 1900);
			}
		}
	}
	
	/* если кнопка 4 нажата, то включить led 0
	if ( BIT_CHECK(data, 4) > 0)
	{
		BIT_SET( data, 0);
	} else {
		BIT_CLEAR(data, 0);
	}

	if ( BIT_CHECK(data, 5) > 0)
	{
		BIT_SET( data, 1);
	} else {
		BIT_CLEAR(data, 1);
	}
	*/
pcf:
	BIT_SET( pcf_data, 4); BIT_SET( pcf_data, 5);
	pcfgpiow8(0x20, pcf_data);
}

void ICACHE_FLASH_ATTR check_values()
{
	if ( WATERCNT1 > MAX_COUNTER_VALUE ||  WATERCNT1 < 0) WATERCNT1 = 0;
	if ( WATERCNT1_Y > MAX_COUNTER_VALUE ||  WATERCNT1_Y < 0) WATERCNT1_Y = 0;
	if ( WATERCNT1_T > MAX_COUNTER_VALUE ||  WATERCNT1_T < 0) WATERCNT1_T = 0;

	if ( WATERCNT2 > MAX_COUNTER_VALUE ||  WATERCNT2 < 0) WATERCNT2 = 0;
	if ( WATERCNT2_Y > MAX_COUNTER_VALUE ||  WATERCNT2_Y < 0) WATERCNT2_Y = 0;
	if ( WATERCNT2_T > MAX_COUNTER_VALUE ||  WATERCNT2_T < 0) WATERCNT2_T = 0;

	if ( WASH_CNT1_START > MAX_COUNTER_VALUE ||  WASH_CNT1_START < 0) WASH_CNT1_START = 0;
	if ( wash_cnt1_switch > MAX_COUNTER_VALUE ||  wash_cnt1_switch < 0) wash_cnt1_switch = 0;		
	if ( wash_cnt1_end > MAX_COUNTER_VALUE ||  wash_cnt1_end < 0) wash_cnt1_end = 0;		

	if ( WASH_CNT2_START > MAX_COUNTER_VALUE ||  WASH_CNT2_START < 0) WASH_CNT2_START = 0;		
	if ( WASH_CNT2_END > MAX_COUNTER_VALUE ||  WASH_CNT2_END < 0) WASH_CNT2_END = 0;		
	if ( wash_cnt2_switch > MAX_COUNTER_VALUE ||  wash_cnt2_switch < 0) wash_cnt2_switch = 0;		
	
	if ( RESET_ALL > 1 ) RESET_ALL = 0;
	//TODO: if ( WASH_AUTO_END < 10 ) WASH_AUTO_END = 0;		
}

void ICACHE_FLASH_ATTR reset_all()
{
	wash_state = STATE_NORMA;
	wash_type = WASH_FERRUM_FREE;

	wash_count = 0;
	wash_duration = 0;

	WASH_START_TS = TIMESTAMP_DEFAULT;
	wash_end_ts = TIMESTAMP_DEFAULT;

	WASH_CNT1_START = WATERCNT1;
	WASH_CNT2_START = WATERCNT2;
	WASH_CNT2_END = WATERCNT2;
	wash_cnt2_switch = WATERCNT2;
	watercnt2_change_ts = GET_TS();
	clean_water = 0;
	percent = 0;

	RESET_ALL = 0;

	save_options();
	save_eeprom();
}

void ICACHE_FLASH_ATTR startfunc()
{

	//читаем данные из eeprom в переменные, если magic не корректный, то обнуляем переменные и записываем в eeprom
	uint32_t val32 = 0xFFFF;
	val32 = read_eeprom(EEPROM_MAGIC_ADDR);
	if ( val32 == RTC_MAGIC ) 
	{
		// eeprom что-то содержит из наших данных
		WATERCNT1 = read_eeprom(EEPROM_WATERCNT1_ADDR);
		WATERCNT1_Y = read_eeprom(EEPROM_WATERCNT1_Y_ADDR);
		WATERCNT1_T = read_eeprom(EEPROM_WATERCNT1_T_ADDR);
						
		WATERCNT2 = read_eeprom(EEPROM_WATERCNT2_ADDR);
		WATERCNT2_Y = read_eeprom(EEPROM_WATERCNT2_Y_ADDR);
		WATERCNT2_T = read_eeprom(EEPROM_WATERCNT2_T_ADDR);
		
		WASH_START_TS = read_eeprom(EEPROM_WASH_START_DT_ADDR);
		wash_end_ts = read_eeprom(EEPROM_WASH_END_DT_ADDR);
		
		watercnt2_change_ts = read_eeprom(EEPROM_WATERCNT2_CHANGE_TS_ADDR);
		
		WASH_CNT1_START = read_eeprom(EEPROM_WATERCNT1_WASH_START_ADDR);
		WASH_CNT2_START = read_eeprom(EEPROM_WATERCNT2_WASH_START_ADDR);
		WASH_CNT2_END = read_eeprom(EEPROM_WATERCNT2_WASH_END_ADDR);
		wash_cnt2_switch = read_eeprom(EEPROM_WATERCNT2_WASH_SWITCH_ADDR);
		
		wash_count = read_eeprom(EEPROM_WASH_COUNT_ADDR);
		wash_duration = read_eeprom(EEPROM_WASH_DURATION_ADDR);
		wash_state = read_eeprom(EEPROM_WASH_STATE_ADDR);		
		wash_type = read_eeprom(EEPROM_WASH_TYPE_ADDR);		
		
		clean_water = read_eeprom(EEPROM_CLEAN_WATER_ADDR);		
		CLEAN_WATER_VOLUME = read_eeprom(EEPROM_WASH_VOLUME_ADDR);	
			
	} 
	else 
	{
		uint32_t mg = RTC_MAGIC;
	
		write_eeprom(EEPROM_MAGIC_ADDR, RTC_MAGIC);		

		// eeprom пустой (нет наших данных), инициализируем нулями
		WATERCNT1 = 0; 
		WATERCNT1_Y = 0;
		WATERCNT1_T = 0;

		WATERCNT2 = 0;
		WATERCNT2_Y = 0;
		WATERCNT2_T = 0;
		
		WASH_START_TS = GET_TS();
		wash_end_ts = GET_TS();

		watercnt2_change_ts = GET_TS();
		
		WASH_CNT1_START = 0;
		WASH_CNT2_START = 0;
		WASH_CNT2_END = 0;
		wash_cnt2_switch = 0;
		
		wash_count = 0;
		wash_duration = 0;
		wash_state = 0;
		wash_type = 0;		
		
		clean_water = 0;	
		CLEAN_WATER_VOLUME = 0;

		save_eeprom();
		save_options();
	}
	
	check_values();

	pcf_data = 0b00110011;
	pcfgpiow8(0x20, pcf_data);
	
	os_timer_disarm(&gpio_timer);
	os_timer_setfn(&gpio_timer, (os_timer_func_t *)read_gpio_cb, NULL);
	os_timer_arm(&gpio_timer, 100, 1);

}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
	if ( RESET_ALL == 1 )
	{
		reset_all();
	}

	if ( CLEAN_WATER_VOLUME < 1000 ) CLEAN_WATER_VOLUME = CLEAN_WATER_VOLUME_DEFAULT;
	
	if (timersrc%1800==0)  //30*60 сек  каждые 30 мин сохраняем данные во флеш
	{
		//SAVEOPT;
		save_options();
	}

	if ( time_loc.hour == 0 && time_loc.min == 0 && time_loc.sec == 0 )
	{
		// обнулить суточные данные ночью
		WATERCNT1_Y = WATERCNT1_T;
		WATERCNT1_T = 0;

		WATERCNT2_Y = WATERCNT2_T;
		WATERCNT2_T = 0;		
	} 
	
	if ( wash_state == STATE_WASH )
	{
		wash_duration++;
	} 
	else 
	{
		percent = (clean_water*100)/CLEAN_WATER_VOLUME;
	}
	
	save_eeprom();

	// автоматическая фиксация начала промывки ( если по счетчику 2 начал увеличиваться расход, но факта начала промывки не зафиксировано)
	if ( 
		wash_state == STATE_NORMA 	// режима Норма, промывка не включена
		&& WATERCNT2 - WASH_CNT2_END > AUTO_WASH_START_DELTA // показания счетчика увеличились на 30 литров
		&& WASH_START_TS <= TIMESTAMP_DEFAULT
	   )
	{
		// автоматическое определение начала промывки
		do_wash_start(AUTO_WASH_START_DELTA);	
	}
	

	// автоматическая фиксация завершения промывки
	uint32_t ts = GET_TS();
	if ( 	
		wash_state == STATE_WASH 
		&& ts > TIMESTAMP_DEFAULT  
		&& WASH_START_TS > TIMESTAMP_DEFAULT
		&& WASH_START_TS > wash_end_ts						 // время начала больше времени предыдущего окончания
	    && (
			  (ts - watercnt2_change_ts) / 60  >= WASH_AUTO_END // время последнего изменения показаний счетчика 2 и если показания не изменялись более 30 мин, значит промывка завершилась
	          || WATERCNT1 - WASH_CNT1_START >= 20				 // показания счетчика 1 изменились на 20 литров (а при промывке у нас счетчик 1 не должен изменять показания)
		   )
	   )
	{
		// промывка длится более 5 часов, значит забыли вручную зафиксировать завершение промывки, зафиксируем автоматически
		// отключаем режим промывки
		do_wash_end(AUTO_WASH_END_DELTA);		 // TODO: срабатывает и сбрасывает state
	}		
}

void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
						"<tr><th>Счетчик 1 (чистая вода):</th></tr>"
			   			);
	os_sprintf(HTTPBUFF ,"<tr><td>текущие показания:</td><td> %d.%03d м³ (%d л)</td></tr>"
						, WATERCNT1 / 1000
						, WATERCNT1 % 1000
						, WATERCNT1
						);

	os_sprintf(HTTPBUFF ,"<tr><td>расход сегодня:</td><td> %d.%03d м³ (%d л)</td></tr>"
						, WATERCNT1_T / 1000
						, WATERCNT1_T % 1000
						, WATERCNT1_T
						);
	
	os_sprintf(HTTPBUFF	,"<tr><td>расход вчера:</td><td> %d.%03d м³ (%d л)</td></tr>"
						, WATERCNT1_Y / 1000
						, WATERCNT1_Y % 1000
						, WATERCNT1_Y
						);
	
	os_sprintf(HTTPBUFF	,"<tr /><tr />"
						 "<tr><th>Счетчик 2 (промывочный):</th></tr>"
						 "<tr><td>текущие показания:</td><td> %d.%03d м³ (%d л)</td></tr>"
						, WATERCNT2 / 1000
						, WATERCNT2 % 1000
						, WATERCNT2
						);
	
	os_sprintf(HTTPBUFF,"</table>");

	os_sprintf(HTTPBUFF	,"<br> <b>Режим:</b> %s"
						, (wash_state == STATE_WASH ) ? "Промывка" : "Норма"
						);
	
	uint32_t water[3];
	if ( wash_state == STATE_WASH )
	{
		if ( wash_type == WASH_FERRUM_FREE ) 
		{
			os_sprintf(HTTPBUFF," (обезжелезивание)");
			water[1] = WATERCNT2 - WASH_CNT2_START;
			water[2] = 0;
		}
		else
		{
			os_sprintf(HTTPBUFF," (умягчение)");
			water[1] = wash_cnt2_switch - WASH_CNT2_START;
			water[2] = WATERCNT2 - wash_cnt2_switch;
		}	
	
		water[0] = WATERCNT2 - WASH_CNT2_START;
		os_sprintf(HTTPBUFF,"<br><br><b>Текущая промывка</b>");
	} 
	else 
	{
		os_sprintf(HTTPBUFF,"<br><br><b>Предыдущая промывка</b>");
	
		water[0] = 0;		
		water[1] = 0;
		water[2] = 0;

		//if ( wash_cnt2_switch > 0 ) 
		//{
			water[0] = WASH_CNT2_END - WASH_CNT2_START;		
		//}

		if ( wash_cnt2_switch < WASH_CNT2_START) 
		{
			water[1] = WASH_CNT2_END - WASH_CNT2_START;	
		}
		else
		{
			water[1] = wash_cnt2_switch - WASH_CNT2_START;	
		}
		
		if ( 
				wash_cnt2_switch > 0 
			//&&  WASH_CNT2_END > WASH_CNT2_START 
			&& 	wash_cnt2_switch > WASH_CNT2_START
			)
		{
			water[2] = WASH_CNT2_END - wash_cnt2_switch;	
		}
	}

	os_sprintf(HTTPBUFF,"<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
						"<tr  align='center'>"
						"<th>Общий</th>"
						"<th>Железо</th>"
						"<th>Умягчение</th>"
						"</tr>"
						"<tr  align='center'>"
						"<td>%d л</td><td>%d л</td><td>%d л</td>"
						"</tr></table>"
						, water[0]
						, water[1]
						, water[2]
						);

	int hh = wash_duration / 3600;
	int mm = (wash_duration % 3600) / 60;
	int ss = (wash_duration % 3600) % 60;
	os_sprintf(HTTPBUFF,"<br> <b>Время промывки:</b> %02d:%02d:%02d", hh, mm, ss);

	os_sprintf(HTTPBUFF,"<br> <b>Промывок:</b> %d", wash_count);

	os_sprintf(HTTPBUFF,"<br> <b>Чистая вода:</b> %d.%03d м³ (%d л)", clean_water/1000, clean_water%1000, clean_water);

	char color[10];

	if ( percent < CLEAN_WATER_PERCENT )
		strcpy(color, "green");
	else if ( percent < 100 )
		strcpy(color, "orange");
	else
		strcpy(color, "red");

	os_sprintf(HTTPBUFF,"<br> <b>Ресурс:</b> <b><span style='color:%s'>%d %%</span></b>", color, percent );

	if ( wash_end_ts == TIMESTAMP_DEFAULT)	
		os_sprintf(HTTPBUFF,"<br><br> Промывка <b>--- дн. назад</b>"); 
	else
		os_sprintf(HTTPBUFF,"<br><br> Промывка <b>%d дн. назад</b> (%s)", ( GET_TS() > TIMESTAMP_DEFAULT ) ? PASSED_DAY_AFTER_WASH() : 0, sntp_get_real_time(wash_end_ts)); 
	
	// os_sprintf(HTTPBUFF,"<br> Счетчик1 на начало: %d", WASH_CNT1_START);
	// os_sprintf(HTTPBUFF,"<br> Счетчик2 на начало: %d", WASH_CNT2_START);
	// os_sprintf(HTTPBUFF,"<br> Счетчик2 на окончание: %d", WASH_CNT2_END);
	// os_sprintf(HTTPBUFF,"<br> wash_cnt2_switch: %d", wash_cnt2_switch);

	//uint32_t rr1 = read_eeprom(EEPROM_WASH_STATE_ADDR);
 	//os_sprintf(HTTPBUFF,"<br> rr1 (eeprom state): %d", rr1);

	//uint32_t rr2 = read_eeprom(EEPROM_WASH_TYPE_ADDR);
 	//os_sprintf(HTTPBUFF,"<br> rr2 (eeprom type): %d", rr2); 

 	//os_sprintf(HTTPBUFF,"<br> wash_state: %d", wash_state); 
 	//os_sprintf(HTTPBUFF,"<br> wash_type: %d", wash_type); 
 	// os_sprintf(HTTPBUFF,"<br> WASH_START_TS: %d", WASH_START_TS);
	// os_sprintf(HTTPBUFF,"<br> wash_end_ts: %d", wash_end_ts);
	// os_sprintf(HTTPBUFF,"<br> TIMESTAMP_DEFAULT: %d", TIMESTAMP_DEFAULT);
 	// os_sprintf(HTTPBUFF,"<br> watercnt2_change_ts: %d", watercnt2_change_ts); 
 	// os_sprintf(HTTPBUFF,"<br> XXX / TS: %d", GET_TS()); 

	os_sprintf(HTTPBUFF,"<br><br> <small>FW ver. %s</small>", "2.64");
}