#define FW_VER "2.3"

/*
Options:
Fan Speed 0-3,
---
Fan Speed: 0-3 fan speed
---
ValdesCount: 1
*/

/*
 defined to 1 - own wifi-iot pwm methods
 not defined - rtos pwm methods
*/
#define PWM_TYPE 1

#define FAN_CTRL_TASK_DELAY 100

#define FAN_SPEED1_GPIO 211
#define FAN_SPEED2_GPIO 212
#define FAN_SPEED3_GPIO 213

#define SENS sensors_param
#define SENSCFG SENS.cfgdes

#define FAN_SPEED_CFG       SENSCFG[0] // fan speed

#define FAN_SPEED_VALDES_IDX 0
#define FAN_SPEED       valdes[FAN_SPEED_VALDES_IDX] // fan speed

uint8_t needSave = 0;

#define ADDLISTSENS {200,LSENSFL0,"FanSpeed","fanspeed",&FAN_SPEED,NULL},\

static int32_t prevFanSpeedCfg = 0;

os_timer_t fanControlTmr; 
os_timer_t fadeDutyTmr; 

#define BRIGHTNESS_STEPS 32
static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,5,8,12,16,21,26,32,38,45,52,60,68,76,85,95,105,115,125,136,148,160,172,185,198,211,225,239,255};

#define BRIGHTNESS_STEP_DELAY 40 
static uint8_t light_delay = BRIGHTNESS_STEP_DELAY; // msec, длительность свечения каждой ступени яркости

typedef struct {
    int ch, 
    int fromDuty, 
    int targetDuty
} pwm_data_t;

void ICACHE_FLASH_ATTR changeFanSpeed(int32_t speed) {
    GPIO_ALL(FAN_SPEED1_GPIO, speed==1);
    GPIO_ALL(FAN_SPEED2_GPIO, speed==2);
    GPIO_ALL(FAN_SPEED3_GPIO, speed==3);
}

int ICACHE_FLASH_ATTR getNextMinDutyIdx(int duty){
	uint8_t i;
	for ( i = BRIGHTNESS_STEPS-1; i >= 0 ; i--){
		if ( brightness[i] > duty ) continue;
		else break;
	}	
    return i;
}

int ICACHE_FLASH_ATTR getNexMaxtDutyIdx(int duty){
	uint8_t i;
	for ( i = 0; i < BRIGHTNESS_STEPS; i++){
		if ( brightness[i] <= duty ) continue;
		else break;
	}
    if (i == BRIGHTNESS_STEPS) i--;
    return i;
}

void ICACHE_FLASH_ATTR updateDuty(int ch, int duty){
#ifdef PWM_TYPE     
    PWM_ALL_SET(ch, duty, 0);
#else    
    pwm_set_duty_iot(duty, ch);
    pwm_start_iot();
#endif    
}

void ICACHE_FLASH_ATTR  fadeUp(pwm_data_t *pwm_data){
    int idx_from = getNexMaxtDutyIdx(pwm_data->fromDuty);
    int idx_to = getNexMaxtDutyIdx(pwm_data->targetDuty);
    int _duty = 0;
    for (int i = idx_from; i <= idx_to; i++) {
        #ifdef PWM_TYPE
            _duty = brightness[i];
        #else
            _duty = pwm_data->targetDuty;
        #endif
        updateDuty(ch, _duty);
    }
    os_timer_disarm(&fadeDutyTmr);
}

void ICACHE_FLASH_ATTR fadeDown(pwm_data_t *pwm_data){
    int idx_from = getNextMinDutyIdx(pwm_data->fromDuty);
    int idx_to = getNextMinDutyIdx(pwm_data->targetDuty);
    int _duty = 0;
    for (int i = idx_from; i >= idx_to; i--) {
        #ifdef PWM_TYPE
            _duty = brightness[i];
        #else
            _duty = pwm_data->targetDuty;
        #endif
        updateDuty(ch, _duty);
    } 
    os_timer_disarm(&fadeDutyTmr);
}


void ICACHE_FLASH_ATTR fadeChannel(uint8_t ch, int duty){
    int fromDuty = pwm_get_duty_iot( ch );
    pwm_data_t pwm_data;
    pwm_data.ch = ch;
    pwm_data.fromDuty = fromDuty;
    pwm_data.targetDuty = duty;

    os_timer_disarm(&fadeDutyTmr);
    
    if ( duty > fromDuty) {
        os_timer_setfn(&fadeDutyTmr, (os_timer_func_t *) fadeUp, &pwm_data);
    } else {
        os_timer_setfn(&fadeDutyTmr, (os_timer_func_t *) fadeDown, &pwm_data);
    }

    os_timer_arm(&fadeDutyTmr, BRIGHTNESS_STEP_DELAY, 1);
}

void ICACHE_FLASH_ATTR mqttSend(const char *topic, int32_t val){
    char payload[20];
	memset(payload, 0, 20);
	os_sprintf(payload,"%d", val);
	MQTT_Publish(&mqttClient, topic, payload, os_strlen(payload), 2, 0, 0);
}

void ICACHE_FLASH_ATTR receiveMqtt(char *topicBuf,char *dataBuf){
    char lwt[64];
    uint16_t lentopic = os_sprintf(lwt, "%s/%s" topicwrite "/", SENS.mqttlogin, SENS.hostname);
    char *topic = (char *)os_strstr(topicBuf, lwt);
    if (!topic) return;
    topic += lentopic;
    if ( !strcoll(topic, "fanspeed")) {
        int32_t m = atoi(dataBuf);
        if (FAN_SPEED == m) return;
        FAN_SPEED = m;
    } else if ( strstr(topic, "fade") != NULL ) {
        char *istr;
        istr = (char *)os_strstr(topic, "fade");     
        char ch[3];
        strcpy(ch, istr + 4);
        
        if (ch) {
            uint8_t channel = atoi(ch);
            int32_t duty = atoi(dataBuf);
            if ( duty > 255 ) duty = 255;
            if (channel < SENS.pwmc) {
                fadeChannel(channel, duty);
            }
        }
    }    
}

void ICACHE_FLASH_ATTR fanControlTask(void *args){
    uint8_t changed = 0;
    if ( prevFanSpeedCfg != FAN_SPEED_CFG ) {
        prevFanSpeedCfg = FAN_SPEED_CFG;
        FAN_SPEED = FAN_SPEED_CFG;
        changed = 1;
    }
    if (FAN_SPEED_CFG != FAN_SPEED) {
        FAN_SPEED_CFG = FAN_SPEED;
        prevFanSpeedCfg = FAN_SPEED;
        changed = 1;
    }
    if ( changed ) {
        changeFanSpeed(FAN_SPEED);
        needSave = 1;
        changed = 0;
    }
}

void ICACHE_FLASH_ATTR startfunc(){
    // выполняется один раз при старте модуля.
    prevFanSpeedCfg = FAN_SPEED_CFG;
    FAN_SPEED = FAN_SPEED_CFG;
    changeFanSpeed(FAN_SPEED);

	os_timer_disarm(&fanControlTmr);
	os_timer_setfn(&fanControlTmr, (os_timer_func_t *) fanControlTask, NULL);
	os_timer_arm(&fanControlTmr, FAN_CTRL_TASK_DELAY, 1);

    cb_mqtt_funs=receiveMqtt;
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду
    if (needSave) {
        mqttSend("fanspeed", FAN_SPEED);
        SAVEOPT;
        needSave = 0;
    }

    if (timersrc%30 == 0) {
        // выполнение кода каждые 30 секунд

    }
}

#define HTML_BUTTON_SPEED "<td><a href='#' onclick='wm(%d)'><div class='g_%d k kk fll wm' id='v%d'>%s</div></a></td>"
void printFanControl(char *pbuf) {
    os_sprintf(HTTPBUFF, "<table><tr>");
    os_sprintf(HTTPBUFF,"<td><b>Fan Speed:</b> </td>");
    os_sprintf(HTTPBUFF, HTML_BUTTON_SPEED, 0, FAN_SPEED == 0, 0, "OFF");
    os_sprintf(HTTPBUFF, HTML_BUTTON_SPEED, 1, FAN_SPEED == 1, 1, "Speed 1");
    os_sprintf(HTTPBUFF, HTML_BUTTON_SPEED, 2, FAN_SPEED == 2, 2, "Speed 2");
    os_sprintf(HTTPBUFF, HTML_BUTTON_SPEED, 3, FAN_SPEED == 3, 3, "Speed 3");
    os_sprintf(HTTPBUFF, "</tr></table>");
    printScript(pbuf);
}

void printScript(char *pbuf) {
	os_sprintf(HTTPBUFF,"<script type='text/javascript'>window.onload=function(){let e=document.createElement('style');e.innerText='.kk{border-radius:4px;margin:2px;width:60px;}';document.head.appendChild(e)};"
						"function wm(t){ajax_request('/valdes?int=%d&set='+t,function(res){let v=document.getElementsByClassName('wm');for(let i=0;i<v.length;i++)v[i].classList.replace('g_1','g_0');document.getElementById('v'+t).classList.add('g_1')})};"
						"</script>",FAN_SPEED_VALDES_IDX+1);
}

void webfunc(char *pbuf) {
    printFanControl(pbuf);
    os_sprintf(HTTPBUFF,"<br><small>Version: %s</small>",FW_VER); 
}