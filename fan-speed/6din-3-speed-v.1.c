#define FW_VER "1.1"

/*
Options:
Fan Speed 0-3
---
Fan Speed: 0-3 fan speed
---
ValdesCount: 1
*/
#define FAN_CTRL_TASK_DELAY 100

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

void changeFanSpeed(int32_t speed) {
    GPIO_ALL(FAN_SPEED1_GPIO, speed==1);
    GPIO_ALL(FAN_SPEED2_GPIO, speed==2);
    GPIO_ALL(FAN_SPEED3_GPIO, speed==3);
}

void mqttSend(const char *topic, int32_t val){
    char payload[20];
	memset(payload, 0, 20);
	os_sprintf(payload,"%d", val);
	MQTT_Publish(topic, payload, os_strlen(payload), 2, 0, 0);
}

void receiveMqtt(char *topicBuf,char *dataBuf){
    char lwt[64];
    uint16_t lentopic = os_sprintf(lwt, "%s/%s" topicwrite "/", SENS.mqttlogin, SENS.hostname);
    char *topic = (char *)os_strstr(topicBuf, lwt);
    if (!topic) return;
    topic += lentopic;
    if( !strcoll(topic, "fanspeed")) {
        int32_t m = atoi(dataBuf);
        if (FAN_SPEED == m) return;
    }    
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

void startfunc(){
    // выполняется один раз при старте модуля.
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