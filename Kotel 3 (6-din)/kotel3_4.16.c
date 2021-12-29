#define FW_VER "4.16"

/*
Количество настроек
Kotel1 gpio, Kotel2 gpio, Pump1 gpio, Pump2 gpio, ESC gpio, Vent gpio, Night(h), Day(h), BacklightTDelay, Kotel1LED, Kotel2LED, KotelWorkLed, PumpWorkLed, ScheduleLed, VentLed, GlobalTempSet, Buzzer GPIO, PumpMode, PumpTimeout
*/

#define TEMPSET_STEP 1
#define HYST_STEP 1
#define TEMPSET_MIN 100
#define TEMPSET_MAX 400
#define HYST_MIN 1
#define HYST_MAX 50

#define SENS sensors_param
#define SENSCFG SENS.cfgdes

#define KOTEL1_IO SENSCFG[0]//208
#define KOTEL2_IO SENSCFG[1]//209
#define PUMP1_IO SENSCFG[2]//210
#define PUMP2_IO SENSCFG[3]//211
#define ESC_IO SENSCFG[4]//212
#define VENT_IO SENSCFG[5]//213

#define NIGHT_TIME SENSCFG[6]//23
#define DAY_TIME SENSCFG[7]//7

#define BACKLIGHT_TIMEOUT SENSCFG[8]//30

#define KOTEL1_LED_IO SENSCFG[9]
#define KOTEL2_LED_IO SENSCFG[10]
#define KOTEL_LED_IO SENSCFG[11]
#define PUMP_LED_IO SENSCFG[12]
#define SCHEDULE_LED_IO SENSCFG[13]
#define VENT_LED_IO SENSCFG[14]

#define TEMPSET SENSCFG[15]
#define BUZZER_IO SENSCFG[16]
#define PUMP_MODE SENSCFG[17]
#define PUMP_OFF_TIMEOUT SENSCFG[18]*1000

#define curT valdes[0]
#define streetT valdes[1]
#define IDX_WORKMODE 2
#define workMode valdes[IDX_WORKMODE]
#define IDX_SCHED 3
#define schedule valdes[IDX_SCHED]
#define reset_fuel valdes[4]

#define flowT data1wire[0]
#define retnT data1wire[1]

#define KOTEL1_IO_D 208
#define KOTEL2_IO_D 209
#define PUMP1_IO_D 210
#define PUMP2_IO_D 211
#define ESC_IO_D 212
#define VENT_IO_D 213

#define NIGHT_TIME_D 23
#define DAY_TIME_D 7

#define BACKLIGHT_IO 199

#define BITCHK(r,n)(r&(1<<n))
#define VPAUSE(t)(t/portTICK_PERIOD_MS)
#define GPIO_INVERT(p)(GPIO_ALL(p,!GPIO_ALL_GET(p))) 

#define THERMO_STATE(x) BITCHK(SENS.thermo[x-1][0],0)
#define THERMO_ON(x){if(!GPIO_ALL_GET(x+99))GPIO_ALL(99+x,1);}
#define THERMO_OFF(x){if(GPIO_ALL_GET(99+x))GPIO_ALL(99+x,0);}

#define THERMO_SETPOINT(x) SENS.thermzn[x-1][0]
#define THERMO_HYSTERESIS(x) SENS.thermzn[x-1][1]
#define THERMO_TEMP_SET(x,y){SENS.thermzn[x-1][0]=y;}
#define THERMO_HYST_SET(x,y){SENS.thermzn[x-1][1]=y;SAVEOPT;}

#define NVSNAME "d51x"
#define PRM_WORKMODE "workmode"
#define PRM_SCHEDULE "schedule"
#define PRM_TEMPSET "tempset"
#define EMPTYSTR"                    "

#define MCP23017_ISR_DELAY_MS 60

#define millis() (unsigned long)(esp_timer_get_time()/1000ULL)

typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;

typedef void (*func_cb)();

typedef enum {
PAGE_MAIN
,PAGE_KOTEL1_RATE
,PAGE_KOTEL2_RATE
,PAGE_WORKMODE
,PAGE_SCHEDULE
,PAGE_TEMPSET
,PAGE_HYST
,PAGE_PUMP_SETTINGS
,PAGE_VERSION
,PAGE_MAX} menu_e;

menu_e mId = PAGE_MAIN;

typedef enum {
MODE_MANUAL,
MODE_AUTO,
MODE_KOTEL1,
MODE_KOTEL2,
MODE_MAX} mode_e;

u8_t activeKotel=0;
u16_t schedTempSet=0;
u8_t displayAlert=0;
TimerHandle_t showAlertTmr;
#define ALERT_TIMEOUT 3000

u32_t lastPress=0;
#define MENU_TIMEOUT 60000

typedef enum{
PUMP_MODE_NONE,//nocontrol
PUMP_MODE_1,//by kotel relay
PUMP_MODE_2,//on by kotel relay,off witin X min kotel off
PUMP_MODE_3,//on by kotel relay,off by return temo
PUMP_MODE_MAX} pump_mode_e;

esp_err_t nvsLoad(const char* s,const char* k,void* d){
    esp_err_t e=ESP_ERR_INVALID_ARG;
    nvs_handle h;
    size_t sz=0;
    nvs_open(s,NVS_READWRITE,&h);
    nvs_get_blob(h,k,NULL,&sz);
    if(!sz){e=ESP_FAIL;goto LOAD_FINISH;}
    e=nvs_get_blob(h,k,d,&sz);
  LOAD_FINISH:
    nvs_close(h);
  OPEN_FAIL:
    return e;
}

esp_err_t nvsSave(const char* s,const char* k,void *p,u16_t l){
    esp_err_t e=ESP_ERR_INVALID_ARG;
    nvs_handle h;
    nvs_open(s,NVS_READWRITE,&h);
    nvs_set_blob(h,k,p,l);
    e=nvs_commit(h);
  SAVE_FINISH:
    nvs_close(h);
  OPEN_FAIL:
    return e;
}

esp_err_t nvsSave32u(const char* s,const char* k,u32_t *p){
    u32_t v=0;
    if(nvsLoad(s,k,&v)!=ESP_OK) return nvsSave(s,k,p,sizeof(u32_t));
    if(v!=*p) return nvsSave(s,k,p,sizeof(u32_t));
}

/*Buzzer*/
typedef enum{
BEEP_SHORT_EXTRA,
BEEP_SHORT_VERY,
BEEP_SHORT,
BEEP_MEDIUM,
BEEP_LONG,
BEEP_LONG_VERY,
BEEP_LONG_EXTRA,
BEEP_DOUBLE_SHORT,
BEEP_ERROR,
BEEP_MAX} beep_type_e;

QueueHandle_t buzzQueue;
TaskHandle_t buzzTask;

typedef struct {
u16_t action;/*0-off,1-on*/
u16_t delay;//msec 
} buzzBeep_t;

#define BEEP_CMD_LEN 6
buzzBeep_t beeps[BEEP_MAX][BEEP_CMD_LEN]={ 
{{1,40},{0,0},{0,0},{0,0},{0,0},{0,0}}//SHORT_EXTRA
,{{1,80},{0,0},{0,0},{0,0},{0,0},{0,0}}//SHORT_VERY
,{{1,120},{0,0},{0,0},{0,0},{0,0},{0,0}}//SHORT
,{{1,160},{0,0},{0,0},{0,0},{0,0},{0,0}}//MEDIUM
,{{1,200},{0,0},{0,0},{0,0},{0,0},{0,0}}//LONG
,{{1,300},{0,0},{0,0},{0,0},{0,0},{0,0}}//LONG_VERY
,{{1,500},{0,0},{0,0},{0,0},{0,0},{0,0}}//LONG_EXTRA
,{{1,120},{0,120},{1,120},{0,0},{0,0},{0,0}}//DOUBLE_SHORT
,{{1,100},{0,100},{1,100},{0,100},{1,100},{0,0}}//ERROR
};

void buzzBeep(u8_t v){
    GPIO_ALL(BUZZER_IO,0);
    for(u8_t i=0;i<BEEP_CMD_LEN;i++){
        GPIO_ALL(BUZZER_IO,beeps[v][i].action);
        if(beeps[v][i].delay>0) vTaskDelay(VPAUSE(beeps[v][i].delay));
    }
    GPIO_ALL(BUZZER_IO,0);
}

static void buzzCb(void *a){
 u8_t p;
 while(1){
  if(xQueueReceive(buzzQueue,&p,0)) buzzBeep(p);
  vTaskDelay(VPAUSE(10));
 }
 vTaskDelete(NULL);
}

void buzzer(u8_t p){
 if(BUZZER_IO==255)return;
 GPIO_ALL(BUZZER_IO,0);
 xQueueOverwrite(buzzQueue,(void*)&p);
}

void buzzInit(){
 buzzQueue=xQueueCreate(1,2);
 xTaskCreate(buzzCb,"buzz",896,NULL,10,&buzzTask);
 buzzer(BEEP_ERROR);
}

#define FUEL_PUMP_IO 13
#define RATE_L_SEC 100000*0.00055f
#define COUNTER_THRESHOLD 30

u16_t fPumpSt=0;
u32_t fPumpStart=0;
u16_t fPumpOnCnt=0;
u32_t fPumpOnDur=0;
u32_t fPumpOnDurPrev=0;
u32_t fPumpWorkTotal=0;
u32_t fPumpWorkToDay=0;
u32_t fPumpWorkPrevDay=0;

u32_t fuelRatePrev=0;
u32_t fuelRateNow=0;
u32_t fuelRateToDay=0;
u32_t fuelRatePrevDay=0;
u32_t fuelRateTotal=0;

/*NVS FUEL PUMP*/
#define SPACE_FUEL "fuelpump"
#define PRM_FSTATE_CNT "fuelcnt"
#define PRM_FRATE_LAST "conslast"
#define PRM_FRATE_NOW "consnow"
#define PRM_FRATE_DAY "consday"
#define PRM_FRATE_PREV "consprev"
#define PRM_FRATE_TOTAL "consttl"

#define PRM_FWRK_LAST "wrktlast"
#define PRM_FWRK_NOW "wrktnow"
#define PRM_FWRK_DAY "wrktday"
#define PRM_FWRK_PREV "wrktprev"
#define PRM_FWRK_TOTAL "wrktttl"

u32_t getRateTotal(){return fuelRateTotal/100;}
u32_t getRateDay(){return fuelRateToDay/100;}
u32_t getRatePrev(){return fuelRatePrevDay/100;}

void detectFuelWork(){
#ifdef count60e
fPumpSt=(count60end[0]>COUNTER_THRESHOLD);
static u16_t prevSt=0;
if(fPumpSt!=prevSt){
 prevSt=fPumpSt;
 if(fPumpSt){
  fPumpOnCnt++;
  fPumpStart=millis();
  fuelRateNow=0;
  nvsSave32u(SPACE_FUEL,PRM_FSTATE_CNT,&fPumpOnCnt);
 }else{
  fuelRatePrev=fuelRateNow;
  fPumpOnDurPrev=fPumpOnDur;
  fPumpOnDur=fuelRateNow=0;
  nvsSave32u(SPACE_FUEL,PRM_FRATE_LAST,&fuelRatePrev);
  nvsSave32u(SPACE_FUEL,PRM_FWRK_LAST,&fPumpOnDurPrev);
  nvsSave32u(SPACE_FUEL,PRM_FRATE_NOW,&fuelRateNow);
  nvsSave32u(SPACE_FUEL,PRM_FWRK_NOW,&fPumpOnDur);
  nvsSave32u(SPACE_FUEL,PRM_FRATE_DAY,&fuelRateToDay);
  nvsSave32u(SPACE_FUEL,PRM_FWRK_DAY,&fPumpWorkToDay);
  nvsSave32u(SPACE_FUEL,PRM_FRATE_TOTAL,&fuelRateTotal);
  nvsSave32u(SPACE_FUEL,PRM_FWRK_TOTAL,&fPumpWorkTotal);
 }
}
#endif
}

void calcFuelRate(){
#ifdef count60e
 static u8_t z=1;
 if(time_loc.hour==1 && !time_loc.min && !time_loc.sec){
  fPumpWorkPrevDay=fPumpWorkToDay;
  fuelRatePrevDay=fuelRateToDay;
  fPumpWorkToDay=fuelRateToDay=fPumpOnCnt=0;
  if(z){
   nvsSave32u(SPACE_FUEL,PRM_FRATE_DAY,&fuelRateToDay);
   nvsSave32u(SPACE_FUEL,PRM_FRATE_PREV,&fuelRatePrevDay);
   nvsSave32u(SPACE_FUEL,PRM_FWRK_DAY,&fPumpWorkToDay);
   nvsSave32u(SPACE_FUEL,PRM_FWRK_PREV,&fuelRatePrevDay);
   z=0;
  }
 }
 if(fPumpSt){
  fPumpWorkTotal++; 
  fPumpWorkToDay++;
  fuelRateTotal++;
  fuelRateNow +=RATE_L_SEC;
  fuelRateToDay +=RATE_L_SEC;
  fuelRateTotal +=RATE_L_SEC;
  fPumpOnDur=millis()-fPumpStart;
 } 
#endif
}

void resetFuel(){
 fuelRatePrev=fuelRateNow=fuelRateTotal=fuelRateToDay=fuelRatePrevDay=fPumpOnDurPrev=fPumpOnDur=fPumpWorkTotal=fPumpWorkToDay=fPumpWorkPrevDay=fPumpOnCnt=0;
 nvsSave32u(SPACE_FUEL,PRM_FSTATE_CNT,&fPumpOnCnt);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_LAST,&fuelRatePrev);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_NOW,&fuelRateNow);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_DAY,&fuelRateToDay);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_PREV,&fuelRatePrevDay);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_TOTAL,&fuelRateTotal);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_LAST,&fPumpOnDurPrev);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_NOW,&fPumpOnDur);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_DAY,&fPumpWorkToDay);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_PREV,&fuelRatePrevDay);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_TOTAL,&fPumpWorkTotal);  
}

void saveFuel(){
 nvsSave32u(SPACE_FUEL,PRM_FSTATE_CNT,&fPumpOnCnt);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_LAST,&fuelRatePrev);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_DAY,&fuelRateToDay);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_NOW,&fuelRateNow);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_PREV,&fuelRatePrevDay);
 nvsSave32u(SPACE_FUEL,PRM_FRATE_TOTAL,&fuelRateTotal);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_NOW,&fPumpOnDur);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_LAST,&fPumpOnDurPrev);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_DAY,&fPumpWorkToDay);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_PREV,&fuelRatePrevDay);
 nvsSave32u(SPACE_FUEL,PRM_FWRK_TOTAL,&fPumpWorkTotal);
}

void loadFuel(){
 nvsLoad(SPACE_FUEL,PRM_FSTATE_CNT,&fPumpOnCnt);    
 nvsLoad(SPACE_FUEL,PRM_FRATE_LAST,&fuelRatePrev);
 nvsLoad(SPACE_FUEL,PRM_FWRK_LAST,&fPumpOnDurPrev);
 nvsLoad(SPACE_FUEL,PRM_FRATE_NOW,&fuelRateNow);
 nvsLoad(SPACE_FUEL,PRM_FWRK_NOW,&fPumpOnDur);
 nvsLoad(SPACE_FUEL,PRM_FRATE_DAY,&fuelRateToDay);
 nvsLoad(SPACE_FUEL,PRM_FWRK_DAY,&fPumpWorkToDay);
 nvsLoad(SPACE_FUEL,PRM_FRATE_PREV,&fuelRatePrevDay);
 nvsLoad(SPACE_FUEL,PRM_FWRK_PREV,&fuelRatePrevDay);
 nvsLoad(SPACE_FUEL,PRM_FRATE_TOTAL,&fuelRateTotal);
 nvsLoad(SPACE_FUEL,PRM_FWRK_TOTAL,&fPumpWorkTotal);
}

#define TBL_TR "<tr align=right>"
#define TD "<td>"
#define TDE "</td><td>"
#define OFF "OFF"
#define TBL1 "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"

void printFuelPump(char *pbuf){
 os_sprintf(HTTPBUFF, "<hr>");
 u32_t s=fPumpWorkTotal%60;
 u32_t m=fPumpWorkTotal/60;
 u32_t h=m/60%24;
 m%=60;

 u32_t _s=fPumpWorkToDay%60;
 u32_t _m=fPumpWorkToDay/60;
 u32_t _h=_m/60;
 _m%=60;
 os_sprintf(HTTPBUFF,TBL1 TBL_TR TD"Fuel Pump:"TDE"<b>%s</b>"TDE"count: <b>%d</b></td></tr>"TBL_TR"<th></th><th>Work time:</th><th>Consumption, L:</th></tr>",fPumpSt?"ON":OFF,fPumpOnCnt);
 os_sprintf(HTTPBUFF,TBL_TR TD"<b>Now:</b>"TDE"%02d:%02d:%02d"TDE"%d.%03d</td></tr>",fPumpOnDur/3600000,fPumpOnDur/60000,(fPumpOnDur/1000)%60,fuelRateNow/100000,(fuelRateNow%100000)/100);
 os_sprintf(HTTPBUFF,TBL_TR"<td><b>Today:</b>"TDE"%02d:%02d:%02d"TDE"%d.%03d</td></tr>",_h,_m,_s,fuelRateToDay/100000,(fuelRateToDay%100000)/100);
 os_sprintf(HTTPBUFF,"</table><details><summary></summary>"TBL1 TBL_TR"<td><b>Prev time:</b>"TDE"%02d:%02d:%02d"TDE"%d.%03d</td></tr>",fPumpOnDurPrev/3600000,fPumpOnDurPrev/60000,(fPumpOnDurPrev/1000)%60,fuelRatePrev/100000,(fuelRatePrev%100000)/100);
 _s=fPumpWorkPrevDay%60;
 _m=fPumpWorkPrevDay/60;
 _h=_m/60;
 _m%=60;
 os_sprintf(HTTPBUFF,TBL_TR"<td><b>Yesterday:</b>"TDE"%02d:%02d:%02d"TDE"%d.%03d</td></tr>",_h,_m,_s,fuelRatePrevDay/100000,(fuelRatePrevDay%100000)/100);
 os_sprintf(HTTPBUFF,TBL_TR"<td><b>Total:</b>"TDE"%02d:%02d:%02d"TDE"%d.%03d</td></tr>",h,m,s,fuelRateTotal/100000,(fuelRateTotal%100000)/100);
 os_sprintf(HTTPBUFF,"</table></details>");
}

/*LCD*/
#include "tcpip_adapter.h"
#include "lwip/ip_addr.h"

#if lcde
#define BACKLIGHT_STATE BITCHK(SENS.lcdled,0)
#else
#define BACKLIGHT_STATE 1
#endif

const u8_t chrMoon[8]={28,6,3,3,3,6,28,0,};
#define S_MOON 2
const u8_t chrSun[8]={4,21,14,31,14,21,4,0,}; 
#define S_SUN 3
const u8_t chrVent1[8]={3,2,25,30,22,25,3,7,};
#define S_VENT1 4
const u8_t chrVent2[8]={24,24,19,13,15,19,8,24,};
#define S_VENT2 5
const u8_t chrSchd1[8]={6,15,16,22,18,23,16,15,};
#define S_SCHD1 6
const u8_t chrSchd2[8] ={12,30,1,25,9,29,1,30,};
#define S_SCHD2 7
const u8_t chrBar[8] ={31,31,31,31,31,31,31,31,};
#define S_BAR 8

#define SPLASH_TIME 5
#define S_DEG 1
#define S_SPC 32
#define S_RIGHT 126
#define S_LEFT 127

TimerHandle_t backlightTmr;
u8_t splash=1;

#define PAGE_KOTEL1_LINES 10
u8_t pageKotel1Line;

#define LCD_PUMP_OPTIONS 3
u8_t pumpOpt;
u8_t pumpInMenu;

void backlightTmrCb(xTimerHandle tmr){
 u8_t p=(u8_t)pvTimerGetTimerID(tmr);
 GPIO_ALL(p,0);
 xTimerStop(tmr,10);
 xTimerDelete(tmr,10);
 backlightTmr=NULL;
}

void backlightTurnOn(u8_t p,u8_t *s){
 GPIO_ALL(p,1);
 if(!backlightTmr) backlightTmr=xTimerCreate("bcklght",VPAUSE(BACKLIGHT_TIMEOUT*1000),pdFALSE,p,backlightTmrCb);
 if(xTimerIsTimerActive(backlightTmr))xTimerStop(backlightTmr,0);
 xTimerStart(backlightTmr,0);
}

void wLcdPrint(u8_t l,const char *s){
 #if lcde
 LCD_print(l,s);
 #endif
}

void lcdPrint(u8_t l,const char *s){
 if(!displayAlert) wLcdPrint(l,s);
}

void showDisplayAlertCb(xTimerHandle tmr){
 displayAlert=0;
 xTimerStop(showAlertTmr,0);
 xTimerDelete(showAlertTmr,10);
 showAlertTmr=NULL;
}

void alertPrint(const char *t,const char *s){
 wLcdPrint(0,t);
 char e[21];
 if(strlen(s)>20){
  strncpy(e,s,21);
  wLcdPrint(1,e);
  s+=20;  
  if(strlen(s)>20){
   strncpy(e,s,21);
   wLcdPrint(2,e);
   s+=20;
   wLcdPrint(3,s);
  }else{
   wLcdPrint(2,s);
   wLcdPrint(3,EMPTYSTR);
  }  
 }else{
  wLcdPrint(1,EMPTYSTR);
  wLcdPrint(2,s);
  wLcdPrint(3,EMPTYSTR);
 }
}

void showDisplayAlert(const char *t,const char *s,func_cb cb){
 displayAlert=0;
 if(!showAlertTmr) showAlertTmr=xTimerCreate("lcdalert",VPAUSE(ALERT_TIMEOUT),pdFALSE,NULL,showDisplayAlertCb);
 if(xTimerIsTimerActive(showAlertTmr)) xTimerStop(showAlertTmr,0);
 xTimerStart(showAlertTmr,0);
 if(cb) cb(); else alertPrint(t,s);
 displayAlert=1;
}

void nextMenu(){
 if(!displayAlert){
  mId++;
  if(mId>=PAGE_MAX) mId=PAGE_MAIN;
  lastPress=millis();
  if(mId==PAGE_KOTEL1_RATE) pageKotel1Line=0;
 }  
}

void prevMenu(){
 if(!displayAlert){
  mId--;
  if(mId==PAGE_MAIN) mId=PAGE_MAX-1;
  lastPress=millis();
  if(mId==PAGE_KOTEL1_RATE) pageKotel1Line=0;
 }
}

void pageMain(){
 char s[30];
 static u32_t i=0;
 char wd[2]="";
 switch(time_loc.dow){
  case 0:strcpy(wd,"Mo");break;
  case 1:strcpy(wd,"Tu");break;
  case 2:strcpy(wd,"We");break;
  case 3:strcpy(wd,"Th");break;
  case 4:strcpy(wd,"Fr");break;
  case 5:strcpy(wd,"Sa");break;
  case 6:strcpy(wd,"Su");break;
  default:break;
 }
 #define td 20

 snprintf(s,21,"%2s, %02d%s%02d   T%c:%2d.%1d%c",wd,(i%td>0)?time_loc.hour:time_loc.day,(i%td>0)?(i%2?":":" "):".",(i%td>0)?time_loc.min:time_loc.month,S_RIGHT,flowT/100,(flowT%100)/10,S_DEG);
 lcdPrint(0,s);
 
 char sm[10]="";
 if(workMode==MODE_MANUAL){
  strcpy(sm,"MANUAL[");
  strcat(sm,GPIO_ALL_GET(KOTEL1_IO)?"1":"-");
  strcat(sm,GPIO_ALL_GET(KOTEL2_IO)?"2":"-");
 }else if(workMode==MODE_KOTEL1){
  strcpy(sm,"KOTEL1[");
  strcat(sm,GPIO_ALL_GET(KOTEL1_IO)?"*":"-");
 }else if(workMode==MODE_KOTEL2){
  strcpy(sm,"KOTEL2[");
  strcat(sm,GPIO_ALL_GET(KOTEL2_IO)?"*":"-");
 }else if(workMode==MODE_AUTO){
  strcpy(sm,"AUTO[");
  if(activeKotel==1) strcat(sm,THERMO_STATE(1)?"1*":"1");
  else if(activeKotel==2) strcat(sm,THERMO_STATE(2)?"2*":"2");
  else strcat(sm,"-");
 }else strcpy(sm,"ERROR[-");
 strcat(sm,"]");
 snprintf(s,21,"%-10s  T%c:%2d.%1d%c",sm,S_LEFT,retnT/100,(retnT%100)/10,S_DEG);
 lcdPrint(1,s);

 char sc[6]="";
 if(schedule){
  static p=0;
  strcpy(sc,"");
  for(u8_t j=0;j<5;j++) strcat(sc,j==p?">":"-");
  p++;
  if(p>5) p=0;
  strcat(sc," ");
 }else strcpy(sc,i%2?"     >":"      ");

 snprintf(s,21,"%c%c%3s %6sTs:%2d.%1d%c",S_SCHD1,S_SCHD2,schedule?"ON ":"OFF",sc,THERMO_SETPOINT(1)/10,THERMO_SETPOINT(1)%10,S_DEG);
 lcdPrint(2,s);
 
 static u8_t _show=0;
 if(i%5==0) _show=1-_show;
 int _temp=streetT%10;
 _temp=(_temp<0)?_temp*-1:_temp;
 snprintf(s,21,"%c%c%3s %c%3s  T%c:%2d.%1d%c",S_VENT1,S_VENT2,GPIO_ALL_GET(VENT_IO)?"ON ":"OFF",S_MOON,GPIO_ALL_GET(ESC_IO)?"ON ":"OFF",_show?S_SUN:'#',_show?streetT/10:curT/10,_show?_temp:curT%10,S_DEG);
 lcdPrint(3,s);
 i++;
}

void showSplash(u8_t t){
 static u8_t c=0;
 char s[21];
 snprintf(s,21,"Starting %s",SENS.hostname);
 lcdPrint(0,s);
 snprintf(s,21,"    firmware v.%5s",FW_VER);
 lcdPrint(1,s);
 tcpip_adapter_ip_info_t ip;
 tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA,&ip);
 snprintf(s,21,"IP: %d.%d.%d.%d",IP2STR(&ip.ip));
 lcdPrint(2,s);
 c++;
 memset(s,0,21);
 memset(s,S_SPC,20);
 memset(s,S_BAR,(20*c)/t);
 lcdPrint(3,s);
 if(c==t) splash=0;
}

#define SETCHR(x,y){LCDI2C_createChar(x,y);}

void lcd_init2(){
 SETCHR(S_MOON,chrMoon);
 SETCHR(S_SUN,chrSun);
 SETCHR(S_VENT1,chrVent1);
 SETCHR(S_VENT2,chrVent2);
 SETCHR(S_SCHD1,chrSchd1);
 SETCHR(S_SCHD2,chrSchd2);
 SETCHR(S_BAR,chrBar);
}

void kotel1RatePage(){
 lcdPrint(0,"*** KOTEL1 RATE ****");
 char s[PAGE_KOTEL1_LINES][30];
 u32_t t=fuelRateNow;
 snprintf(s[0],21,"current    %3d.%03d L",t/100000,(t%100000)/100);
 t=fPumpOnDur;
 snprintf(s[1],21,"current     %02d:%02d:%02d",t/3600000,t/60000,t/1000%60);
 t=fuelRatePrev;
 snprintf(s[2],21,"   last    %3d.%03d L",t/100000,(t%100000)/100);
 t=fPumpOnDurPrev;
 snprintf(s[3],21,"   last     %02d:%02d:%02d",t/3600000,t/60000,t/1000%60);
 t=fuelRateToDay;
 snprintf(s[4],21,"  today      %3d.%03d L",t/100000,(t%100000)/100);
 t=fPumpWorkToDay;
 snprintf(s[5],21,"  today     %02d:%02d:%02d",t/3600,t/60,t%60);
 t=fuelRatePrevDay;
 snprintf(s[6],21,"   prev      %3d.%03d L",t/100000,(t%100000)/100);
 t=fPumpWorkPrevDay;
 snprintf(s[7],21,"   prev     %02d:%02d:%02d",t/3600,t/60,t%60);
 t=fuelRateTotal;
 snprintf(s[8],21,"  total    %3d.%03d L",t/100000,(t%100000)/100);
 t=fPumpWorkTotal;
 snprintf(s[9],21,"  total     %02d:%02d:%02d",t/3600,t/60,t%60);

 lcdPrint(1,s[pageKotel1Line]);
 lcdPrint(2,s[pageKotel1Line+1]);
 lcdPrint(3,s[pageKotel1Line+2]); 
}

void kotel2RatePage(){
 char s[30];
 lcdPrint(0,"*** KOTEL2 RATE ****");
 snprintf(s,21,"  today,kWh: %5d.%1d",0,0);
 lcdPrint(1,s);
 snprintf(s,21,"lastday,kWh: %5d.%1d",0,0);
 lcdPrint(2,s);
 snprintf(s,21,"  total,kWh: %5d.%1d",0,0);
 lcdPrint(3,s);
}

void workModePage(){
 char s[30];
 lcdPrint(0,"**** WORK MODE *****");
 memset(s,S_SPC,30);
 lcdPrint(1,s);
 lcdPrint(3,s);
 switch(workMode){
  case MODE_MANUAL:strcpy(s,"       MANUAL       ");break;
  case MODE_AUTO:strcpy(s,"        AUTO        ");break;
  case MODE_KOTEL1:strcpy(s,"       KOTEL1       ");break;
  case MODE_KOTEL2:strcpy(s,"       KOTEL2       ");break;
  default:strcpy(s,"      UNKNOWN       ");break; 
 }
 lcdPrint(2,s);
}

void schedulePage(){
 char s[30];
 lcdPrint(0,"***** SCHEDULE *****");
 memset(s,S_SPC,30);
 lcdPrint(1,s);
 lcdPrint(3,s);
 snprintf(s,21,"        %3s         ",schedule?" ON":"OFF");
 lcdPrint(2,s);
}

void tempsetPage(){
 char s[30];
 lcdPrint(0,"***** TEMPSET *****");
 memset(s,S_SPC,21);
 lcdPrint(1,s);
 snprintf(s,21,"Setpoint:      %2d.%1d%c",TEMPSET/10,TEMPSET%10,S_DEG);
 lcdPrint(2,s);
 snprintf(s,21,"Schedule:        %3s",schedule?" ON":"OFF");
 lcdPrint(3,s);
}

void hystPage(){
 char s[30];
 lcdPrint(0,"**** HYSTERESIS ****");
 memset(s,S_SPC,21);
 lcdPrint(1,s);
 lcdPrint(3,s);
 u32_t t=THERMO_HYSTERESIS(1);
 snprintf(s,21,"Hysteresis:    %2d.%1d%c",t/10,t%10,S_DEG);
 lcdPrint(2,s);
}

void pumpSettingsPage(){
 char s[30];
 lcdPrint(0,"*** PUMP SETTINGS **");
 char sm[13]="";
 u32_t t=PUMP_MODE;
 if(t==PUMP_MODE_NONE) strcat(sm,"manual off");
 else if(t==PUMP_MODE_1) strcat(sm,"relay off");
 else if(t==PUMP_MODE_2) strcat(sm,"delay off");
 else if(t==PUMP_MODE_3) strcat(sm,"T return off");

 static u32_t i=0;
 i++;
 if(!pumpOpt && pumpInMenu && !(i%2)) snprintf(s,21,"%sMode: %13s"," "," ");
 else snprintf(s,21,"%sMode: %13s",!pumpOpt?">":" ",sm);
 lcdPrint(1,s);
 t=THERMO_SETPOINT(3);
 if(pumpOpt==1 && pumpInMenu && !(i%2)) snprintf(s,21,"%sSetpoint:     %2s.%1s%c"," ","  "," ",S_DEG);
 else snprintf(s,21,"%sSetpoint:     %2d.%1d%c",pumpOpt==1?">":" ",t/10,t%10,S_DEG);
 lcdPrint(2,s);
 t=THERMO_HYSTERESIS(3);
 if(pumpOpt==2 && pumpInMenu && !(i%2)) snprintf(s,21,"%sHysteresis:   %2s.%1s%c"," ","  "," ",S_DEG);
 else snprintf(s,21,"%sHysteresis:   %2d.%1d%c",pumpOpt==2?">":" ",t/10,t%10,S_DEG);
 lcdPrint(3,s);
}

void versionPage(){
 char s[21];
 memset(s,S_SPC,21);
 snprintf(s,21,"Hostname:%11s",SENS.hostname);
 lcdPrint(0,s);
 memset(s,S_SPC,21);
 snprintf(s,21,"firmware     v.%5s",FW_VER);
 lcdPrint(1,s);
 snprintf(s,21,"FreeHeap: %10d",esp_get_free_heap_size());
 lcdPrint(2,s);
 memset(s,S_SPC,21);
 tcpip_adapter_ip_info_t ip;
 tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA,&ip);
 snprintf(s,21,"IP: %d.%d.%d.%d     ",IP2STR(&ip.ip));
 lcdPrint(3,s); 
}

void showPage(u8_t i){
 if(splash){showSplash(SPLASH_TIME);return;}
 switch(i){
  case PAGE_MAIN:pageMain();break;
  case PAGE_KOTEL1_RATE:kotel1RatePage();break;
  case PAGE_KOTEL2_RATE:kotel2RatePage();break;
  case PAGE_WORKMODE:workModePage();break;
  case PAGE_SCHEDULE:schedulePage();break;
  case PAGE_TEMPSET:tempsetPage();break;
  case PAGE_HYST:hystPage();break;
  case PAGE_PUMP_SETTINGS:pumpSettingsPage();break;
  case PAGE_VERSION:versionPage();break;
  case PAGE_MAX:pageMain();break;
  default:pageMain();break;
 } 
}

/* Thermo */
void decTempSet(u8_t v){
 TEMPSET-=TEMPSET_STEP;
 if(TEMPSET<TEMPSET_MIN) TEMPSET=TEMPSET_MIN;
 if(!v){
  THERMO_TEMP_SET(1,TEMPSET);
  THERMO_TEMP_SET(2,TEMPSET);
  SAVEOPT;
 }
}

void incTempSet(u8_t v){
 TEMPSET+=TEMPSET_STEP;
 if(TEMPSET>TEMPSET_MAX) TEMPSET=TEMPSET_MAX;
 if(!v){
  THERMO_TEMP_SET(1,TEMPSET);
  THERMO_TEMP_SET(2,TEMPSET);
  SAVEOPT;
 }
}

void decHyst(){
 u16_t h=THERMO_HYSTERESIS(1)-HYST_STEP;
 if(h<HYST_MIN) h=HYST_MIN;
 THERMO_HYST_SET(1,h);
 THERMO_HYST_SET(2,h);
}

void incHyst(){
 u16_t h=THERMO_HYSTERESIS(1)+HYST_STEP;
 if(h>HYST_MAX) h=HYST_MAX;
 THERMO_HYST_SET(1,h);
 THERMO_HYST_SET(2,h);
}

void decPumpMode(){
 if(PUMP_MODE) PUMP_MODE--;
}

void incPumpMode(){
 PUMP_MODE++;
 if(PUMP_MODE>=PUMP_MODE_MAX) PUMP_MODE=PUMP_MODE_MAX-1;
}

void decPumpTempSet(){
 u16_t t=THERMO_SETPOINT(3)-TEMPSET_STEP;
 if(t<TEMPSET_MIN) t=TEMPSET_MIN;
 THERMO_TEMP_SET(3,t);
 THERMO_TEMP_SET(4,t);
 SAVEOPT;
}

void incPumpTempSet(){
 u16_t t=THERMO_SETPOINT(3)+TEMPSET_STEP;
 if(t>TEMPSET_MAX) t=TEMPSET_MAX;
 THERMO_TEMP_SET(3,t);
 THERMO_TEMP_SET(4,t);
 SAVEOPT;
}

void decPumpHyst(){
 u16_t h=THERMO_HYSTERESIS(3)-HYST_STEP;
 if(h<HYST_MIN) h=HYST_MIN;
 THERMO_HYST_SET(3,h);
 THERMO_HYST_SET(4,h);
}

void incPumpHyst(){
 u16_t h=THERMO_HYSTERESIS(3)+HYST_STEP;
 if(h>HYST_MAX) h=HYST_MAX;
 THERMO_HYST_SET(3,h);
 THERMO_HYST_SET(4,h);
}

void setScheduleMode(u8_t m){
 static u8_t p=0;
 schedule=m;
 if(p!=schedule){
  ESP_LOGI("", "%s: schedule=%d, prev=%d", __func__, schedule, p);
  nvsSave32u(NVSNAME,PRM_SCHEDULE,&schedule);
 }
 p=schedule; 
}

void switchSchedule(){
 setScheduleMode(1-schedule);
 if(schedule) setTempSetBySchedule();
 else{THERMO_TEMP_SET(1,TEMPSET);THERMO_TEMP_SET(2,TEMPSET);SAVEOPT;}
}

int8_t schedule_id;

void setTempSetBySchedule(u8_t v){
 if(!v) return;
 u16_t t=TEMPSET;
 schedule_id=-1;
 for(u8_t i=0;i<maxscher;i++){
  if(BITCHK(SENS.schweek[i],time_loc.dow)){
   u16_t t1=SENS.scheduler[i][1]*60+SENS.scheduler[i][2];
   u16_t t2=time_loc.hour*60+time_loc.min;
   if(t2>=t1){
    t = SENS.scheduler[i][3];
    schedule_id=i;
   } 
  }
 }
 schedTempSet=t;
 THERMO_TEMP_SET(1,t);
 THERMO_TEMP_SET(2,t);
}

#define PRM_PUMP_MODE "pumpmode"

void setPumpMode(u8_t m){
 if(m>=PUMP_MODE_MAX || m==PUMP_MODE) return;
 PUMP_MODE=m;
}

void ctrlReturnWaterThermo(){
 u8_t m,s3,s4;
 m=PUMP_MODE;
 s3=THERMO_STATE(3);
 s4=THERMO_STATE(4);
 if(m==PUMP_MODE_NONE)return;
 if(m==PUMP_MODE_1){
  if(s3) THERMO_OFF(3);
  if(s4) THERMO_OFF(4);
  GPIO_ALL(PUMP1_IO,GPIO_ALL_GET(KOTEL1_IO));
  GPIO_ALL(PUMP2_IO,GPIO_ALL_GET(KOTEL2_IO));
 }else if(m==PUMP_MODE_2){
  if(s3) THERMO_OFF(3);
  if(s4) THERMO_OFF(4);
  if(GPIO_ALL_GET(KOTEL1_IO)) GPIO_ALL(PUMP1_IO,1);
  if(GPIO_ALL_GET(KOTEL2_IO)) GPIO_ALL(PUMP2_IO,1);
  static u32_t t1,t2=0;
  if(!GPIO_ALL_GET(KOTEL1_IO)&&(millis()-t1>PUMP_OFF_TIMEOUT)){GPIO_ALL(PUMP1_IO,0);t1=millis();}
  else if(GPIO_ALL_GET(KOTEL1_IO)) t1=millis();
  if(!GPIO_ALL_GET(KOTEL2_IO)&&(millis()-t2>PUMP_OFF_TIMEOUT)){GPIO_ALL(PUMP2_IO,0);t2=millis();}
  else if(GPIO_ALL_GET(KOTEL2_IO)) t2 = millis();  
 }else if(m==PUMP_MODE_3){
  if(GPIO_ALL_GET(KOTEL1_IO)&&!s3) THERMO_ON(3);
  if(GPIO_ALL_GET(KOTEL2_IO)&&!s4) THERMO_ON(4);
  if(!GPIO_ALL_GET(KOTEL1_IO)){
   if(!GPIO_ALL_GET(PUMP1_IO)||GPIO_ALL_GET(KOTEL2_IO)) THERMO_OFF(3);
  }
  if(!GPIO_ALL_GET(KOTEL2_IO)){
   if(!GPIO_ALL_GET(PUMP2_IO)||GPIO_ALL_GET(KOTEL1_IO)) THERMO_OFF(4);
  }  
 }
}

void setActiveKotel(mode_e m){
 switch(m){
  case MODE_MANUAL:activeKotel=0;break;
  case MODE_AUTO:activeKotel=(time_loc.hour>=DAY_TIME&&time_loc.hour<NIGHT_TIME)?1:2;break;
  case MODE_KOTEL1:activeKotel=1;break;
  case MODE_KOTEL2:activeKotel=2;break;
  default:activeKotel=0;
 }
 if(m!=MODE_MANUAL){
  GPIO_ALL(100,activeKotel==1);
  if(activeKotel!=1) GPIO_ALL(KOTEL1_IO,0);
  GPIO_ALL(101,activeKotel==2);
  if(activeKotel!=2) GPIO_ALL(KOTEL2_IO,0);
 }else{
  GPIO_ALL(100,0);
  GPIO_ALL(101,0);
 }
}

void setWorkMode(u8_t m){
 if(m>=MODE_MAX) return;
 static u8_t p=0;
 workMode=m;
 if(p!=workMode){
  ESP_LOGI("", "%s: workMode=%d, p=%d", __func__, workMode, p);
  nvsSave32u(NVSNAME,PRM_WORKMODE,&m);
 }
 p=workMode;
 setActiveKotel(workMode);
}

void changeWorkMode(){
 if(!displayAlert){
  workMode++;
  if(workMode>=MODE_MAX) workMode=MODE_MANUAL;
  setWorkMode(workMode);
 }
}

void changeWorkModeBack(){
 if(!displayAlert){
  if(workMode==MODE_MANUAL) workMode=MODE_MAX;
  workMode--;
  setWorkMode(workMode);
 }
}

/* MCP23017 */
#define GPINTENA 4
#define DEFVALA 6
#define INTCONA 8
#define INTFA 14
#define INTCAPA 16

#define MCP23017_INTA_PIN 4
TaskHandle_t mcpTask;
QueueHandle_t mcpQueue;

typedef void (*intr_cb)(void *a,u8_t *s);

typedef struct mcpPinISR{
 u8_t pin;
 intr_cb pin_cb;
 void *args;
 intr_cb pin_cb2;
 void *args2;
 gpio_int_type_t intrType; 
 u32_t upDelay;
 u32_t downTs;
 u32_t upTs;
} mcpPinISR_t;

mcpPinISR_t *pinISR;
u8_t pinISR_cnt;

static void IRAM_ATTR mcpISRHandler(void *a){
 BaseType_t x=pdFALSE;
 u16_t data[2];
 data[0]=MCPread_reg16(0,INTFA);
 data[1]=MCPread_reg16(0,INTCAPA);
 static u32_t t=0;
 if(millis()-t>=MCP23017_ISR_DELAY_MS){xQueueOverwriteFromISR(mcpQueue,&data,&x);t=millis();}
 portEND_SWITCHING_ISR(x);
}

static void mcpISRCb(void *a){
 while(1){
  u16_t data[2];
  if(xQueueReceive(mcpQueue,&data,0)){
   for(u8_t i=0;i<16;i++){
    if(BITCHK(data[0],i)){
     u8_t st=BITCHK(data[1],i)>0;
     for(u8_t j=0;j<pinISR_cnt;j++){
      if(pinISR[j].pin==i){
       if((!st && pinISR[j].intrType==GPIO_INTR_POSEDGE) || (st && pinISR[j].intrType==GPIO_INTR_NEGEDGE)){pinISR[j].downTs=pinISR[j].upTs=millis();}
       if((st && pinISR[j].intrType==GPIO_INTR_POSEDGE) || (!st && pinISR[j].intrType==GPIO_INTR_NEGEDGE)){
        pinISR[j].upTs=millis();
        if((pinISR[j].upTs-pinISR[j].downTs<=pinISR[j].upDelay)||!pinISR[j].pin_cb2){pinISR[j].pin_cb(pinISR[j].args,&st);}
        else if(pinISR[j].pin_cb2) pinISR[j].pin_cb2(pinISR[j].args2,&st);
       }
       else if(pinISR[j].intrType==GPIO_INTR_ANYEDGE) pinISR[j].pin_cb(pinISR[j].args,&st);
      }
     } 
    }
   }
  }
  vTaskDelay(VPAUSE(10));
 }
 vTaskDelete(NULL);
}

esp_err_t mcpISRHandlerAdd(u8_t p,gpio_int_type_t it,intr_cb cb,void *a,intr_cb cb2,void *a2,u32_t d){
 if(!cb) return ESP_FAIL;
 pinISR_cnt++;
 pinISR=(mcpPinISR_t *)realloc(pinISR,pinISR_cnt*sizeof(mcpPinISR_t));
 if(!pinISR){pinISR_cnt--;return ESP_FAIL;}
 u8_t i=pinISR_cnt-1;
 pinISR[i].pin=p;
 pinISR[i].pin_cb=cb;
 pinISR[i].intrType=it;
 pinISR[i].args=a;
 pinISR[i].pin_cb2=cb2;
 pinISR[i].args2=a2;
 pinISR[i].upDelay=d;
 return ESP_OK;  
}

void shortPressBtn1(void *a,u8_t *s){
 u8_t b=BACKLIGHT_STATE;
 backlightTurnOn(BACKLIGHT_IO,NULL);
 if(!b&&SENS.lcden) return;
 buzzer(BEEP_SHORT);
 switch(mId){
  case PAGE_MAIN:
   if(workMode!=MODE_MANUAL){
    if(schedule){buzzer(BEEP_ERROR);showDisplayAlert("   *** ERROR ***    ","Schedule is enabled. Can't change temperature setpiont!",NULL);}
    else{decTempSet(schedule);showDisplayAlert(NULL,NULL,tempsetPage);}
   }else GPIO_INVERT(KOTEL1_IO);
   break;
  case PAGE_KOTEL1_RATE:if(pageKotel1Line) pageKotel1Line--;else buzzer(BEEP_ERROR);break;
  case PAGE_KOTEL2_RATE:break;
  case PAGE_WORKMODE:changeWorkModeBack();break;
  case PAGE_SCHEDULE:switchSchedule();break;
  case PAGE_TEMPSET:decTempSet(schedule);break;
  case PAGE_HYST:decHyst();break;
  case PAGE_PUMP_SETTINGS:
   if(!pumpInMenu){pumpOpt++;if(pumpOpt>LCD_PUMP_OPTIONS-1) pumpOpt=0;} 
   else{
    if(pumpOpt==0) decPumpMode();
    else if(pumpOpt==1) decPumpTempSet();
    else if(pumpOpt==2) decPumpHyst();
   }
   break;
  case PAGE_VERSION:break;
  default:break;
 }
 lastPress=millis();
}

void longPressBtn1(void *a,u8_t *s){
 backlightTurnOn(BACKLIGHT_IO,NULL);
 buzzer(BEEP_DOUBLE_SHORT);
 if(mId==PAGE_MAIN) changeWorkMode();
 else if(mId==PAGE_KOTEL1_RATE) resetFuel();
 lastPress=millis();
}

void shortPressBtn2(u8_t p,u8_t *s){
 u8_t b=BACKLIGHT_STATE;
 backlightTurnOn(BACKLIGHT_IO,NULL);
 if(!b&&SENS.lcden)return;
 buzzer(BEEP_SHORT);
 switch(mId){
  case PAGE_MAIN:
   if(workMode!=MODE_MANUAL){
    if(schedule){buzzer(BEEP_ERROR);showDisplayAlert("   *** ERROR ***    ","Schedule is enabled. Can't change temperature setpiont!",NULL);}
    else{incTempSet(schedule);showDisplayAlert(NULL,NULL,tempsetPage);}
   }else GPIO_INVERT(KOTEL2_IO);
   break;
  case PAGE_KOTEL1_RATE:pageKotel1Line++;if(pageKotel1Line==(PAGE_KOTEL1_LINES-2)){pageKotel1Line=0;buzzer(BEEP_ERROR);}break;
  case PAGE_KOTEL2_RATE:break;
  case PAGE_WORKMODE:changeWorkMode();break;
  case PAGE_SCHEDULE:switchSchedule();break;
  case PAGE_TEMPSET:incTempSet(schedule);break;
  case PAGE_HYST:incHyst();break;
  case PAGE_PUMP_SETTINGS:if(pumpInMenu){if (pumpOpt==0)incPumpMode();else if(pumpOpt==1)incPumpTempSet();else if(pumpOpt==2)incPumpHyst();}break;
  case PAGE_VERSION:break;
  default:break;
 }
 lastPress=millis();
}

void longPressBtn2(u8_t p,u8_t *s){
 backlightTurnOn(BACKLIGHT_IO,NULL);
 buzzer(BEEP_DOUBLE_SHORT);
 if(mId==PAGE_MAIN){
  GPIO_INVERT(ESC_IO);
  char s[21];
  snprintf(s,21,"  Night mode is %3s ",GPIO_ALL_GET(ESC_IO)?"ON ":"OFF");
  showDisplayAlert("                    ",s,NULL);
 }
 lastPress=millis();
}

void shortPressBtn3(u8_t p,u8_t *s){
 backlightTurnOn(BACKLIGHT_IO,NULL);
 buzzer(BEEP_SHORT);
 if(mId==PAGE_MAIN){
  GPIO_INVERT(VENT_IO);
  char s[21];
  snprintf(s,21," Ventilation is %3s ",GPIO_ALL_GET(VENT_IO)?"ON ":"OFF");
  showDisplayAlert("                    ",s,NULL);
 }else if(mId==PAGE_PUMP_SETTINGS && !pumpInMenu){pumpOpt=0;prevMenu();}
 lastPress=millis();
}

void longPressBtn3(u8_t p,u8_t *s){
 backlightTurnOn(BACKLIGHT_IO,NULL);
 buzzer(BEEP_DOUBLE_SHORT);
 if(mId==PAGE_MAIN){
  switchSchedule();
  char s[21];
  snprintf(s,21,"   Schedule is %3s  ",schedule?"ON ":"OFF");
  showDisplayAlert("                    ",s,NULL);
 }else if(mId==PAGE_PUMP_SETTINGS) pumpInMenu=1-pumpInMenu;
 lastPress=millis();
}

void shortPressBtn4(u8_t p,u8_t *s){
 backlightTurnOn( BACKLIGHT_IO,NULL);
 if(mId==PAGE_PUMP_SETTINGS && pumpInMenu)
 {}else{pumpOpt = 0;nextMenu();}
 buzzer(BEEP_SHORT);
 lastPress=millis();
}

void longPressBtn4(u8_t p,u8_t *s){
 backlightTurnOn(BACKLIGHT_IO,NULL);
 buzzer(BEEP_DOUBLE_SHORT);
 if(mId==PAGE_MAIN){GPIO_ALL(PUMP1_IO,0);GPIO_ALL(PUMP2_IO,0);}
 else{buzzer(BEEP_DOUBLE_SHORT);mId=PAGE_MAIN;}
 lastPress=millis();
}

void mcpInit(){
 MCPwrite_reg16(0,GPINTENA,0xFFFF);
 MCPwrite_reg16(0,INTCONA,0);
 MCPwrite_reg16(0,DEFVALA,0xFFFF); 
 gpio_config_t gpio_conf;
 gpio_conf.intr_type=GPIO_INTR_NEGEDGE;
 gpio_conf.mode=GPIO_MODE_INPUT;
 gpio_conf.pull_down_en=GPIO_PULLDOWN_DISABLE;
 gpio_conf.pull_up_en=GPIO_PULLUP_DISABLE;
 gpio_conf.pin_bit_mask=1ULL<<MCP23017_INTA_PIN;
 gpio_config(&gpio_conf);
 gpio_install_isr_service(0);
 gpio_isr_handler_add(MCP23017_INTA_PIN,mcpISRHandler,NULL);
 mcpISRHandlerAdd(0,GPIO_INTR_POSEDGE,shortPressBtn1,NULL,longPressBtn1,NULL,800);
 mcpISRHandlerAdd(1,GPIO_INTR_POSEDGE,shortPressBtn2,NULL,longPressBtn2,NULL,800);
 mcpISRHandlerAdd(2,GPIO_INTR_POSEDGE,shortPressBtn3,NULL,longPressBtn3,NULL,800);
 mcpISRHandlerAdd(3,GPIO_INTR_POSEDGE,shortPressBtn4,NULL,longPressBtn4,NULL,800);
 mcpQueue=xQueueCreate(5,sizeof(u16_t)*2);
 xTaskCreate(mcpISRCb,"mcpisr",1024,NULL,10,&mcpTask);
}

void controlIndications(){
 if(workMode!=MODE_MANUAL){GPIO_ALL(KOTEL1_LED_IO,THERMO_STATE(1));GPIO_ALL(KOTEL2_LED_IO,THERMO_STATE(2));}
 else{GPIO_ALL(KOTEL1_LED_IO,GPIO_ALL_GET(KOTEL1_IO));GPIO_ALL(KOTEL2_LED_IO,GPIO_ALL_GET(KOTEL2_IO));}
 GPIO_ALL(KOTEL_LED_IO,!GPIO_ALL_GET(KOTEL1_IO)&&!GPIO_ALL_GET(KOTEL2_IO));
 GPIO_ALL(PUMP_LED_IO,!GPIO_ALL_GET(PUMP1_IO)&&!GPIO_ALL_GET(PUMP2_IO));
}

#define html_button_mode "<td><a href='#' onclick='wm(%d)'><div class='g_%d k kk fll wm' id='v%d'>%s</div></a></td>"

void printKotel(char *pbuf){
 os_sprintf(HTTPBUFF,"<table><tr><td>Temperature:"TDE"<b>%d.%d °C</b></td></tr><tr><td>Mode:</td>",curT/10,curT%10);
 os_sprintf(HTTPBUFF,html_button_mode,MODE_MANUAL,workMode==MODE_MANUAL,MODE_MANUAL,"Manual");
 os_sprintf(HTTPBUFF,html_button_mode,MODE_AUTO,workMode==MODE_AUTO,MODE_AUTO,"Auto");
 os_sprintf(HTTPBUFF,html_button_mode,MODE_KOTEL1,workMode==MODE_KOTEL1,MODE_KOTEL1,"Kotel1");
 os_sprintf(HTTPBUFF,html_button_mode,MODE_KOTEL2,workMode==MODE_KOTEL2,MODE_KOTEL2,"Kotel2");
 os_sprintf(HTTPBUFF,"</tr><tr><td>Schedule:</td><td><a id='ushd' href='#' data-val='%d' onclick='schd(this.dataset.val)'><div class='g_%d k kk fll' id='sch' data-text='%s'>%s</div></a></td>",!schedule,schedule,schedule?"Off":"On",schedule?"On":"Off");
 os_sprintf(HTTPBUFF,"<td colspan=2 align=right>%s tempset:</td><td><b>%d.%d °C</b></td></tr>",schedule?"Schedule":"Global",schedule?schedTempSet/10:TEMPSET/10,schedule?schedTempSet%10:TEMPSET%10);
 if(schedule){
  char w[32]="";
  if(schedule_id>-1 && schedule_id<maxscher){
   sprintf(w," #%d %02d:%02d %s%s%s%s%s%s%s",schedule_id+1,SENS.scheduler[schedule_id][1],SENS.scheduler[schedule_id][2],BITCHK(SENS.schweek[schedule_id],0)?"Mo ":""
    ,BITCHK(SENS.schweek[schedule_id],1)?"Tu ":"",BITCHK(SENS.schweek[schedule_id],2)?"We ":"",BITCHK(SENS.schweek[schedule_id],3)?"Th ":""
    ,BITCHK(SENS.schweek[schedule_id],4)?"Fr ":"",BITCHK(SENS.schweek[schedule_id],5)?"Sa ":"",BITCHK(SENS.schweek[schedule_id],6)?"Su ":"");
  }
  os_sprintf(HTTPBUFF,"<tr><td colspan=5 align=right>[%s]</td></tr>",w);
 }
 os_sprintf(HTTPBUFF,"</table>");
}

void printScript(char *pbuf){
	os_sprintf(HTTPBUFF,"<script type='text/javascript'>window.onload=function(){let e=document.createElement('style');e.innerText='.kk{border-radius:4px;margin:2px;width:60px;}';document.head.appendChild(e)};"
						"function wm(t){ajax_request('/valdes?int=%d&set='+t,function(res){let v=document.getElementsByClassName('wm');for(let i=0;i<v.length;i++)v[i].classList.replace('g_1','g_0');document.getElementById('v'+t).classList.add('g_1')})};"
						"function schd(t){ajax_request('/valdes?int=%d&set='+t,function(res){var n=1-parseInt(t);var sc=document.getElementById('sch');sc.classList.replace('g_'+n,'g_'+t);sc.innerHTML=sc.getAttribute('data-text');"
										"document.getElementById('ushd').setAttribute('data-val',n);})}"
						"</script>",IDX_WORKMODE+1,IDX_SCHED+1);
}


#define ADDLISTSENS {200,LSENSFL0,"WorkMode",PRM_WORKMODE,&workMode,NULL},\
{201,LSENSFL1,"Temperature","temp",&curT,NULL},\
{202,LSENSFL0,"Schedule",PRM_SCHEDULE,&schedule,NULL},\
{203,LSENSFL0,"TempSet",PRM_TEMPSET,&TEMPSET,NULL},\
{204,LSENSFL0,"PumpMode",PRM_PUMP_MODE,&PUMP_MODE,NULL},\
{205,LSENSFL0,"FuelPump","fuelpump",&fPumpSt,NULL},\
{206,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelRate","fuelrate",getRateTotal,NULL},\
{207,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelRateT","fuelratet",getRateDay,NULL},\
{208,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelRateY","fuelratey",getRatePrev,NULL},\
{209,LSENSFL0|LSENS32BIT,"FuelTime","fueltime",&fPumpWorkTotal,NULL},\
{210,LSENSFL0|LSENS32BIT,"FuelTimeT","fueltimet",&fPumpWorkToDay,NULL},\
{211,LSENSFL0|LSENS32BIT,"FuelTimeY","fueltimey",&fPumpWorkPrevDay,NULL},\
{212,LSENSFL0,"FuelOnCnt","foncnt",&fPumpOnCnt,NULL},\
{213,LSENSFL0|LSENS32BIT,"FuelOnDur","fondur",&fPumpOnDurPrev,NULL},

void receiveMqtt(char *topicBuf,char *dataBuf){
 char lwt[64];
 u16_t lentopic=os_sprintf(lwt,"%s/%s" topicwrite "/",SENS.mqttlogin,SENS.hostname);
 char *topic=(char *)os_strstr(topicBuf,lwt);
 if(!topic)return;
 topic+=lentopic;
 if(!strcoll(topic,PRM_WORKMODE)){
  u8_t m=atoi(dataBuf);
  if(m==workMode)return;
  setWorkMode(m);
  nvsSave32u(NVSNAME,PRM_WORKMODE,&workMode);
 }else if(!strcoll(topic,PRM_SCHEDULE)){
  u8_t sc=atoi(dataBuf);
  if(sc==schedule) return;
  if(sc>1) switchSchedule();
  else{
   setScheduleMode(sc);
   if(schedule) setTempSetBySchedule(schedule);
   else{THERMO_TEMP_SET(1,TEMPSET);THERMO_TEMP_SET(2, TEMPSET);SAVEOPT;}
  }
  nvsSave32u(NVSNAME,PRM_SCHEDULE,&schedule);
 }else if(!strcoll(topic,PRM_TEMPSET)){ 
  if(strchr(dataBuf,'.')==NULL) TEMPSET=atoi(dataBuf);
  else TEMPSET=(u16_t)(atof(dataBuf)*10);
  if(!schedule){THERMO_TEMP_SET(1,TEMPSET);THERMO_TEMP_SET(2,TEMPSET);SAVEOPT;} 
 }else if(!strcoll(topic,PRM_PUMP_MODE)) setPumpMode(atoi(dataBuf));
}

void startfunc(){
 if(nvsLoad(NVSNAME,PRM_WORKMODE,&workMode)!=ESP_OK) workMode=MODE_MANUAL;
 if(nvsLoad(NVSNAME,PRM_SCHEDULE,&schedule)!= ESP_OK) schedule=0;
 u8_t e=0;
 if(!KOTEL1_IO||KOTEL1_IO>=255){KOTEL1_IO=KOTEL1_IO_D;e=1;}
 if(!KOTEL2_IO||KOTEL2_IO>=255){KOTEL2_IO=KOTEL2_IO_D;e=1;}
 if(!PUMP1_IO||PUMP1_IO>=255){PUMP1_IO=PUMP1_IO_D;e=1;}
 if(!PUMP2_IO||PUMP2_IO>=255){PUMP2_IO=PUMP2_IO_D;e=1;}
 if(!ESC_IO||ESC_IO>=255){ESC_IO=ESC_IO_D;e=1;}
 if(!VENT_IO||VENT_IO>=255){VENT_IO=VENT_IO_D;e=1;}
 if(NIGHT_TIME>=23){NIGHT_TIME=NIGHT_TIME_D;e=1;}
 if(DAY_TIME>=23){DAY_TIME=DAY_TIME_D;e=1;}
 if(!KOTEL1_LED_IO||KOTEL1_LED_IO>=255){KOTEL1_LED_IO=255;e= 1;}
 if(!KOTEL2_LED_IO||KOTEL2_LED_IO>=255){KOTEL2_LED_IO=255;e= 1;}
 if(!KOTEL_LED_IO||KOTEL_LED_IO>=255){KOTEL_LED_IO=255;e=1;}
 if(!PUMP_LED_IO||PUMP_LED_IO>=255){PUMP_LED_IO=255;e=1;}
 if(!SCHEDULE_LED_IO||SCHEDULE_LED_IO>=255){SCHEDULE_LED_IO=255;e=1;}
 if(!VENT_LED_IO||VENT_LED_IO>=255){VENT_LED_IO=255;e= 1;}
 if(TEMPSET<100||TEMPSET>300){TEMPSET=240;e=1;}
 if(!BUZZER_IO||BUZZER_IO>=255){BUZZER_IO=255;e=1;}
 if(PUMP_MODE>=PUMP_MODE_MAX){PUMP_MODE=PUMP_MODE_1;e=1;}
 if(e) SAVEOPT;
 buzzInit();
 mcpInit();
 setWorkMode(workMode);
 lcd_init2();
 backlightTurnOn(BACKLIGHT_IO,NULL);
 loadFuel();
 cb_mqtt_funs=receiveMqtt;
}

void timerfunc(u32_t t){
 if(t%5==0){
  setWorkMode(workMode);
  setScheduleMode(schedule);
 }
 if(t%30==0){setActiveKotel(workMode);if(workMode>=MODE_MAX)workMode=MODE_MANUAL;}
 if(mId!=PAGE_MAIN&&(millis()-lastPress>=MENU_TIMEOUT)){mId=PAGE_MAIN;pumpInMenu=pumpOpt=0;}
 if(t%5==0)setTempSetBySchedule(schedule);
 showPage(mId);
 ctrlReturnWaterThermo();
 GPIO_ALL(VENT_LED_IO,GPIO_ALL_GET(VENT_IO));
 GPIO_ALL(SCHEDULE_LED_IO,schedule);
 controlIndications();
 detectFuelWork();
 calcFuelRate();
 if(t%1800==0) saveFuel();
 if(reset_fuel){resetFuel();reset_fuel=0;}
 vTaskDelay(VPAUSE(1000));
}

void webfunc(char *pbuf){
 printKotel(pbuf);
 char s[45]="Ручное управление";
 u16_t t1=THERMO_SETPOINT(3)-THERMO_HYSTERESIS(3);
 u16_t t2=THERMO_SETPOINT(4)-THERMO_HYSTERESIS(4);
 switch(PUMP_MODE){
  case PUMP_MODE_1:strncpy(s,45,"по реле котла");break;    
  case PUMP_MODE_2:snprintf(s,45,"через %d сек после выкл котла",PUMP_OFF_TIMEOUT/1000);break;    
  case PUMP_MODE_3:snprintf(s,45,"по T-обратки #1 %d.%d° #2 %d.%d°",t1/10,t1%10,t2/10,t2%10);break;  
  default:break;  
 }

 os_sprintf(HTTPBUFF,"<br>Режим насоса: %d. Выкл: %s</small>",PUMP_MODE, s);
 printScript(pbuf);
 printFuelPump(pbuf);
 os_sprintf(HTTPBUFF,"<br><small>Version: %s</small>",FW_VER); 
}