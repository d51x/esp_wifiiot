#define FW_VER "3.0"

/*
Options:
Fan Speed 0-3,
---
Fan Speed: 0-3 fan speed
---
ValdesCount: 1
*/
#define FAN_CTRL_TASK_DELAY 200

#define FAN_SPEED1_GPIO 211
#define FAN_SPEED2_GPIO 212
#define FAN_SPEED3_GPIO 213

#define SENS sensors_param
#define SENSCFG SENS.cfgdes

#define FAN_SPEED_CFG       SENSCFG[0] // fan speed

#define FAN_SPEED_VALDES_IDX 0
#define FAN_SPEED       valdes[FAN_SPEED_VALDES_IDX] // fan speed

#define millis() (unsigned long)(esp_timer_get_time()/1000ULL)
#define pauseTask(delay)  (vTaskDelay(delay / portTICK_PERIOD_MS))

uint8_t needSave = 0;

#define ADDLISTSENS {200,LSENSFL0,"FanSpeed","fanspeed",&FAN_SPEED,NULL},\

static int32_t prevFanSpeedCfg = 0;


#define BRIGHTNESS_STEPS 32
static uint8_t brightness[BRIGHTNESS_STEPS] = {0,1,2,3,5,8,12,16,21,26,32,38,45,52,60,68,76,85,95,105,115,125,136,148,160,172,185,198,211,225,239,255};

#define BRIGHTNESS_STEP_DELAY 40 
static uint8_t light_delay = BRIGHTNESS_STEP_DELAY; // msec, длительность свечения каждой ступени яркости

void mqttSend(const char *topic, int32_t val){
    char payload[20];
	memset(payload, 0, 20);
	os_sprintf(payload,"%d", val);
	MQTT_Publish(topic, payload, os_strlen(payload), 2, 0, 0);
}

int mapToDuty(int duty, int period){return (duty * period) / 255;}

int mapFromDuty(int duty, int period){return (duty * 255) / period;}

int getNextMinDutyIdx(int duty){
	uint8_t i;
	for ( i = BRIGHTNESS_STEPS-1; i >= 0 ; i--){
		if ( brightness[i] > duty ) continue;
		else break;
	}	
    return i;
}

int getNexMaxtDutyIdx(int duty){
	uint8_t i;
	for ( i = 0; i < BRIGHTNESS_STEPS; i++){
		if ( brightness[i] <= duty ) continue;
		else break;
	}
    if (i == BRIGHTNESS_STEPS) i--;
    return i;
}

void updateDuty(int ch, int duty){
    //ESP_LOGI("PWM", "set duty %d", duty);
    //pwm_set_duty(ch, duty);
    //pwm_start();
    PWM_ALL_SET(ch, duty, 0);
    pauseTask(light_delay);
}

void fadeUp(uint8_t ch, int from, int to, int period){
    int idx_from = getNexMaxtDutyIdx(from);
    int idx_to = getNexMaxtDutyIdx(to);
    // ESP_LOGI("PWM", "fadeUp: found next max index from = %d value %d", idx_from, brightness[idx_from]);
    // ESP_LOGI("PWM", "fadeUp: found next max index to = %d value %d", idx_to, brightness[idx_to]);
    for (int i = idx_from; i <= idx_to; i++) {
        //int _duty = mapToDuty(brightness[i], period);
        //updateDuty(ch, _duty);
        updateDuty(ch, brightness[i]);
    }
}

void fadeDown(uint8_t ch, int from, int to, int period){
    int idx_from = getNextMinDutyIdx(from);
    int idx_to = getNextMinDutyIdx(to);
    // ESP_LOGI("PWM", "fadeDown: found next min index from = %d value %d", idx_from, brightness[idx_from]);
    // ESP_LOGI("PWM", "fadeDown: found next min index to = %d value %d", idx_to, brightness[idx_to]);
    for (int i = idx_from; i >= idx_to; i--) {
        //int _duty = mapToDuty(brightness[i], period);
        //updateDuty(ch, _duty);
        updateDuty(ch, brightness[i]);
    } 
}

void fadeChannel(uint8_t ch, int duty){
    int period = 2000;
    pwm_get_period(&period);

    int _fromDuty;
    pwm_get_duty(ch, &_fromDuty);
    
    int fromDuty = mapFromDuty(_fromDuty, period);

    if ( duty > fromDuty) {
        // ESP_LOGI("PWM", "inc duty from %d to %d", fromDuty, duty);
        fadeUp(ch, fromDuty, duty, period);
    } else {
        // ESP_LOGI("PWM", "inc duty from %d to %d", fromDuty, duty);
        fadeDown(ch, fromDuty, duty, period);
    }
    //TODO: update pwmX to new duty
    // char topic[20];
    // sprintf(topic, "pwm%d", ch);
    // mqttSend(topic, duty);
}

void changeFanSpeed(int32_t speed) {
    GPIO_ALL(FAN_SPEED1_GPIO, speed==1);
    GPIO_ALL(FAN_SPEED2_GPIO, speed==2);
    GPIO_ALL(FAN_SPEED3_GPIO, speed==3);
}


void fanControlTask(void * pvParameters){
    for(;;){
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
        pauseTask(FAN_CTRL_TASK_DELAY);
    }
    vTaskDelete(NULL);
}

void receiveMqtt(char *topicBuf,char *dataBuf){
    char lwt[64];
    uint16_t lentopic = os_sprintf(lwt, "%s/%s" topicwrite "/", SENS.mqttlogin, SENS.hostname);
    char *topic = (char *)os_strstr(topicBuf, lwt);
    if (!topic) return;
    topic += lentopic;
    if ( !strcoll(topic, "fanspeed") ) {
        int32_t m = atoi(dataBuf);
        ESP_LOGI("MQTT", "received fan speed  = %d", m);
        if (FAN_SPEED == m) return;
        FAN_SPEED = m;
    } else if ( strstr(topic, "fade") != NULL ) {
          char *istr;
            ESP_LOGI("MQTT", "received fade  = %s", topic);
            istr = (char *)os_strstr(topic, "fade");     
            char ch[3];
            strcpy(ch, istr + 4);
            if (ch) {
                uint8_t channel = atoi(ch);
                ESP_LOGI("MQTT", "fade channel  = %d", channel);
                int32_t duty = atoi(dataBuf);
                if ( duty > 255 ) duty = 255;
                if (channel < SENS.pwmc) {
                    fadeChannel(channel, duty);
                    //pwm_start();
                } else {
                    ESP_LOGE("MQTT", "channel %d is not allowed", channel);    
                }
            } else {
                ESP_LOGI("MQTT", "topic fade without index"); // TODO - по всем каналам сразу
                return;
            }
    } else {
        ESP_LOGI("MQTT", "received topic  = %s", topic);        
    }   
}

void startfunc(){
    // // выполняется один раз при старте модуля.
    prevFanSpeedCfg = FAN_SPEED_CFG;
    FAN_SPEED = FAN_SPEED_CFG;
    changeFanSpeed(FAN_SPEED);

    xTaskCreate(fanControlTask, "fanCtrlTask", 2048, NULL, 5, NULL);
    cb_mqtt_funs=receiveMqtt;
}

void timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду
    if (needSave) {
        mqttSend("fanspeed", FAN_SPEED);
        SAVEOPT;
        needSave = 0;
    }

    if (timersrc%30 == 0) {
        // выполнение кода каждые 30 секунд

    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
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

    // int duty;
    // pwm_get_duty(0, &duty);
    // os_sprintf(HTTPBUFF, "<br>Channel0 duty = %d", duty);

    // int period;
    // pwm_get_period(&period);
    // os_sprintf(HTTPBUFF, "<br>Channel0 period = %d", period);

    // os_sprintf(HTTPBUFF, "<br>pwm0 = %d", SENS.pwm[0]);

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