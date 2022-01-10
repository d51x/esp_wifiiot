#define FW_VER "1.5"
/*
Отсечка счетчика,Время общее,Расход общий,Время предыдущее,Расход предыдущий,Время сегодня,Расход сегодня,Время вчера,Расход вчера
 
if(gpioget(15)==1)
gpioset(12,1)
else
if(d2d0101<2500)
gpioset(12,0)
else
if(d2d0101>2700)
gpioset(12,1)
endif
endif
endif
*/
#define FUEL_PUMP_GPIO	2
#define FUEL_PUMP_LED_GPIO 15

#define millis() (uint32_t) (micros() / 1000ULL) 

#define FUEL_PUMP "fPumpState"

#define SENS sensors_param
#define SENSCFG sensors_param.cfgdes
#define COUNTER_THRESHOLD SENSCFG[0]

//#define ADDLISTSENS {201,LSENSFL0,"имя","val1",&cnttest,NULL}, uint16_t cnttest;
// можно так уже к существущей переменной
// #define ADDLISTSENS {201,LSENSFL0,"имя","val1",&valdes[0],NULL},
// 201- номер, 2 параметр - режим. В данном случае 0 знаков после запятой
/*
1,98	л/ч
1980	мл/ч
33		мл/мин
0,55	мл/сек
*/

#define CONSUMP_ML_SEC 0.55f
#define CONSUMP_L_SEC 100000*0.00055f

uint16_t fpState=0;

//uint32_t fpWorkA=0;		// время работы общее
#define fpWorkA SENSCFG[1]
//uint32_t fpWorkT=0;		// время работы сегодня
#define  fpWorkT SENSCFG[5]
//uint32_t fpWorkY=0;		// время работы вчера
#define fpWorkY SENSCFG[7]
//uint32_t fpWorkL=0;		// время работы предыдущее
#define fpWorkL SENSCFG[3]
uint32_t fpWorkN=0; 	// время работы текущее

//uint32_t fpRateA=0;		// расход общее
#define fpRateA SENSCFG[2]
//uint32_t fpRateT=0;		// расход сегодня
#define fpRateT SENSCFG[6]
//uint32_t fpRateY=0;		// расход вчера
#define fpRateY SENSCFG[8]
//uint32_t fpRateL=0;		// расход предыдущее
#define fpRateL SENSCFG[4]
uint32_t fpRateN=0; 	// расход текущее

uint32_t getFuelRateTotal() {return 	fpRateA / 100;}
uint32_t getFuelRateToday() {return 	fpRateT / 100;}
uint32_t getFuelRatePrev()	{return 	fpRateY / 100;}
uint32_t getFuelRateLast()	{return 	fpRateL / 100;}
uint32_t getFuelRateNow()	{return 	fpRateN / 100;}

uint32_t getFuelWorkTotal() {return 	fpWorkA;}
uint32_t getFuelWorkToday() {return 	fpWorkT;}
uint32_t getFuelWorkPrev()	{return 	fpWorkY;}
uint32_t getFuelWorkLast()	{return 	fpWorkL;}


uint16_t on_cnt = 0;

#define ADDLISTSENS {200,LSENSFL0,"FuelPump",FUEL_PUMP,&fpState,NULL}, \
					{201,LSENSFL3|LSENS32BIT|LSENSFUNS,"fuelRateTotal","fRateA",getFuelRateTotal,NULL}, \
					{202,LSENSFL3|LSENS32BIT|LSENSFUNS,"fuelRateToday","fRateT",getFuelRateToday,NULL}, \
					{203,LSENSFL3|LSENS32BIT|LSENSFUNS,"fuelRatePrev","fRateY",getFuelRatePrev,NULL}, \
					{204,LSENSFL3|LSENS32BIT|LSENSFUNS,"fuelRateLast","fRateL",getFuelRateLast,NULL}, \
					{205,LSENSFL3|LSENS32BIT|LSENSFUNS,"fuelRateNow","fRateN",getFuelRateNow,NULL}, \
					{206,LSENSFL0|LSENS32BIT|LSENSFUNS,"fuelWorkTotal","fpWorkA",getFuelWorkTotal,NULL}, \
					{207,LSENSFL0|LSENS32BIT|LSENSFUNS,"fuelWorkToday","fpWorkT",getFuelWorkToday,NULL}, \
					{208,LSENSFL0|LSENS32BIT|LSENSFUNS,"fuelWorkPrev","fpWorkY",getFuelWorkPrev,NULL}, \
					{209,LSENSFL0|LSENS32BIT|LSENSFUNS,"fuelWorkLast","fpWorkL",getFuelWorkLast,NULL}, \
					{210,LSENSFL0|LSENS32BIT,"fuelWorkNow","fpWorkN",&fpWorkN,NULL}, \
					{211,LSENSFL0,"OnCount","oncnt",&on_cnt,NULL},
	typedef struct {
		uint32_t _fpRateA;
		uint32_t _fpRateT;
		uint32_t _fpRateY;
		uint32_t _fpRateL;

		uint32_t _fpWorkA;
		uint32_t _fpWorkT;
		uint32_t _fpWorkY;
		uint32_t _fpWorkL;

		uint16_t on_cnt;
		uint32_t crc32;
	} rtc_data_t;

	rtc_data_t rtc_data;
	
	uint8_t opt_saving = 0;
	


uint32_t calcCRC32(const uint8_t *data, uint16_t sz) {
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

void ICACHE_FLASH_ATTR startfunc()
{
	
	system_rtc_mem_read(70, &rtc_data, sizeof(rtc_data_t));
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	if ( crc32 != rtc_data.crc32 ) {
		// кривые данные, обнулим
		os_memset(&rtc_data, 0, sizeof(rtc_data_t));
		crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
		rtc_data.crc32 = crc32;
		// пишем в rtc обнуленные данные
		system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));		
	}
	
	on_cnt = rtc_data.on_cnt;
	
	// если значения сохраненных данных меньше чем данные из rtc
	if (fpWorkA < rtc_data._fpWorkA) fpWorkA = rtc_data._fpWorkA;
	if (fpWorkT < rtc_data._fpWorkT) fpWorkT = rtc_data._fpWorkT;
	if (fpWorkY < rtc_data._fpWorkY) fpWorkY = rtc_data._fpWorkY;
	if (fpWorkL < rtc_data._fpWorkL) fpWorkL = rtc_data._fpWorkL;
	
	if (fpRateA < rtc_data._fpRateA) fpRateA = rtc_data._fpRateA;
	if (fpRateT < rtc_data._fpRateT) fpRateT = rtc_data._fpRateT;
	if (fpRateY < rtc_data._fpRateY) fpRateY = rtc_data._fpRateY;
	if (fpRateL < rtc_data._fpRateL) fpRateL = rtc_data._fpRateL;
}


static void ICACHE_FLASH_ATTR mqtt_send_int(const char *topic, int val)
{
	#if mqtte
	if ( !sensors_param.mqtten ) return;		
	char payload[10];
	os_sprintf(payload,"%d", val);
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 0, 0, 0);
	#endif
}


// void ICACHE_FLASH_ATTR save_data(uint8_t save)
// {
// 	// FUEL_WORK_TOTAL = fpWorkA;
// 	// FUEL_WORK_DAY = fpWorkT;
// 	// FUEL_WORK_PREV = fpWorkY;
// 	// FUEL_WORK_LAST = fpWorkL;
	
// 	// FUEL_RATE_TOTAL = fpRateA;
// 	// FUEL_RATE_DAY = fpRateT;
// 	// FUEL_RATE_PREV = fpRateY;
// 	// FUEL_RATE_LAST = fpRateL;

// 	if (save) SAVEOPT;
// }

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
	if ( safemode ) return;

	// корректировка нулевых значений счетчиков
	if ( rtc_data._fpRateA == 0) rtc_data._fpRateA = fpRateA;
	if ( rtc_data._fpRateT == 0) rtc_data._fpRateT = fpRateT;
	if ( rtc_data._fpRateY == 0) rtc_data._fpRateY = fpRateY;
	if ( rtc_data._fpRateL == 0) rtc_data._fpRateL = fpRateL;
	
	if ( rtc_data._fpWorkA == 0) rtc_data._fpWorkA = fpWorkA;
	if ( rtc_data._fpWorkT == 0) rtc_data._fpWorkT = fpWorkT;
	if ( rtc_data._fpWorkY == 0) rtc_data._fpWorkY = fpWorkY;
	if ( rtc_data._fpWorkL == 0) rtc_data._fpWorkL = fpWorkL;
	
	if ( rtc_data.on_cnt == 0) rtc_data.on_cnt = on_cnt;
	
	if ( time_loc.hour == 23 && time_loc.min == 59 && time_loc.sec == 0 )
	{
		// обнулить суточные данные ночью
		fpWorkY = fpWorkT;
		fpWorkT = 0;
		fpRateY = fpRateT;
		fpRateT = 0;
		on_cnt = 0;
	} 
	
	static uint8_t prev_state = 0;
	static uint32_t t_start = 0;
	
	fpState = ( count60end[0] > COUNTER_THRESHOLD );  // если просто > 0? то проскакивают левые импульсы
	
	if ( prev_state != fpState )
	{
		GPIO_ALL( FUEL_PUMP_LED_GPIO, fpState );
		mqtt_send_int( FUEL_PUMP, fpState );
		prev_state = fpState;
		
		if ( fpState ) 
		{
			// переключился из 0 в 1  (!!! может проскакивать импульс и поэтому cnt увеличивается на 1 и предыдущее время обнуляется, регулируется отсечкой подсчета импульсов)
			on_cnt++;
			t_start = millis(); // при включении начали отсчет
			fpRateN = 0; // обнуление текущего расхода	
		} else {
			// переключился из 1 в 0
			fpWorkL = fpWorkN;
			fpRateL = fpRateN;
			fpRateN = 0;
			fpWorkN = 0;
			// сохраним в nvs
			//save_data(0); SAVEOPT
		}
	}
	
	if ( fpState ) {
		fpWorkA++;
		fpRateA += CONSUMP_L_SEC;//*100000;
		
		fpWorkT++;
		fpRateT += CONSUMP_L_SEC;//*100000;
		
		fpRateN += CONSUMP_L_SEC;//*100000;
		fpWorkN = millis() - t_start;	// считаем время
	}
	
	
	if ( GPIO_ALL_GET( 6 ) == 1 )
	{
		fpRateA = 0;
		fpRateT = 0;
		fpRateY = 0;
		fpRateL = 0;
		
		fpWorkA = 0;
		fpWorkT = 0;
		fpWorkY = 0;
		fpWorkL = 0;

		on_cnt = 0;
		
		GPIO_ALL(6, 0);
		
		//save_data(1);
		SAVEOPT;
	}
	
	// пишем данные в rtc
	rtc_data._fpRateA = fpRateA;
	rtc_data._fpRateT = fpRateT;
	rtc_data._fpRateY = fpRateY;
	rtc_data._fpRateL = fpRateL;
	
	rtc_data._fpWorkA = fpWorkA;
	rtc_data._fpWorkT = fpWorkT;
	rtc_data._fpWorkY = fpWorkY;
	rtc_data._fpWorkL = fpWorkL;
	
	rtc_data.on_cnt = on_cnt;
	
	uint32_t crc32 = calcCRC32( (uint8_t *)&rtc_data, sizeof(rtc_data_t));
	rtc_data.crc32 = crc32;
	system_rtc_mem_write(70, &rtc_data, sizeof(rtc_data_t));
	
	//save_data(0);
	
	if ( timersrc % 3600 ) {
		//save_data(1);
		SAVEOPT;
	}
}


void webfunc(char *pbuf) 
{
	os_sprintf(HTTPBUFF, "<b>Fuel Pump:</b> %s &nbsp; <b>count:</b> %d <br>", fpState ? "ON" : "OFF", on_cnt );
	
	uint32_t sec, min, hour = 0;

	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"
							"<tr align='center'><th></th><th>Work time:</th><th>Consumption, L:</th></tr>"
				);
				

	os_sprintf(HTTPBUFF, 	"<tr align='center'><td><b>Now:</b></td><td>%02d:%02d</td><td>%d.%03d</td></tr>"
							, (fpWorkN / 1000) / 60,  (fpWorkN / 1000) % 60
							, fpRateN / 100000, (fpRateN % 100000) / 100
	);				

	os_sprintf(HTTPBUFF, 	"<tr align='center'><td><b>Last:</b></td><td>%02d:%02d</td><td>%d.%03d</td></tr>"
							, (fpWorkL / 1000) / 60,  (fpWorkL / 1000) % 60
							, fpRateL / 100000, (fpRateL % 100000) / 100
	);

	sec = fpWorkT % 60;
	min = fpWorkT / 60;
	hour = min / 60;
	min = min % 60;
	os_sprintf(HTTPBUFF, 	"<tr align='center'><td><b>Today:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td></tr>"
							, hour, min, sec
							, fpRateT / 100000, (fpRateT % 100000) / 100
	);


	sec = fpWorkY % 60;
	min = fpWorkY / 60;
	hour = min / 60;
	min = min % 60;
	os_sprintf(HTTPBUFF, 	"<tr align='center'><td><b>Yesterday:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td></tr>"
							, hour, min, sec
							, fpRateY / 100000, (fpRateY % 100000) / 100
	);
	
	sec = fpWorkA % 60;
	min = fpWorkA / 60;
	hour = (min / 60 % 24);
	min = min % 60;
	os_sprintf(HTTPBUFF, 	"<tr align='center'><td><b>Total:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td></tr>"
							, hour, min, sec
							, fpRateA / 100000, (fpRateA % 100000) / 100
	);	
	
	os_sprintf(HTTPBUFF, 	"</table>");	
	os_sprintf(HTTPBUFF, 	"<br><small>ver.%s</small>", FW_VER);	
	
}