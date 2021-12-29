static const char* UTAG = "USR";
#define FW_VER "4.11"

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

#define KOTEL1_GPIO sensors_param.cfgdes[0] //208
#define KOTEL2_GPIO sensors_param.cfgdes[1] //209
#define PUMP1_GPIO  sensors_param.cfgdes[2] //210
#define PUMP2_GPIO  sensors_param.cfgdes[3] //211
#define ESC_GPIO    sensors_param.cfgdes[4] //212
#define VENT_GPIO   sensors_param.cfgdes[5] //213

#define NIGHT_TIME  sensors_param.cfgdes[6] //23
#define DAY_TIME    sensors_param.cfgdes[7] //7

#define BACKLIGHT_TIMEOUT    sensors_param.cfgdes[8] // 30

#define KOTEL1_LED_GPIO sensors_param.cfgdes[9]
#define KOTEL2_LED_GPIO sensors_param.cfgdes[10]
#define KOTEL_LED_GPIO sensors_param.cfgdes[11]
#define PUMP_LED_GPIO sensors_param.cfgdes[12]
#define SCHEDULE_LED_GPIO sensors_param.cfgdes[13]
#define VENT_LED_GPIO sensors_param.cfgdes[14]

#define TEMPSET sensors_param.cfgdes[15]
#define BUZZER_GPIO sensors_param.cfgdes[16]
#define PUMP_MODE sensors_param.cfgdes[17]
#define PUMP_OFF_TIMEOUT sensors_param.cfgdes[18]

#define current_temp        valdes[0]  // устанавливать через интерпретер или mqtt
#define street_temp         valdes[1]  // устанавливать через интерпретер или mqtt

#define VALDES_INDEX_WORK_MODE     2   //  
#define work_mode           valdes[VALDES_INDEX_WORK_MODE]

#define VALDES_INDEX_SCHEDULE      3   //  
#define schedule            valdes[VALDES_INDEX_SCHEDULE]
#define reset_fuel          valdes[4]

#define flow_temp data1wire[0]
#define return_temp data1wire[1]

#define KOTEL1_GPIO_DEFAULT 208
#define KOTEL2_GPIO_DEFAULT 209
#define PUMP1_GPIO_DEFAULT 210
#define PUMP2_GPIO_DEFAULT 211
#define ESC_GPIO_DEFAULT 212
#define VENT_GPIO_DEFAULT 213

#define NIGHT_TIME_DEFAULT 23
#define DAY_TIME_DEFAULT 7

#define BACKLIGHT_GPIO 199

#define B(bit_no)         (1 << (bit_no))
#define BIT_CLEAR(reg, bit_no)   (reg) &= ~B(bit_no)
#define BIT_SET(reg, bit_no)   (reg) |= B(bit_no)
#define BIT_CHECK(reg, bit_no)   ( (reg) & B(bit_no) )
#define BIT_TRIGGER(reg, bit_no)   (reg) ^= B(bit_no)

#define GPIO_INVERT(pin) ( GPIO_ALL( pin, !GPIO_ALL_GET( pin )) ) 

#define THERMO_STATE(x)		BIT_CHECK(sensors_param.thermo[x-1][0],0)
#define THERMO_ON(x)  { if ( GPIO_ALL_GET(x+99) == 0 ) GPIO_ALL(99+x,1);}
#define THERMO_OFF(x) { if ( GPIO_ALL_GET(99+x) == 1 ) GPIO_ALL(99+x,0);}


#define THERMO_SETPOINT(x)      sensors_param.thermzn[x-1][0]
#define THERMO_HYSTERESIS(x)	sensors_param.thermzn[x-1][1]

#define THERMO_TEMP_SET(x,y)  { sensors_param.thermzn[x-1][0] = y; SAVEOPT; }
#define THERMO_HYST_SET(x,y)  { sensors_param.thermzn[x-1][1] = y; SAVEOPT; }

#define SPACE_NAME "d51x"
#define PARAM_NAME_WORKMODE "workmode"
#define PARAM_NAME_SCHEDULE "schedule"
#define PARAM_NAME_TEMPSET "tempset"

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c" 
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

#define MCP23017_ISR_DELAY_MS 60

#define millis() (unsigned long) (esp_timer_get_time() / 1000ULL)

typedef void (*func_cb)();

typedef enum {
      PAGE_MAIN
    , PAGE_KOTEL1_RATE
    , PAGE_KOTEL2_RATE
    , PAGE_MENU_WORKMODE
    , PAGE_MENU_SCHEDULE
    , PAGE_MENU_TEMPSET
    , PAGE_MENU_HYST
    , PAGE_MENU_PUMP_SETTINGS
    , PAGE_MENU_VERSION
    , PAGE_MAX
} menu_e;

menu_e menu_idx = PAGE_MAIN;

typedef enum {
    MODE_MANUAL,
    MODE_AUTO,
    MODE_KOTEL1,
    MODE_KOTEL2,
    MODE_MAX
} mode_e;

typedef enum {
    KOTEL_NONE,
    KOTEL_1,
    KOTEL_2
} active_kotel_e;

active_kotel_e active_kotel = KOTEL_NONE;

uint16_t shed_tempset = 0;

#define WORKMODE    work_mode

uint8_t display_alert = 0;
TimerHandle_t  show_alert_timer;
#define SHOW_ALERT_TIMEOUT 3000

uint32_t last_key_press = 0;
#define MENU_EXIT_TIMEOUT 60000 // 10 sec

typedef enum {
    PUMP_MODE_NONE, // * 0. не управляем реле насоса
    PUMP_MODE_1,    // * 1. реле насоса вкл/выкл по реле котла
    PUMP_MODE_2,    // * 2. реле насоса вкл по реле котла, выкл через Х минут, после выключения реле котла
    PUMP_MODE_3,    // * 3. реле насоса вкл по реле котла, выкл по температуре обратки
    PUMP_MODE_MAX
} pump_mode_e;

//pump_mode_e pump_mode;

#define PUMP1_OFF_TIMEOUT    PUMP_OFF_TIMEOUT  // seconds
#define PUMP2_OFF_TIMEOUT    PUMP_OFF_TIMEOUT  // seconds

// ******************************************************************************
// ********** ФУНКЦИИ ДЛЯ РАБОТЫ С NVS *****************************************
// ******************************************************************************
esp_err_t nvs_param_load(const char* space_name, const char* key, void* dest)
{
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    nvs_handle my_handle;
    size_t required_size = 0;
    ret = nvs_open(space_name, NVS_READWRITE, &my_handle);
    ret = nvs_get_blob(my_handle, key, NULL, &required_size);
    if (required_size == 0) {
        ESP_LOGW("NVS", "the target [ %s ] you want to load has never been saved", key);
        ret = ESP_FAIL;
        goto LOAD_FINISH;
    }
    ret = nvs_get_blob(my_handle, key, dest, &required_size);

  LOAD_FINISH:
    nvs_close(my_handle);

  OPEN_FAIL:
    return ret;
}

esp_err_t nvs_param_save(const char* space_name, const char* key, void *param, uint16_t len)
{
    esp_err_t ret = ESP_ERR_INVALID_ARG;
    nvs_handle my_handle;
    ret = nvs_open(space_name, NVS_READWRITE, &my_handle);
    ret = nvs_set_blob(my_handle, key, param, len);
    ret = nvs_commit(my_handle);

  SAVE_FINISH:
    nvs_close(my_handle);

  OPEN_FAIL:
    return ret;
}

esp_err_t nvs_param_save_u16(const char* space_name, const char* key, uint16_t *param)
{
    uint16_t val = 0;
    esp_err_t err = nvs_param_load(space_name, key, &val);
    if ( err != ESP_OK ) return err;
    if ( val != *param) {
        err = nvs_param_save(space_name, key, param, sizeof(uint16_t));
    }
    return err;
}

esp_err_t nvs_param_save_u32(const char* space_name, const char* key, uint32_t *param)
{
    ESP_LOGI(UTAG, "%s: space = %s, param = %s", __func__, space_name, key);
    uint32_t val = 0;
    esp_err_t err = nvs_param_load(space_name, key, &val);
    //ESP_LOGI(UTAG, "value in nvs = %d, value to save = %d", val, *param);
    if ( err != ESP_OK ) {
        err = nvs_param_save(space_name, key, param, sizeof(uint32_t));
        return err;
    }
    if ( val != *param) {
        //ESP_LOGI(UTAG, "need save");
        err = nvs_param_save(space_name, key, param, sizeof(uint32_t));
    } 
    //else {
    //     ESP_LOGI(UTAG, "no need save");
    // }
    return err;
}

// ******************************************************************************
// ********** ФУНКЦИИ ДЛЯ РАБОТЫ С Buzzer ***************************************
// ******************************************************************************
//#define BUZZER_GPIO 16

typedef enum {
    BUZZER_BEEP_SHORT_EXTRA,
    BUZZER_BEEP_SHORT_VERY,
    BUZZER_BEEP_SHORT,
    BUZZER_BEEP_MEDIUM,
    BUZZER_BEEP_LONG,
    BUZZER_BEEP_LONG_VERY,
    BUZZER_BEEP_LONG_EXTRA,
    BUZZER_BEEP_DOUBLE_SHORT,
    BUZZER_BEEP_ERROR,
    BUZZER_BEEP_MAX
} beep_type_e;

TimerHandle_t  buzzer_timer;
QueueHandle_t buzzer_queue;
TaskHandle_t buzzer_task;

typedef struct {
    uint16_t action;    // 0 - off, 1 - on
    uint16_t delay; //msec 
} buzzer_beep_t;

#define BEEP_COMMAND_LENGTH 6
buzzer_beep_t beeps[BUZZER_BEEP_MAX][BEEP_COMMAND_LENGTH] = 
        { 
              { {1,40}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }           //BUZZER_BEEP_SHORT_EXTRA,
            , { {1,80}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }           //BUZZER_BEEP_SHORT_VERY
            , { {1,120}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }          //BUZZER_BEEP_SHORT
            , { {1,160}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }          //BUZZER_BEEP_MEDIUM
            , { {1,200}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }          //BUZZER_BEEP_LONG
            , { {1,300}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }          //BUZZER_BEEP_LONG_VERY
            , { {1,500}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }          //BUZZER_BEEP_LONG_EXTRA
            , { {1,120}, {0,120}, {1,120}, {0,0}, {0,0}, {0,0} }          //BUZZER_BEEP_DOUBLE_SHORT
            , { {1,100}, {0,100}, {1,100}, {0,100}, {1,100}, {0,0} }          //BUZZER_BEEP_ERROR
        };



void buzzer_beep(uint8_t pattern)
{
    GPIO_ALL( BUZZER_GPIO, 0);
    for (uint8_t i = 0; i < BEEP_COMMAND_LENGTH; i++)
    {
        //ESP_LOGI(UTAG, "buzzer %d, action %d, delay %d", i, pattern[i].action, pattern[i].delay);
        GPIO_ALL( BUZZER_GPIO, beeps[pattern][i].action);
        if (beeps[pattern][i].delay > 0 )
            vTaskDelay( beeps[pattern][i].delay / portTICK_PERIOD_MS );
    }
    GPIO_ALL( BUZZER_GPIO, 0);
}

static void buzzer_cb(void *arg) {
	while (1) {

            uint8_t pattern;
            if ( xQueueReceive(buzzer_queue, &pattern, 0) == pdPASS) 
            {
                buzzer_beep(pattern);
            }
        vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
    vTaskDelete(NULL);
}

void buzzer(uint8_t pattern)   // str_idx номер строки конструктора строк, с 0 начинается
{
    if ( BUZZER_GPIO == 255 ) return;

    GPIO_ALL( BUZZER_GPIO, 0);
    xQueueOverwrite(buzzer_queue, ( void * )&pattern);
  
}

void buzzer_init()
{
    buzzer_queue = xQueueCreate(1, sizeof(50));
    xTaskCreate( buzzer_cb, "buzzer", 512+256+128, NULL, 10, &buzzer_task); 

    buzzer( BUZZER_BEEP_ERROR );   
}


//*****************************************************************************************************************
//*****************  БЛОК ПЕРЕМЕННЫХ И ФУНКЦИЙ ДЛЯ УЧЕТА РАСХОДА ДИЗЕЛЯ *******************************************
//*****************************************************************************************************************
#define FUEL_PUMP_GPIO	13
#define CONSUMP_ML_SEC 0.55f
#define CONSUMP_L_SEC 0.00055f
#define COUNTER_THRESHOLD 30    // задать через cfgdes

uint16_t fpump_state = 0;
uint32_t fpump_start_dt = 0;
uint16_t fpump_on_cnt = 0;
uint32_t fpump_on_duration = 0;
uint32_t fpump_on_duration_prev = 0;
//время работы
uint32_t fpump_work_time = 0;             // время работы за все время
uint32_t fpump_today_time = 0;          // время работы за сегодня
uint32_t fpump_prev_time = 0;           // время работы за вчера
// расходы
uint32_t i_fuel_consump_last;                    // предыдущий расход
uint32_t i_fuel_consump_now;                    // текущий расход
uint32_t i_fuel_consump_today;                    // расход за сегодня
uint32_t i_fuel_consump_prev;                       // расход за вчера
uint32_t i_fuel_consump_total;                  // расход за все время

// NVS FUEL PUMP
#define SPACE_FUEL_PUMP "fuelpump"
#define FUEL_STATE_CNT_PARAM "fuelcnt"      // расход за предыдущее включение
#define FUEL_CONSUMP_LAST_PARAM "conslast"      // расход за предыдущее включение
#define FUEL_CONSUMP_NOW_PARAM "consnow"      // расход за последнее включение
#define FUEL_CONSUMP_DAY_PARAM "consday"        // расход за день
#define FUEL_CONSUMP_PREV_PARAM "consprev"      // расход за вчера
#define FUEL_CONSUMP_TOTAL_PARAM "consttl"      // расход общий

#define FUEL_WORKTIME_LAST_PARAM "wrktlast"     // предыдущая длительность работы
#define FUEL_WORKTIME_NOW_PARAM "wrktnow"     // последняя длительность работы
#define FUEL_WORKTIME_DAY_PARAM "wrktday"       // длительность работы сегодня
#define FUEL_WORKTIME_PREV_PARAM "wrktprev"     // длительность работы вчера
#define FUEL_WORKTIME_TOTAL_PARAM "wrktttl"     // длительность работы общая

uint32_t get_consump_total()
{
	return 	i_fuel_consump_total / 100;
}

uint32_t get_consump_today()
{
	return 	i_fuel_consump_today / 100;
}

uint32_t get_consump_prev()
{
	return 	i_fuel_consump_prev / 100;
}

void detect_fuel_pump_work()
{
   // ESP_LOGI(UTAG, "%s", __func__);
    #ifdef count60e     // on/off option
        fpump_state = ( count60end[0] > COUNTER_THRESHOLD );  // если просто > 0? то проскакивают левые импульсы

        static uint16_t fpump_state_prev = 0;
        if ( fpump_state != fpump_state_prev ) {
            // состояние изменилось
            // TODO: отправить по mqtt немедленно
            // TODO: включить индикацию, если надо
            fpump_state_prev = fpump_state;

            if ( fpump_state ) {
                // переключился из 0 в 1  (!!! может проскакивать импульс и поэтому cnt увеличивается на 1 и предыдущее время обнуляется, регулируется отсечкой подсчета импульсов)
                fpump_on_cnt++;
                fpump_start_dt = millis();  // при включении начали отсчет
                i_fuel_consump_now = 0;     // обнуление текущего расхода       

                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_STATE_CNT_PARAM, &fpump_on_cnt);         
            } else {
                // переключился из 1 в 0


                
                i_fuel_consump_last = i_fuel_consump_now;
                i_fuel_consump_now = 0;
                fpump_on_duration_prev = fpump_on_duration;
                fpump_on_duration = 0;

                // LAST
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_LAST_PARAM, &i_fuel_consump_last);
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_LAST_PARAM, &fpump_on_duration_prev);

                // now (last)
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_NOW_PARAM, &i_fuel_consump_now);
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_NOW_PARAM, &fpump_on_duration);
                // сохраним результаты подсчета в nvs

                // today
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_DAY_PARAM, &i_fuel_consump_today);
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_DAY_PARAM, &fpump_today_time);

                // total
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_TOTAL_PARAM, &i_fuel_consump_total);
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_TOTAL_PARAM, &fpump_work_time);
		    }
        }
    #endif
}

void fuel_consumption_calc()
{
    //ESP_LOGI(UTAG, "%s", __func__);
    #ifdef count60e     // on/off option

        static uint8_t real_zero = 1;
        if ( time_loc.hour == 1 && time_loc.min == 0 && time_loc.sec == 0 )
        {
            // обнулить суточные данные ночью (в час ночи)
            fpump_prev_time = fpump_today_time;
            fpump_today_time = 0;
            i_fuel_consump_prev = i_fuel_consump_today;
            i_fuel_consump_today = 0;
            fpump_on_cnt = 0;

            if ( real_zero  ){
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_DAY_PARAM, &i_fuel_consump_today);
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_PREV_PARAM, &i_fuel_consump_prev);
            
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_DAY_PARAM, &fpump_today_time);
                nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_PREV_PARAM, &i_fuel_consump_prev);
                real_zero = 0;
            }
        }

        if ( fpump_state ) {
            // топливный насос включен, увеличиваем расходы
            fpump_work_time++;  // увеличиваем время работы за все время
            fpump_today_time++; // увеличиваем время работы за сегодня
            i_fuel_consump_total++; // увеличиваем время работы за все время
            i_fuel_consump_now += CONSUMP_L_SEC*100000; // увеличиваем счетчик текущего расхода
            i_fuel_consump_today += CONSUMP_L_SEC*100000; // увеличиваем счетчик расходня за сегодня
            i_fuel_consump_total += CONSUMP_L_SEC*100000; // увеличиваем счетчик расходня за сегодня
            fpump_on_duration = millis() - fpump_start_dt;	// считаем время
        }    
    #endif
}

void fuel_reset_data()
{
    ESP_LOGI(UTAG, "%s", __func__);
    i_fuel_consump_last = 0;
    i_fuel_consump_now = 0;
    i_fuel_consump_total = 0;
    i_fuel_consump_today = 0;
    i_fuel_consump_prev = 0;
    
    fpump_on_duration_prev = 0;
    fpump_on_duration = 0;
    fpump_work_time = 0;
    fpump_today_time = 0;
    fpump_prev_time = 0;
    fpump_on_cnt = 0;   

    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_STATE_CNT_PARAM, &fpump_on_cnt); 
    
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_LAST_PARAM, &i_fuel_consump_last);            
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_NOW_PARAM, &i_fuel_consump_now);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_DAY_PARAM, &i_fuel_consump_today);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_PREV_PARAM, &i_fuel_consump_prev);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_TOTAL_PARAM, &i_fuel_consump_total);

    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_LAST_PARAM, &fpump_on_duration_prev);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_NOW_PARAM, &fpump_on_duration);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_DAY_PARAM, &fpump_today_time);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_PREV_PARAM, &i_fuel_consump_prev);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_TOTAL_PARAM, &fpump_work_time);     
}

void fuel_save_data()
{
    ESP_LOGI(UTAG, "%s", __func__);

    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_STATE_CNT_PARAM, &fpump_on_cnt); 

    // last
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_LAST_PARAM, &i_fuel_consump_last);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_LAST_PARAM, &fpump_on_duration_prev);
	    
    // now (last)
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_NOW_PARAM, &i_fuel_consump_now);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_NOW_PARAM, &fpump_on_duration);

    // today
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_DAY_PARAM, &i_fuel_consump_today);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_DAY_PARAM, &fpump_today_time);

    // yesterday
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_PREV_PARAM, &i_fuel_consump_prev);
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_PREV_PARAM, &i_fuel_consump_prev);

    // total
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_CONSUMP_TOTAL_PARAM, &i_fuel_consump_total);    
    nvs_param_save_u32(SPACE_FUEL_PUMP, FUEL_WORKTIME_TOTAL_PARAM, &fpump_work_time);
}

void fuel_load_data()
{
    ESP_LOGI(UTAG, "%s", __func__);

    // prev
    nvs_param_load(SPACE_FUEL_PUMP, FUEL_CONSUMP_LAST_PARAM, &i_fuel_consump_last);
    ESP_LOGI(UTAG, "loaded " FUEL_CONSUMP_LAST_PARAM " = %d", i_fuel_consump_last);

    nvs_param_load(SPACE_FUEL_PUMP, FUEL_WORKTIME_LAST_PARAM, &fpump_on_duration_prev);
    ESP_LOGI(UTAG, "loaded " FUEL_WORKTIME_LAST_PARAM " = %d", fpump_on_duration_prev);

    // now
    nvs_param_load(SPACE_FUEL_PUMP, FUEL_CONSUMP_NOW_PARAM, &i_fuel_consump_now);
    ESP_LOGI(UTAG, "loaded " FUEL_CONSUMP_NOW_PARAM " = %d", i_fuel_consump_now);

    nvs_param_load(SPACE_FUEL_PUMP, FUEL_WORKTIME_NOW_PARAM, &fpump_on_duration);
    ESP_LOGI(UTAG, "loaded " FUEL_WORKTIME_NOW_PARAM " = %d", fpump_on_duration);

    // today
    nvs_param_load(SPACE_FUEL_PUMP, FUEL_CONSUMP_DAY_PARAM, &i_fuel_consump_today);
    ESP_LOGI(UTAG, "loaded " FUEL_CONSUMP_DAY_PARAM " = %d", i_fuel_consump_today);

    nvs_param_load(SPACE_FUEL_PUMP, FUEL_WORKTIME_DAY_PARAM, &fpump_today_time);
    ESP_LOGI(UTAG, "loaded " FUEL_WORKTIME_DAY_PARAM " = %d", fpump_today_time);

    // yesterday
    nvs_param_load(SPACE_FUEL_PUMP, FUEL_CONSUMP_PREV_PARAM, &i_fuel_consump_prev);
    ESP_LOGI(UTAG, "loaded " FUEL_CONSUMP_PREV_PARAM " = %d", i_fuel_consump_prev);

    nvs_param_load(SPACE_FUEL_PUMP, FUEL_WORKTIME_PREV_PARAM, &i_fuel_consump_prev);
    ESP_LOGI(UTAG, "loaded " FUEL_WORKTIME_PREV_PARAM " = %d", i_fuel_consump_prev);

    // total
    nvs_param_load(SPACE_FUEL_PUMP, FUEL_CONSUMP_TOTAL_PARAM, &i_fuel_consump_total);
    ESP_LOGI(UTAG, "loaded " FUEL_CONSUMP_TOTAL_PARAM " = %d", i_fuel_consump_total);

    nvs_param_load(SPACE_FUEL_PUMP, FUEL_WORKTIME_TOTAL_PARAM, &fpump_work_time);
    ESP_LOGI(UTAG, "loaded " FUEL_WORKTIME_TOTAL_PARAM " = %d", fpump_work_time);
}

void webfunc_print_fuel_pump_data(char *pbuf)
{
	os_sprintf(HTTPBUFF, "<hr>");
	
	//os_sprintf(HTTPBUFF, "Fuel Pump: <b>%s</b> &nbsp; <b>count:</b> %d &nbsp; ", fpump_state ? "ON" : "OFF", fpump_on_cnt );
	//os_sprintf(HTTPBUFF, "<details><summary>");
    //os_sprintf(HTTPBUFF, "<b>PrevDuration:</b> %d:%02d", (fpump_on_duration_prev / 1000) / 60,  (fpump_on_duration_prev / 1000) % 60);
	//os_sprintf(HTTPBUFF, "&nbsp;<b>PrevConsumption:</b> %d.%03d<br>", i_fuel_consump_last / 100000, (i_fuel_consump_last % 100000) / 100);
    
    
    //os_sprintf(HTTPBUFF, "</summary>");
    

	uint32_t sec = fpump_work_time % 60;
	uint32_t min = fpump_work_time / 60;
	uint32_t hour = (min / 60 % 24);
	min = min % 60;

	uint32_t _sec = fpump_today_time % 60;
	uint32_t _min = fpump_today_time / 60;
	uint32_t _hour = _min / 60;
	_min = _min % 60;
    
    //os_sprintf(HTTPBUFF, "Fuel Pump: <b>%s</b> &nbsp; count: <b>%d</b>", fpump_state ? "ON" : "OFF", fpump_on_cnt );
	os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>"

                            "<tr align=right>"
                                "<td>Fuel Pump:</td><td><b>%s</b></td><td>count: <b>%d</b></td>"
                            "</tr>"

							"<tr align=right>"
								"<th></th><th>Work time:</th><th>Consumption, L:</th>"
							"</tr>"
                            , fpump_state ? "ON" : "OFF", fpump_on_cnt
				);
				

	os_sprintf(HTTPBUFF, 	"<tr align=right>"
								"<td><b>Now:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, (fpump_on_duration  / 1000 ) / 60 / 60
                            , (fpump_on_duration  / 1000 ) / 60
                            , (fpump_on_duration  / 1000 ) % 60
							, i_fuel_consump_now / 100000
                            , (i_fuel_consump_now % 100000) / 100
	);	
	os_sprintf(HTTPBUFF, 	"<tr align=right>"
								"<td><b>Today:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, _hour, _min, _sec
							, i_fuel_consump_today / 100000, (i_fuel_consump_today % 100000) / 100
	);

    os_sprintf(HTTPBUFF, "</table>");



    os_sprintf(HTTPBUFF, "<details><summary></summary>");
    os_sprintf(HTTPBUFF, "<table width='100%%' cellpadding='2' cellspacing='2' cols='3'>");

	os_sprintf(HTTPBUFF, 	"<tr align=right>"
								"<td><b>Prev time:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, (fpump_on_duration_prev / 1000 ) / 60 / 60 
							, (fpump_on_duration_prev / 1000 ) / 60
                            , (fpump_on_duration_prev / 1000 ) % 60
							, i_fuel_consump_last / 100000
                            , (i_fuel_consump_last % 100000) / 100
	);




	_sec = fpump_prev_time % 60;
	_min = fpump_prev_time / 60;
	_hour = _min / 60;
	_min = _min % 60;

	os_sprintf(HTTPBUFF, 	"<tr align=right>"
								"<td><b>Yesterday:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, _hour, _min, _sec
							, i_fuel_consump_prev / 100000, (i_fuel_consump_prev % 100000) / 100
	);

	os_sprintf(HTTPBUFF, 	"<tr align=right>"
								"<td><b>Total:</b></td><td>%02d:%02d:%02d</td><td>%d.%03d</td>"
							"</tr>"
							, hour, min, sec
							, i_fuel_consump_total / 100000, (i_fuel_consump_total % 100000) / 100
	);	

	os_sprintf(HTTPBUFF, 	"</table>"); 
    os_sprintf(HTTPBUFF, "</details>");      
}



// ******************************************************************************
// ********** УПРАВЛЕНИЕ ПОДСВЕТКОЙ ДИСПЛЕЯ И ВЫВОДОМ ***************************
// ******************************************************************************
#include "tcpip_adapter.h"
#include "lwip/ip_addr.h"

#define BACKLIGHT_TIMEOUT_DEFAULT 30 //sec
#if lcde
    #define LCD_BACKLIGHT_STATE BIT_CHECK(sensors_param.lcdled,0)
#else
    #define LCD_BACKLIGHT_STATE 1
#endif

// const uint8_t lcd_char_arrow_up[8] =      // кодируем символ градуса
// {
//   0b00000,
//   0b00100,
//   0b01110,
//   0b10101,
//   0b00100,
//   0b00100,
//   0b00100,
//   0b00000,
// }; 
// #define SYMBOL_ARROW_UP 0x02

const uint8_t lcd_char_moon[8] =      // кодируем символ градуса
{
  0b11100,
  0b00110,
  0b00011,
  0b00011,
  0b00011,
  0b00110,
  0b11100,
  0b00000,  
}; 
#define SYMBOL_MOON 0x02

// const uint8_t lcd_char_arrow_down[8] =      // кодируем символ градуса
// {
//   0b00000,
//   0b00100,
//   0b00100,
//   0b00100,
//   0b10101,
//   0b01110,
//   0b00100,
//   0b00000,
// }; 
// #define SYMBOL_ARROW_DOWN 0x03

const uint8_t lcd_char_sun[8] =      // кодируем символ градуса
{
  0b00100,
  0b10101,
  0b01110,
  0b11111,
  0b01110,
  0b10101,
  0b00100,
  0b00000,
}; 
#define SYMBOL_SUN 0x03

const uint8_t lcd_char_vent_part1[8] =      // кодируем символ градуса
{
  0b00011,
  0b00010,
  0b11001,
  0b11110,
  0b10110,
  0b11001,
  0b00011,
  0b00111,
}; 
#define SYMBOL_VENT_PART1 0x04

const uint8_t lcd_char_vent_part2[8] =      // кодируем символ градуса
{
  0b11000,
  0b11000,
  0b10011,
  0b01101,
  0b01111,
  0b10011,
  0b01000,
  0b11000,
}; 
#define SYMBOL_VENT_PART2 0x05

const uint8_t lcd_char_schedule_part1[8] =
{
  0b00110,
  0b01111,
  0b10000,
  0b10110,
  0b10010,
  0b10111,
  0b10000,
  0b01111,
}; 
#define SYMBOL_SCHEDULE_PART1 0x06

const uint8_t lcd_char_schedule_part2[8] =
{
  0b01100,
  0b11110,
  0b00001,
  0b11001,
  0b01001,
  0b11101,
  0b00001,
  0b11110,
}; 
#define SYMBOL_SCHEDULE_PART2 0x07

const uint8_t lcd_char_bar_full[8] =
{
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
}; 
#define SYMBOL_BAR_FULL 0x08

// const uint8_t lcd_char_bar_half[8] =
// {
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
// }; 
// #define SYMBOL_BAR_HALF 0x09

// const uint8_t lcd_char_night[8] =
// {
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
//   0b11100,
// }; 
// #define SYMBOL_NIGHT 0x0A

#define LCD_SPLASH_TIMEOUT 5
#define SYMBOL_DEGREE 0x01
#define SYMBOL_SPACE 0x20
#define SYMBOL_ARROW_RIGHT 126
#define SYMBOL_ARROW_LEFT 127
#define SYMBOL_CELL 10

#define LCD_CMD_CLEAR               0x01 

#define LCD_CMD_DISPLAY_ON           0x04        // turns display ON/retrive text (D)
#define LCD_CMD_DISPLAY_OFF          0x00        // turns display OFF/clears text (D)
#define LCD_CMD_UNDERLINE_CURSOR_ON  0x02        // turns ON  underline cursor (C)
#define LCD_CMD_UNDERLINE_CURSOR_OFF 0x00        // turns OFF underline cursor (C)
#define LCD_CMD_BLINK_CURSOR_ON      0x01        // turns ON  blinking  cursor (B)
#define LCD_CMD_BLINK_CURSOR_OFF     0x00        // turns OFF blinking  cursor (B)

TimerHandle_t  backlight_timer;
uint8_t lcd_splash = 1;

#define LCD_KOTEL1_PAGE_LINES_CNT 10
uint8_t lcd_kotel1_page_line = 0;

#define LCD_PUMP_OPTIONS 3
uint8_t lcd_pump_options = 0;
uint8_t lcd_pump_in_menu = 0;

void backlight_timer_cb(xTimerHandle tmr)   // rtos
{
    uint8_t pin = (uint8_t)pvTimerGetTimerID(tmr); // rtos
    GPIO_ALL(pin, 0);
    xTimerStop(tmr, 10);
    xTimerDelete(tmr, 10);
    backlight_timer = NULL;
}

void turn_on_lcd_backlight(uint8_t pin, uint8_t *state)
{
    
   // каждое нажатие кнопки включает подсветку дисплея
    GPIO_ALL(pin, 1);

    // и запускает таймер на отключение подсветки дисплея на Х секунд, передается в аргументе
    // BACKLIGHT_TIMEOUT
    if ( backlight_timer == NULL )
    {
        backlight_timer = xTimerCreate("bcklght", BACKLIGHT_TIMEOUT * 1000 / portTICK_PERIOD_MS, pdFALSE, pin, backlight_timer_cb);
    }

    if ( xTimerIsTimerActive( backlight_timer ) == pdTRUE )
    {
        xTimerStop( backlight_timer, 0);
    }    

    xTimerStart( backlight_timer, 0);
}

void lcd_print_(uint8_t line, const char *str)
{
    #if lcde
        LCD_print(line, str);
    #endif
}

void lcd_print(uint8_t line, const char *str)
{
    // если sens_state вздедена датчиками, то дисплей не выводит )))
    if ( display_alert == 1 ) return;
    lcd_print_(line, str);
}

void show_display_alert_cb(xTimerHandle tmr)   // rtos
{
    display_alert = 0;
    xTimerStop( show_alert_timer, 0);
    xTimerDelete(show_alert_timer, 10);
    show_alert_timer = NULL;
}

void print_alert(const char *title, const char *str)
{
    lcd_print_(0, title);
    char err[21];
    if ( strlen(str) > 20 )
    {
        strncpy(err, str, 21);
        lcd_print_(1, err);  
        str += 20;
        
        if ( strlen(str) > 20 ) 
        {
            strncpy(err, str, 21);
            lcd_print_(2, err); 
            str += 20;
            lcd_print_(3, str);
        } else {
            lcd_print_(2, str);
            lcd_print_(3, "                    "); 
        }  

        
    } else {
        lcd_print_(1, "                    "); 
        lcd_print_(2, str); 
        lcd_print_(3, "                    "); 
    }
    
}

void show_display_alert(const char *title, const char *str, func_cb cb)
{
    
    display_alert = 0;

    if ( show_alert_timer == NULL )
    {
        show_alert_timer = xTimerCreate("lcdalert", SHOW_ALERT_TIMEOUT / portTICK_PERIOD_MS, pdFALSE, NULL, show_display_alert_cb);
    }

    if ( xTimerIsTimerActive( show_alert_timer ) == pdTRUE )
    {
        xTimerStop( show_alert_timer, 0);
    }    

    xTimerStart( show_alert_timer, 0);   

    // show error
    if ( cb == NULL)
        print_alert(title, str);
    else
        cb();

    display_alert = 1;
}

void menu_next()
{
    if ( display_alert == 1 ) return;
    menu_idx++;
    if ( menu_idx >= PAGE_MAX ) menu_idx = PAGE_MAIN;
    last_key_press = millis();

    if ( menu_idx == PAGE_KOTEL1_RATE ) lcd_kotel1_page_line = 0;
}

void menu_prev()
{
    if ( display_alert == 1 ) return;
    menu_idx--;
    if ( menu_idx == PAGE_MAIN ) menu_idx = PAGE_MAX - 1;
    last_key_press = millis();

    if ( menu_idx == PAGE_KOTEL1_RATE ) lcd_kotel1_page_line = 0;
}

int round_int_100(int val)
{
    int res = val / 100;
    int mod = val % 100;
    if ( mod >=50 ) {
        if ( res > 0) res++;
        else res--;
    }
    return res;
}

void show_main_page()
{
    char str[30];

    // tu, hh:mm    либо tu, dd.mm
    static uint32_t ii = 0;
    char weekday[2] = "";
    switch ( time_loc.dow ) {
        case 0: strcpy(weekday, "Mo"); break;
        case 1: strcpy(weekday, "Tu"); break;
        case 2: strcpy(weekday, "We"); break;
        case 3: strcpy(weekday, "Th"); break;
        case 4: strcpy(weekday, "Fr"); break;
        case 5: strcpy(weekday, "Sa"); break;
        case 6: strcpy(weekday, "Su"); break;
        default: break;
    }

    #define time_delay 20
    
    // t	u	,	1	1	:	2	2					T	>	:	2	4	.	2	°
    #define line1_pattern "%2s, %02d%s%02d   T%c:%2d.%1d%c"
    
    // K	O	T	E	L	1	(	*	)				T	<	:	3	1	.	2	°
    #define line2_pattern "%-10s  T%c:%2d.%1d%c"
    
    

    snprintf(str, 21, line1_pattern
                , weekday
                , ( ii % time_delay > 0) ? time_loc.hour : time_loc.day
                , ( ii % time_delay > 0) ? ( ii % 2 ? ":" : " ") : "."
                , ( ii % time_delay > 0) ? time_loc.min : time_loc.month
                , SYMBOL_ARROW_RIGHT            
                , flow_temp / 100
                , (flow_temp % 100)/10
                , SYMBOL_DEGREE  
            );

    lcd_print(0, str);

    // режим работы и котел активный
    char smode[10] = "";
    //char sactive[4];
    if ( work_mode == MODE_MANUAL ) 
    {
        strcpy(smode, "MANUAL");
        strcat(smode, "[");
        strcat(smode, GPIO_ALL_GET( KOTEL1_GPIO) ? "1" : "-");
        strcat(smode, GPIO_ALL_GET( KOTEL2_GPIO) ? "2" : "-");
    }
    else if ( work_mode == MODE_KOTEL1 ) {
        strcpy(smode, "KOTEL1");
        strcat(smode, GPIO_ALL_GET( KOTEL1_GPIO ) ? "[*" : "[-");
    }
    else if ( work_mode == MODE_KOTEL2 ) 
    {
        strcpy(smode, "KOTEL2");
        strcat(smode, GPIO_ALL_GET( KOTEL2_GPIO ) ? "[*" : "[-");
    }    
    else if ( work_mode == MODE_AUTO ) 
    {
        strcpy(smode, "AUTO");
        if ( active_kotel == KOTEL_1 ) 
        {
            strcat(smode,  "[1");
            strcat(smode, THERMO_STATE(1) ? "*" : "");
        }
        else if ( active_kotel == KOTEL_2 )
        {
            strcat(smode,  "[2");
            strcat(smode, THERMO_STATE(2) ? "*" : "");
        }
        else 
            strcat( smode, "[-"); 
    }
    else {
        strcpy(smode, "ERROR[-");
    }
    strcat(smode, "]");
    
    snprintf(str, 21, line2_pattern
                , smode
                , SYMBOL_ARROW_LEFT       
                , return_temp / 100
                , (return_temp % 100)/10
                , SYMBOL_DEGREE  
            );

    lcd_print(1, str);

    char sc[6] = "";
    if ( schedule ) 
    {
        static pos = 0;
        strcpy(sc, "");
        for ( uint8_t j = 0; j < 5; j++) 
        {
            strcat(sc, j == pos ? ">" : "-");
        }
        pos++;
        if ( pos > 5 ) pos = 0;
        strcat(sc, " ");
    } else {
        strcpy(sc, ii % 2 ? "     >" : "      ");
    }
    // S	с	h	d	:	O	F	F				>	T	s	:	2	2	.	2	°
    //#define line3_pattern "Schd:%3s%4sTs:%2d.%1d%c"    
    #define line3_pattern "%c%c%3s %6sTs:%2d.%1d%c"    

    snprintf(str, 21, line3_pattern
                , SYMBOL_SCHEDULE_PART1
                , SYMBOL_SCHEDULE_PART2
                , schedule ? "ON " : "OFF"
                , sc
                , THERMO_SETPOINT(1) / 10
                , THERMO_SETPOINT(1) % 10
                , SYMBOL_DEGREE);
    lcd_print(2, str);
    
    static uint8_t show_cur_temp = 0;
    if ( ii % 5 == 0 ) show_cur_temp = 1 - show_cur_temp;

     // V	e	n	t	:	O	F	F					S	t	:	2	2	.	2	°
    //#define line4_pattern "Vent:%3s    %2s:%2d.%1d%c"
    #define line4_pattern "%c%c%3s %c%3s  T%c:%2d.%1d%c"

    snprintf(str, 21, line4_pattern
                , SYMBOL_VENT_PART1
                , SYMBOL_VENT_PART2
                , GPIO_ALL_GET( VENT_GPIO ) ? "ON " : "OFF"
                , SYMBOL_MOON
                , GPIO_ALL_GET( ESC_GPIO ) ? "ON " : "OFF"
                , show_cur_temp ?  SYMBOL_SUN : '#'
                , show_cur_temp ? ( street_temp / 10 ) : ( current_temp / 10 )
                , show_cur_temp ? ( street_temp % 10 ) : ( current_temp % 10 )
                , SYMBOL_DEGREE);
    lcd_print(3, str);        

    ii++;
}

void lcd_show_splash(uint8_t timeout)
{
    static uint8_t countup = 0;
    char str[21];
    snprintf(str, 21, "Starting %s", sensors_param.hostname);
    lcd_print(0, str);
    snprintf(str, 21, "    firmware v.%5s", FW_VER);
    
    lcd_print(1, str);

    tcpip_adapter_ip_info_t local_ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
    snprintf(str,21, "IP: %d.%d.%d.%d" , IP2STR( &local_ip.ip) );
    lcd_print(2, str);

    countup++;
    memset(str, 0, 21);

    //for ( uint8_t i = 0; i < timeout; i++ )
    //{
        memset(str, SYMBOL_SPACE, 20);
        //if ( i < countup ) {
            memset(str, SYMBOL_BAR_FULL, (20 * countup ) / timeout);
        //sprintf(str + strlen( str ), "%c%c", SYMBOL_BAR_FULL, SYMBOL_BAR_FULL);      
        //} 
    //}

    lcd_print(3, str);
    if ( countup == timeout ) lcd_splash = 0;

}

void lcd_init2()
{
    //LCDI2C_createChar( SYMBOL_ARROW_UP, lcd_char_arrow_up );    
    //LCDI2C_createChar( SYMBOL_ARROW_DOWN, lcd_char_arrow_down );
    LCDI2C_createChar( SYMBOL_MOON, lcd_char_moon );
    LCDI2C_createChar( SYMBOL_SUN, lcd_char_sun );
    LCDI2C_createChar( SYMBOL_VENT_PART1, lcd_char_vent_part1 );
    LCDI2C_createChar( SYMBOL_VENT_PART2, lcd_char_vent_part2 );
    LCDI2C_createChar( SYMBOL_SCHEDULE_PART1, lcd_char_schedule_part1 );
    LCDI2C_createChar( SYMBOL_SCHEDULE_PART2, lcd_char_schedule_part2 );
    LCDI2C_createChar( SYMBOL_BAR_FULL, lcd_char_bar_full );
    //LCDI2C_createChar( SYMBOL_BAR_HALF, lcd_char_bar_half );
}

void show_page_kotel1_rate()
{
    //char str[30] = "";
    lcd_print(0, "*** KOTEL1 RATE ****");

    char lines[LCD_KOTEL1_PAGE_LINES_CNT][30];

    snprintf(lines[0], 21, "current    %3d.%03d L", i_fuel_consump_now / 100000, (i_fuel_consump_now % 100000) / 100);
    snprintf(lines[1], 21, "current     %02d:%02d:%02d", fpump_on_duration  / 1000 / 60 / 60
                                                    , fpump_on_duration  / 1000 / 60
                                                    , fpump_on_duration  / 1000 % 60
            );

    snprintf(lines[2], 21, "   last    %3d.%03d L", i_fuel_consump_last / 100000, (i_fuel_consump_last % 100000) / 100);
    snprintf(lines[3], 21, "   last     %02d:%02d:%02d", fpump_on_duration_prev  / 1000 / 60 / 60
                                                    , fpump_on_duration_prev  / 1000 / 60
                                                    , fpump_on_duration_prev  / 1000 % 60
            );

    snprintf(lines[4], 21, "  today      %3d.%03d L", i_fuel_consump_today / 100000, (i_fuel_consump_today % 100000) / 100);
    snprintf(lines[5], 21, "  today     %02d:%02d:%02d", fpump_today_time / 60 / 60  
                                                    , fpump_today_time / 60
                                                    , fpump_today_time % 60
            );

    snprintf(lines[6], 21, "   prev      %3d.%03d L", i_fuel_consump_prev / 100000, (i_fuel_consump_prev % 100000) / 100);
    snprintf(lines[7], 21, "   prev     %02d:%02d:%02d", fpump_prev_time / 60 / 60
                                                    , fpump_prev_time / 60
                                                    , fpump_prev_time % 60
            );

    snprintf(lines[8], 21, "  total    %3d.%03d L", i_fuel_consump_total / 100000, (i_fuel_consump_total % 100000) / 100);
    snprintf(lines[9], 21, "  total     %02d:%02d:%02d", fpump_work_time / 60 / 60
                                                    , fpump_work_time / 60
                                                    , fpump_work_time % 60
            );

    lcd_print(1, lines[lcd_kotel1_page_line] ); 
    lcd_print(2, lines[lcd_kotel1_page_line+1] ); 
    lcd_print(3, lines[lcd_kotel1_page_line+2] ); 


    //strcpy(str, "lastday,L:   %3d.%03d", i_fuel_consump_total / 100000, (i_fuel_consump_total % 100000) / 100);
    //lcd_print(3, str);         
}

void show_page_kotel2_rate()
{
    char str[30];
    lcd_print(0, "*** KOTEL2 RATE ****");

    snprintf(str, 21, "  today,kWh: %5d.%1d", 0, 0);
    lcd_print(1, str);

    snprintf(str, 21, "lastday,kWh: %5d.%1d", 0, 0);
    lcd_print(2, str);    

    snprintf(str, 21, "  total,kWh: %5d.%1d", 0, 0);
    lcd_print(3, str);

    //strcpy(str, "lastday,L:   %3d.%03d", i_fuel_consump_total / 100000, (i_fuel_consump_total % 100000) / 100);
    //lcd_print(3, str);         
}

void show_page_work_mode()
{
    char str[30];
    lcd_print(0, "**** WORK MODE *****");

    memset(str, SYMBOL_SPACE, 30);
    lcd_print(1, str);
    lcd_print(3, str);

    switch ( work_mode )
    {
        case MODE_MANUAL:
            strcpy(str, "       MANUAL       ");
            break;

        case MODE_AUTO:
            strcpy(str, "        AUTO        ");
            break;            

        case MODE_KOTEL1:
            strcpy(str, "       KOTEL1       ");
            break; 

        case MODE_KOTEL2:
            strcpy(str, "       KOTEL2       ");
            break;    

        default:   
            strcpy(str, "      UNKNOWN       ");
            break;                 
    }

    lcd_print(2, str);
}

void show_page_schedule()
{
    char str[30];
    lcd_print(0, "***** SCHEDULE *****");

    memset(str, SYMBOL_SPACE, 30);
    lcd_print(1, str);
    lcd_print(3, str);

    snprintf(str, 21, "        %3s         ", schedule ? " ON" : "OFF");
    lcd_print(2, str);
}

void show_page_tempset()
{
    char str[30];
    lcd_print(0, "***** TEMPSET *****");

    memset(str, SYMBOL_SPACE, 21);
    lcd_print(1, str);
    

    //snprintf(str, 21, "  Setpoint:    %2d.%1d%c", THERMO_SETPOINT(1) / 10, THERMO_SETPOINT(1) % 10, SYMBOL_DEGREE);
    snprintf(str, 21, "Setpoint:      %2d.%1d%c", TEMPSET / 10, TEMPSET % 10, SYMBOL_DEGREE);
    lcd_print(2, str);

    snprintf(str, 21, "Schedule:        %3s", schedule ? " ON" : "OFF");
    lcd_print(3, str);
}

void show_page_hyst()
{
    char str[30];

    lcd_print(0, "**** HYSTERESIS ****");

    memset(str, SYMBOL_SPACE, 21);
    lcd_print(1, str);
    lcd_print(3, str);

    snprintf(str, 21, "Hysteresis:    %2d.%1d%c", THERMO_HYSTERESIS(1) / 10, THERMO_HYSTERESIS(1) % 10, SYMBOL_DEGREE);
    lcd_print(2, str);
}

void show_pump_settings()
{
    char str[30];

    lcd_print(0, "*** PUMP SETTINGS **");

    //memset(str, SYMBOL_SPACE, 21);
    char smode[13] = "";
    if ( PUMP_MODE == PUMP_MODE_NONE)   strcat(smode, "manual off");
    else if ( PUMP_MODE == PUMP_MODE_1) strcat(smode, "relay off");
    else if ( PUMP_MODE == PUMP_MODE_2) strcat(smode, "delay off");
    else if ( PUMP_MODE == PUMP_MODE_3) strcat(smode, "T return off");

    static uint32_t ii = 0;
    ii++;
    if ( lcd_pump_options == 0 && lcd_pump_in_menu && ( ii % 2 == 0) ) {
        snprintf(str, 21, "%sMode: %13s", " ", " "); 
    } else {
        snprintf(str, 21, "%sMode: %13s", lcd_pump_options == 0 ? ">" : " ", smode);    
    }
    lcd_print(1, str);  

    if ( lcd_pump_options == 1 && lcd_pump_in_menu && ( ii % 2 == 0) ) {
        snprintf(str, 21, "%sSetpoint:     %2s.%1s%c", " ", "  ", " ", SYMBOL_DEGREE);       
    } else {
        snprintf(str, 21, "%sSetpoint:     %2d.%1d%c", lcd_pump_options == 1 ? ">" : " ", THERMO_SETPOINT(3) / 10, THERMO_SETPOINT(3) % 10, SYMBOL_DEGREE);
    }  
    lcd_print(2, str);  

    if ( lcd_pump_options == 2 && lcd_pump_in_menu && ( ii % 2 == 0) ) {
        snprintf(str, 21, "%sHysteresis:   %2s.%1s%c", " ", "  ", " ", SYMBOL_DEGREE);      
    } else {
        snprintf(str, 21, "%sHysteresis:   %2d.%1d%c", lcd_pump_options == 2 ? ">" : " ", THERMO_HYSTERESIS(3) / 10, THERMO_HYSTERESIS(3) % 10, SYMBOL_DEGREE);
    }
    lcd_print(3, str);      
}

void show_page_version()
{

    char str[21];
    memset(str, SYMBOL_SPACE, 21);
    snprintf(str, 21, "Hostname:%11s", sensors_param.hostname);
    lcd_print(0, str);

    memset(str, SYMBOL_SPACE, 21);
    snprintf(str, 21, "firmware     v.%5s", FW_VER);
    lcd_print(1, str);

    snprintf(str, 21, "FreeHeap: %10d", esp_get_free_heap_size());
    lcd_print(2, str);

    memset(str, SYMBOL_SPACE, 21);
    tcpip_adapter_ip_info_t local_ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &local_ip);
    snprintf(str,21, "IP: %d.%d.%d.%d     " , IP2STR( &local_ip.ip) );
    lcd_print(3, str);   
    
}

void show_page(uint8_t idx)
{
    if ( lcd_splash  ) 
    {
        lcd_show_splash(LCD_SPLASH_TIMEOUT  );
        return;
    }

    switch ( idx ) 
    {
        case PAGE_MAIN: 
            show_main_page(); 
            break;

        case PAGE_KOTEL1_RATE:
            show_page_kotel1_rate();
            break;

        case PAGE_KOTEL2_RATE:
            show_page_kotel2_rate();
            break;

        case PAGE_MENU_WORKMODE:
            show_page_work_mode();
            break;

        case PAGE_MENU_SCHEDULE:
            show_page_schedule();
            break;

        case PAGE_MENU_TEMPSET:
            show_page_tempset();
            break;

        case PAGE_MENU_HYST:
            show_page_hyst();
            break;

        case PAGE_MENU_PUMP_SETTINGS:
            show_pump_settings();
            break;

        case PAGE_MENU_VERSION:
            show_page_version();
            break;
            
        case PAGE_MAX: 
            show_main_page(); 
            break;
        default: 
            show_main_page(); 
            break;
    } 
}

// ********************************************************************************
// ************ ФУНКЦИИ УПРАВЛЕНИЯ ТЕРМОСТАТАМИ И УСТАВКАМИ ***********************
// ********************************************************************************
void tempset_dec(uint8_t _schedule)
{
    //uint16_t setpoint = THERMO_SETPOINT(1);
    TEMPSET -= TEMPSET_STEP;

    if ( TEMPSET < TEMPSET_MIN ) {
        TEMPSET = TEMPSET_MIN;
    } 

    if ( _schedule ) return;
    THERMO_TEMP_SET(1, TEMPSET);
    THERMO_TEMP_SET(2, TEMPSET);
}

void tempset_inc(uint8_t _schedule)
{
    //uint16_t setpoint = THERMO_SETPOINT(1);
    TEMPSET += TEMPSET_STEP;

    if ( TEMPSET > TEMPSET_MAX ) {
        TEMPSET = TEMPSET_MAX;
    } 

    if ( _schedule ) return;
    THERMO_TEMP_SET(1, TEMPSET);
    THERMO_TEMP_SET(2, TEMPSET);
}

void hyst_dec()
{
    uint16_t hyst = THERMO_HYSTERESIS(1);
    hyst -= HYST_STEP;

    if ( hyst < HYST_MIN ) {
        hyst = HYST_MIN;
    } 

    THERMO_HYST_SET(1, hyst);
    THERMO_HYST_SET(2, hyst);
}

void hyst_inc()
{
    uint16_t hyst = THERMO_HYSTERESIS(1);
    hyst += HYST_STEP;

    if ( hyst > HYST_MAX ) {
        hyst = HYST_MAX;
    } 

    THERMO_HYST_SET(1, hyst);
    THERMO_HYST_SET(2, hyst);
}

void pump_mode_dec(){
    if ( PUMP_MODE > 0 ) PUMP_MODE--;
}

void pump_mode_inc(){
    PUMP_MODE++;
  if ( PUMP_MODE >= PUMP_MODE_MAX ) PUMP_MODE = PUMP_MODE_MAX - 1;
}

void pump_tempset_dec(){
    uint16_t setpoint = THERMO_SETPOINT(3);
    setpoint -= TEMPSET_STEP;

    if ( setpoint < TEMPSET_MIN ) {
        setpoint = TEMPSET_MIN;
    } 

    THERMO_TEMP_SET(3, setpoint);
    THERMO_TEMP_SET(4, setpoint);    
}

void pump_tempset_inc(){
    uint16_t setpoint = THERMO_SETPOINT(3);
    setpoint += TEMPSET_STEP;

    if ( setpoint > TEMPSET_MAX ) {
        setpoint = TEMPSET_MAX;
    } 

    THERMO_TEMP_SET(3, setpoint);
    THERMO_TEMP_SET(4, setpoint);    
}

void pump_hyst_dec(){
    uint16_t hyst = THERMO_HYSTERESIS(3);
    hyst -= HYST_STEP;

    if ( hyst < HYST_MIN ) {
        hyst = HYST_MIN;
    } 

    THERMO_HYST_SET(3, hyst);
    THERMO_HYST_SET(4, hyst);
}

void pump_hyst_inc(){
    uint16_t hyst = THERMO_HYSTERESIS(3);
    hyst += HYST_STEP;

    if ( hyst > HYST_MAX ) {
        hyst = HYST_MAX;
    } 

    THERMO_HYST_SET(3, hyst);
    THERMO_HYST_SET(4, hyst);    
}


void switch_schedule()
{
    static uint8_t prev = 0;    
    
    schedule = 1 - schedule;
    
    if ( schedule ) 
    {
        set_tempset_by_schedule();

    } else {
        // расписание выключено, установим глобальную уставку
        THERMO_TEMP_SET(1, TEMPSET);
        THERMO_TEMP_SET(2, TEMPSET);   
    }


   if ( prev != schedule)
         nvs_param_save_u32(SPACE_NAME, PARAM_NAME_SCHEDULE, &schedule);

    prev = schedule;

}

int8_t schedule_id;

void set_tempset_by_schedule(uint8_t _schedule)
{
    if ( !_schedule ) return;
    
    // проверить день недели

    // цикл по элементам
    // текущее время сравнить с установленным, 
    // если текущее меньше больше установленного, то взять текущую уставку
    // и так пройтись до конца расписания, перезаписывая уставку в термостаты
    // если не нашлось ни одной уставки в расписании, то выставить глобальную уставку
    // #maxscher - переменная прошивки, кол-во расписаний 

    uint16_t local_tempset = TEMPSET;  // по дефолту из глобальной уставки возьмем
    
    schedule_id = -1;
    for ( uint8_t si = 0; si < maxscher; si++)
    {
        // проверяем день недели
        if ( BIT_CHECK( sensors_param.schweek[si], time_loc.dow ) ) 
        {
            // день недели включен в шедулере
            // теперь сравним время
            uint16_t sched_t = sensors_param.scheduler[si][1]*60 + sensors_param.scheduler[si][2];
            uint16_t loc_t = time_loc.hour * 60 + time_loc.min;
            
            if ( loc_t  >= sched_t ) 
            {
                local_tempset = sensors_param.scheduler[si][3];
                schedule_id = si;
            } 

        }
    }

    shed_tempset = local_tempset;
    THERMO_TEMP_SET(1, local_tempset);
    THERMO_TEMP_SET(2, local_tempset);  
    
}


//#define pump_mode PUMP_MODE
#define  PARAM_NAME_PUMP_MODE "pumpmode"

void set_pump_mode(uint8_t _mode)
{
    if ( _mode >= PUMP_MODE_MAX || _mode == PUMP_MODE ) return;
    PUMP_MODE = _mode;
    //nvs_param_save_u32(SPACE_NAME, PARAM_NAME_PUMP_MODE, &pump_mode);
}

void control_return_water_thermostats()
{
    if ( PUMP_MODE == PUMP_MODE_NONE ) return;
    
    // управление термостатами воды
    // включаем насос всегда при включении реле котла

    // выключаем по режиму работы
    if ( PUMP_MODE == PUMP_MODE_1 )
    {
        // * 1. реле насоса вкл/выкл по реле котла
        if ( THERMO_STATE(3) ) THERMO_OFF(3);
        if ( THERMO_STATE(4) ) THERMO_OFF(4);
        GPIO_ALL( PUMP1_GPIO, GPIO_ALL_GET(KOTEL1_GPIO));
        GPIO_ALL( PUMP2_GPIO, GPIO_ALL_GET(KOTEL2_GPIO));  
    }
    else if ( PUMP_MODE == PUMP_MODE_2 )
    {
        // * 2. реле насоса вкл по реле котла, выкл через Х минут, после выключения реле котла
        if ( THERMO_STATE(3) ) THERMO_OFF(3);
        if ( THERMO_STATE(4) ) THERMO_OFF(4);

        if ( GPIO_ALL_GET(KOTEL1_GPIO) ) GPIO_ALL( PUMP1_GPIO, 1);
        if ( GPIO_ALL_GET(KOTEL2_GPIO) ) GPIO_ALL( PUMP2_GPIO, 1);     

        static uint32_t t1, t2 = 0;

        if ( !GPIO_ALL_GET(KOTEL1_GPIO) && ( millis() - t1 > PUMP1_OFF_TIMEOUT*1000) ) {
            GPIO_ALL( PUMP1_GPIO, 0);
            t1 = millis();
        } else if ( GPIO_ALL_GET(KOTEL1_GPIO) ) t1 = millis();

        if ( !GPIO_ALL_GET(KOTEL2_GPIO) && ( millis() - t2 > PUMP2_OFF_TIMEOUT*1000) ) {
            GPIO_ALL( PUMP2_GPIO, 0);
            t2 =millis();
        }  else if ( GPIO_ALL_GET(KOTEL2_GPIO) ) t2 = millis();        
    }
    else if ( PUMP_MODE == PUMP_MODE_3 )
    {
        // при температуре между нижней и верхней границей постоянно вкл - выкл
        if ( GPIO_ALL_GET(KOTEL1_GPIO) && !THERMO_STATE(3)) THERMO_ON(3);
        if ( GPIO_ALL_GET(KOTEL2_GPIO) && !THERMO_STATE(4)) THERMO_ON(4);

        // * 3. реле насоса вкл по реле котла, выкл по температуре обратки
        if ( !GPIO_ALL_GET(KOTEL1_GPIO) ) {
            // котел выключили, ждем понижения температуры обратки
            // т.е отключения реле насоса и выключаем термостат воды
            if ( GPIO_ALL_GET(PUMP1_GPIO) == 0 || GPIO_ALL_GET(KOTEL2_GPIO) == 1) THERMO_OFF(3);
        }

        if ( !GPIO_ALL_GET(KOTEL2_GPIO) ) {
            // котел выключили, ждем понижения температуры обратки
            // или отключения реле насоса и выключаем термостат воды
            if ( GPIO_ALL_GET(PUMP2_GPIO) == 0 || GPIO_ALL_GET(KOTEL1_GPIO) == 1) THERMO_OFF(4);
        }  
    }


}

void set_active_kotel(mode_e mode)
{
    switch ( mode ) {
        case MODE_MANUAL:
            active_kotel = KOTEL_NONE;
            break;
        case MODE_AUTO:
            // выбираем по времени
            active_kotel = ( time_loc.hour >= DAY_TIME && time_loc.hour < NIGHT_TIME) ? KOTEL_1 : KOTEL_2;             
            break;
        case MODE_KOTEL1: 
            active_kotel = KOTEL_1;          
            break;        
        case MODE_KOTEL2: 
            active_kotel = KOTEL_2;       
            break;
        default:      
            active_kotel = KOTEL_NONE;
    } 

    if ( mode != MODE_MANUAL) 
    {
        GPIO_ALL(100, active_kotel == KOTEL_1 );
        if ( active_kotel != KOTEL_1 ) GPIO_ALL(KOTEL1_GPIO, 0 );

        GPIO_ALL(101, active_kotel == KOTEL_2 );
        if ( active_kotel != KOTEL_2 ) GPIO_ALL(KOTEL2_GPIO, 0 );        
    } else {
        GPIO_ALL(100, 0 );
        GPIO_ALL(101, 0 );
    }
}

void set_work_mode(uint8_t _mode)
{
    if ( _mode >= MODE_MAX || _mode == work_mode ) return;
    static uint8_t prev = 0;    
    if ( prev != work_mode )
        nvs_param_save_u32(SPACE_NAME, PARAM_NAME_WORKMODE, &work_mode); 

    set_active_kotel( work_mode );
    prev = work_mode;
}

void change_work_mode()
{
    
    if ( display_alert == 1 ) return;

    work_mode++;
    if ( work_mode >= MODE_MAX ) work_mode = MODE_MANUAL;

    set_work_mode(work_mode);
}

void change_work_mode_back()
{
    if ( display_alert == 1 ) return;

    if ( work_mode == MODE_MANUAL ) work_mode = MODE_MAX;
    work_mode--;

    set_work_mode( work_mode );
}

// *******************************************************************************
// ************** ОБРАБОТЧИК ПРЕРЫВАНИЙ MCP23017 *********************************
// *******************************************************************************
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


#define MCP23017_INTA_PIN 4     // pin esp
TaskHandle_t mcp23017_task;
QueueHandle_t mcp23017_queue;

typedef void (*interrupt_cb)(void *arg, uint8_t *state);

typedef struct mcp23017_pin_isr {
        uint8_t pin;
        interrupt_cb pin_cb;
        void *args;

        interrupt_cb pin_cb2;
        void *args2;

        gpio_int_type_t intr_type;
        
        uint32_t up_delay_ms;
        uint32_t ts_down;
        uint32_t ts_up;

} mcp23017_pin_isr_t;

mcp23017_pin_isr_t *pin_isr; // указатель на массив коллбеков для пинов
uint8_t pin_isr_cnt;

static void IRAM_ATTR mcp23017_isr_handler(void *arg) {
    portBASE_TYPE HPTaskAwoken = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken;

    uint16_t data[2];

    data[0] = MCPread_reg16(0, INTFA); // считываем данные с mcp23017
    data[1] = MCPread_reg16(0, INTCAPA); // считываем данные с mcp23017 // чтение снимка ножек при прерывании сбрасывает прерывание

    static uint32_t t = 0;

    if ( millis() - t >= MCP23017_ISR_DELAY_MS )
    {
        xQueueOverwriteFromISR(mcp23017_queue, &data, &xHigherPriorityTaskWoken);
        t = millis();
    }
    portEND_SWITCHING_ISR( HPTaskAwoken == pdTRUE );
}

static void mcp23017_isr_cb(void *arg) {
	while (1) {

            uint16_t data[2];
            if ( xQueueReceive(mcp23017_queue, &data, 0) == pdPASS) 
            {
                //ESP_LOGI(UTAG, " interrput: %4d \t 0x%04X \t " BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN, data[0], data[0], BYTE_TO_BINARY(data[0] >> 8), BYTE_TO_BINARY(data[0]));
                //ESP_LOGI(UTAG, "gpio state: %4d \t 0x%04X \t " BYTE_TO_BINARY_PATTERN BYTE_TO_BINARY_PATTERN, data[1], data[1], BYTE_TO_BINARY(data[1] >> 8), BYTE_TO_BINARY(data[1]));

                // check pins with interrupts
                for ( uint8_t i = 0; i < 16; i++)
                {  
                    if ( BIT_CHECK( data[0], i) != 0)
                    {
                        // check pin state
                        uint8_t state = BIT_CHECK( data[1], i) != 0;                        
                        //ESP_LOGI(UTAG, "pin = %d, state = %d", i+1, state);                        
                        
                        // поиск коллбека для нажатия кнопок
                        for ( uint8_t j = 0; j < pin_isr_cnt; j++)
                        {
                            if ( pin_isr[ j ].pin == i )
                            {
                                //ESP_LOGI(UTAG, "pin = %d, state = %d, intr type: %d", i+1, state, pin_isr[ j ].intr_type);  
                                if ( ( state == 0 && pin_isr[ j ].intr_type == GPIO_INTR_POSEDGE) || 
                                     ( state == 1 && pin_isr[ j ].intr_type == GPIO_INTR_NEGEDGE))
                                {
                                    pin_isr[ j ].ts_down = millis();
                                    pin_isr[ j ].ts_up = millis();
                                }

                                if (( state == 1 && pin_isr[ j ].intr_type == GPIO_INTR_POSEDGE) || 
                                    ( state == 0 && pin_isr[ j ].intr_type == GPIO_INTR_NEGEDGE))
                                {
                                    pin_isr[ j ].ts_up = millis();
                                    // execute callback
                                    if ( ( pin_isr[ j ].ts_up - pin_isr[ j ].ts_down <= pin_isr[ j ].up_delay_ms ) 
                                         || 
                                         pin_isr[ j ].pin_cb2 == NULL)
                                    {
                                        pin_isr[ j ].pin_cb( pin_isr[ j ].args, &state );
                                    } else {
                                        if ( pin_isr[ j ].pin_cb2 != NULL )
                                            pin_isr[ j ].pin_cb2( pin_isr[ j ].args2, &state );
                                    }
                                } 
                                else if ( pin_isr[ j ].intr_type == GPIO_INTR_ANYEDGE )
                                {
                                    pin_isr[ j ].pin_cb( pin_isr[ j ].args, &state );
                                }
                            }
                        }                               
                    }
                }
            }
        vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
    vTaskDelete(NULL);
}

esp_err_t mcp23017_isr_handler_add(uint8_t pin, gpio_int_type_t intr_type, interrupt_cb cb, void *args, interrupt_cb cb2, void *args2, uint32_t up_delay)
{
    if ( cb == NULL ) return ESP_FAIL;

    pin_isr_cnt++;
    pin_isr = (mcp23017_pin_isr_t *) realloc(pin_isr, pin_isr_cnt * sizeof(mcp23017_pin_isr_t));
    if ( pin_isr == NULL ) {
        pin_isr_cnt--;
        return ESP_FAIL;
    }

    pin_isr[ pin_isr_cnt-1 ].pin = pin;
    pin_isr[ pin_isr_cnt-1 ].pin_cb = cb;
    pin_isr[ pin_isr_cnt-1 ].intr_type = intr_type;
    pin_isr[ pin_isr_cnt-1 ].args = args;

    pin_isr[ pin_isr_cnt-1 ].pin_cb2 = cb2;
    pin_isr[ pin_isr_cnt-1 ].args2 = args2;

    pin_isr[ pin_isr_cnt-1 ].up_delay_ms = up_delay;

    return ESP_OK;     
}
// *******************************************************************************
// *********** БЛОК ФУНКЦИЙ ОБРАБОТЧИКОВ НАЖАТИЯ КНОПОК MCP23017 *****************
// *******************************************************************************


void mcp23017_button_isr_cb(uint8_t pin, uint8_t *state)
{
    //ESP_LOGI(UTAG, "%s: pin %d, state %d", __func__, pin, *state);
    GPIO_ALL(pin, !GPIO_ALL_GET(pin));
}

void mcp23017_pir_sensor_cb(uint8_t pin, uint8_t *state)
{
    //ESP_LOGI(UTAG, "%s: pin %d, state %d", __func__, pin, *state);

    GPIO_ALL(pin, *state);
}

void button1_short_press(void *args, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    if ( backlight == 0 && sensors_param.lcden > 0) return;

    buzzer( BUZZER_BEEP_SHORT ); 

    switch ( menu_idx ) {
        case PAGE_MAIN:
                if ( work_mode != MODE_MANUAL ) {
                    if ( schedule ) {
                        buzzer( BUZZER_BEEP_ERROR );
                        show_display_alert("   *** ERROR ***    ", "Schedule is enabled. Can't change temperature setpiont!", NULL);
                    } else {
                        tempset_dec(schedule);
                        show_display_alert(NULL, NULL, show_page_tempset);
                    } 
                } else {
                    GPIO_INVERT( KOTEL1_GPIO );
                }
            break;
        case PAGE_KOTEL1_RATE:
            // TODO: scroll to top
            if ( lcd_kotel1_page_line > 0 ) lcd_kotel1_page_line--;
            else buzzer( BUZZER_BEEP_ERROR );
            break;
        case PAGE_KOTEL2_RATE:
            // TODO: scroll to bottom
            break;
        case PAGE_MENU_WORKMODE:
            change_work_mode_back();
            break;
        case PAGE_MENU_SCHEDULE:
            switch_schedule();
            break;
        case PAGE_MENU_TEMPSET:
            tempset_dec(schedule);
            break;
        case PAGE_MENU_HYST:
            hyst_dec();
            break;
        case PAGE_MENU_PUMP_SETTINGS:
            if ( lcd_pump_in_menu == 0 ) {
                lcd_pump_options++;
                if ( lcd_pump_options > LCD_PUMP_OPTIONS-1 ) lcd_pump_options = 0;
            } else {
                // change 
                if ( lcd_pump_options == 0 ) {
                    // change pump mode
                    pump_mode_dec();
                } else if ( lcd_pump_options == 1) {
                    pump_tempset_dec();
                } else if ( lcd_pump_options == 2 ) {
                    pump_hyst_dec();
                }
            }
            
            break;
        case PAGE_MENU_VERSION:
            break;
        default:
            break;
    }
     
    last_key_press = millis();  
}

void button1_long_press(void *args, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;   
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    buzzer( BUZZER_BEEP_DOUBLE_SHORT );

    switch ( menu_idx ) {
        case PAGE_MAIN:
            change_work_mode();
            break;
        case PAGE_KOTEL1_RATE:
            fuel_reset_data();
            break;
        case PAGE_KOTEL2_RATE:
            // TODO: do nothing
            break;
        case PAGE_MENU_WORKMODE:
            break;
        case PAGE_MENU_SCHEDULE:
            break;
        case PAGE_MENU_TEMPSET:
            break;
        case PAGE_MENU_HYST:
            break;
        case PAGE_MENU_PUMP_SETTINGS:
            break;
        case PAGE_MENU_VERSION:
            break;
        default:
            break;
    }

    last_key_press = millis();
}

void button2_short_press(uint8_t pin, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    if ( backlight == 0 && sensors_param.lcden > 0) return;

    buzzer( BUZZER_BEEP_SHORT ); 

    switch ( menu_idx ) {
        case PAGE_MAIN:
                if ( work_mode != MODE_MANUAL ) {
                    if ( schedule ) {
                        buzzer( BUZZER_BEEP_ERROR );
                        show_display_alert("   *** ERROR ***    ", "Schedule is enabled. Can't change temperature setpiont!", NULL);
                    } else {
                        tempset_inc(schedule);
                        show_display_alert(NULL, NULL, show_page_tempset);
                    } 
                } else {
                    GPIO_INVERT( KOTEL2_GPIO );
                }
            break;
        case PAGE_KOTEL1_RATE:
            // TODO: scroll to bottom
            lcd_kotel1_page_line++;
            if ( lcd_kotel1_page_line == (LCD_KOTEL1_PAGE_LINES_CNT-2) ) {
                lcd_kotel1_page_line = 0;
                buzzer( BUZZER_BEEP_ERROR );
            }            
            break;
        case PAGE_KOTEL2_RATE:
            // TODO: scroll to bottom
            break;
        case PAGE_MENU_WORKMODE:
            change_work_mode();
            break;
        case PAGE_MENU_SCHEDULE:
            switch_schedule();
            break;
        case PAGE_MENU_TEMPSET:
            tempset_inc(schedule);
            break;
        case PAGE_MENU_HYST:
            hyst_inc();
            break;
        case PAGE_MENU_PUMP_SETTINGS:
            // enter to menu    
            if ( lcd_pump_in_menu )
            {
                if ( lcd_pump_options == 0 ) {
                    // change pump mode
                    pump_mode_inc();
                } else if ( lcd_pump_options == 1) {
                    pump_tempset_inc();
                } else if ( lcd_pump_options == 2 ) {
                    pump_hyst_inc();
                }
            }
            break;
        case PAGE_MENU_VERSION:
            break;
        default:
            break;
    }

    last_key_press = millis();
}

void button2_long_press(uint8_t pin, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    buzzer( BUZZER_BEEP_DOUBLE_SHORT );

    if ( menu_idx == PAGE_MAIN) {
            GPIO_INVERT(ESC_GPIO);
            char str[21];
            snprintf(str, 21, "  Night mode is %3s ", GPIO_ALL_GET( ESC_GPIO ) ? "ON " : "OFF");
            show_display_alert("                    ", str, NULL);      
    } else {

    }      

    last_key_press = millis();
}

void button3_short_press(uint8_t pin, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    //if ( backlight == 0 && sensors_param.lcden > 0) return;
    buzzer( BUZZER_BEEP_SHORT );

    if ( menu_idx == PAGE_MAIN) {
        GPIO_INVERT( VENT_GPIO );
        char str[21];
        snprintf(str, 21, " Ventilation is %3s ", GPIO_ALL_GET( VENT_GPIO ) ? "ON " : "OFF");
        show_display_alert("                    ", str, NULL);
    } else {
        if ( menu_idx == PAGE_MENU_PUMP_SETTINGS && lcd_pump_in_menu )
        {

        }
        else
        {
            lcd_pump_options = 0;
            menu_prev();
        }    
    }

    last_key_press = millis();
    
}

void button3_long_press(uint8_t pin, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    buzzer( BUZZER_BEEP_DOUBLE_SHORT );    
    if ( menu_idx == PAGE_MAIN) {
            switch_schedule();
            char str[21];
            snprintf(str, 21, "   Schedule is %3s  ", schedule ? "ON " : "OFF");
            show_display_alert("                    ", str, NULL);            
    } else if ( menu_idx == PAGE_MENU_PUMP_SETTINGS ) {
        lcd_pump_in_menu = 1- lcd_pump_in_menu;
    } else {

    }
    last_key_press = millis();
}

void button4_short_press(uint8_t pin, uint8_t *state)
{
    uint8_t backlight = LCD_BACKLIGHT_STATE;
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);
    //if ( backlight == 0 && sensors_param.lcden > 0) return;

    if ( menu_idx == PAGE_MENU_PUMP_SETTINGS && lcd_pump_in_menu )
    {

    } else {
        lcd_pump_options = 0;
        menu_next();        
    }

    buzzer( BUZZER_BEEP_SHORT );
    last_key_press = millis();
}

void button4_long_press(uint8_t pin, uint8_t *state)
{
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL); 
    buzzer( BUZZER_BEEP_DOUBLE_SHORT );  

    if ( menu_idx == PAGE_MAIN )
    {
            //  PUMP's off manualy!!!  TODO off pump termostats (3,4)
        GPIO_ALL( PUMP1_GPIO, 0);
        GPIO_ALL( PUMP2_GPIO, 0);
    } else {
        buzzer( BUZZER_BEEP_DOUBLE_SHORT ); 
        menu_idx = PAGE_MAIN;     
    } 

    last_key_press = millis(); 
}

void mcp23017_init()
{
    // установить прерывания пинов
    MCPwrite_reg16(0, GPINTENA, 0b1111111111111111); // 0b0000111000000000

    // условия сработки прерывания на ногах
    MCPwrite_reg16(0, INTCONA, 0);  // при нулях

    // дефолтные значения ног, прерывание сработает, если на ноге сигнал отличается от дефолтного, если на пинах значение отличается от  заданного ниже (DEFVAL  = 1 )
    MCPwrite_reg16(0, DEFVALA, 0b1111111111111111);    

    // установить прерывания на GPIO
    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_INTR_NEGEDGE; //GPIO_INTR_NEGEDGE; //GPIO_INTR_POSEDGE; // GPIO_INTR_ANYEDGE;           
    gpio_conf.mode = GPIO_MODE_INPUT;
    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_conf.pin_bit_mask = (1ULL << MCP23017_INTA_PIN);
    gpio_config(&gpio_conf);    
    gpio_install_isr_service(0);

    // прерывание на кнопки mcp23017
    gpio_isr_handler_add( MCP23017_INTA_PIN, mcp23017_isr_handler, NULL);  

        // 1 - сразу при нажатии         GPIO_INTR_POSEDGE 
        // 2 - только после отпускания   GPIO_INTR_NEGEDGE 
        // 3 - любое состояние           GPIO_INTR_ANYEDGE 
        // или наоборот, зависит от дефолтного состояния пина 1 = 1 или 0 = 2
    mcp23017_isr_handler_add( 0, GPIO_INTR_POSEDGE, button1_short_press, NULL,   button1_long_press,     NULL, 800);
    mcp23017_isr_handler_add( 1, GPIO_INTR_POSEDGE, button2_short_press, NULL,   button2_long_press,     NULL, 800);
    mcp23017_isr_handler_add( 2, GPIO_INTR_POSEDGE, button3_short_press, NULL,   button3_long_press,     NULL, 800);
    mcp23017_isr_handler_add( 3, GPIO_INTR_POSEDGE, button4_short_press, NULL,   button4_long_press,     NULL, 800);

    mcp23017_queue = xQueueCreate(5, sizeof(uint16_t) * 2);
    xTaskCreate( mcp23017_isr_cb, "mcp23017_isr", 1024, NULL, 10, &mcp23017_task);
}
// *******************************************************************************

void control_indications()
{
    // индикации 
    
    if ( work_mode != MODE_MANUAL )
    {
        // термостат котла 1
        GPIO_ALL( KOTEL1_LED_GPIO, THERMO_STATE(1) );

        // тормостат котла 2
        GPIO_ALL( KOTEL2_LED_GPIO, THERMO_STATE(2) );        
    } else {
        // в ручном режиме
        GPIO_ALL( KOTEL1_LED_GPIO, GPIO_ALL_GET( KOTEL1_GPIO ) );
        GPIO_ALL( KOTEL2_LED_GPIO, GPIO_ALL_GET( KOTEL2_GPIO ) );
    }

    // индикация работы реле котлов, подсвечиваем, если хотя бы одно реле включено
    if ( GPIO_ALL_GET( KOTEL1_GPIO) == 0 && GPIO_ALL_GET( KOTEL2_GPIO ) == 0)
        GPIO_ALL( KOTEL_LED_GPIO, 0 );
    else
        GPIO_ALL( KOTEL_LED_GPIO, 1 );

    // индикация работы насоса, подсвечиваем, если хотя бы один насос включен
    if ( GPIO_ALL_GET( PUMP1_GPIO) == 0 && GPIO_ALL_GET( PUMP2_GPIO ) == 0)
        GPIO_ALL( PUMP_LED_GPIO, 0 );
    else
        GPIO_ALL( PUMP_LED_GPIO, 1 );

}

void webfunc_print_kotel_data(char *pbuf)
{
    os_sprintf(HTTPBUFF,"<table>");
    
    os_sprintf(HTTPBUFF,"<tr><td>Temperature:</td><td><b>%d.%d °C</b></td></tr>", current_temp / 10, current_temp % 10); 

    os_sprintf(HTTPBUFF, "<tr>"
                        "<td>Mode:</td>");
 
    #define html_button_mode "<td><a href='#' onclick='wm(%d)'><div class='g_%d k kk fll wm' id='v%d'>%s</div></a></td>"

    os_sprintf(HTTPBUFF, html_button_mode, MODE_MANUAL, work_mode == MODE_MANUAL,   MODE_MANUAL, "Manual");
    os_sprintf(HTTPBUFF, html_button_mode, MODE_AUTO,   work_mode == MODE_AUTO,     MODE_AUTO, "Auto");
    os_sprintf(HTTPBUFF, html_button_mode, MODE_KOTEL1, work_mode == MODE_KOTEL1,   MODE_KOTEL1, "Kotel1");
    os_sprintf(HTTPBUFF, html_button_mode, MODE_KOTEL2, work_mode == MODE_KOTEL2,   MODE_KOTEL2, "Kotel2");
    os_sprintf(HTTPBUFF,"</tr><tr>"
                        "<td>Schedule:</td>");

    os_sprintf(HTTPBUFF, "<td><a id='ushd' href='#' data-val='%d' onclick='schd(this.dataset.val)'><div class='g_%d k kk fll' id='sch' data-text='%s'>%s</div></a></td>"
                        , !schedule
                        , schedule
                        , schedule ? "Off" : "On" //обратное значение, подставится после нажатия
                        , schedule ? "On" : "Off"
                        );   

    os_sprintf(HTTPBUFF,"<td colspan=2 align=right>%s tempset:</td>"
                        "<td><b>%d.%d °C</b></td>"
                        "</tr>"
                        , schedule ? "Schedule" : "Global"
                        , schedule ? (shed_tempset / 10) : TEMPSET / 10
                        , schedule ? (shed_tempset % 10) : TEMPSET % 10
                );   

    if ( schedule ) {
        char weeks[32] = ""; //"#1 hh:mm Mo,Tu,We,Th,Fr,Sa,Su"
        if ( schedule_id > -1 && schedule_id < maxscher )
        {
            sprintf(weeks, " #%d %02d:%02d %s%s%s%s%s%s%s"
                , schedule_id+1 
                , sensors_param.scheduler[schedule_id][1]
                , sensors_param.scheduler[schedule_id][2]
                , BIT_CHECK( sensors_param.schweek[schedule_id], 0 ) ? "Mo " : ""
                , BIT_CHECK( sensors_param.schweek[schedule_id], 1 ) ? "Tu " : ""
                , BIT_CHECK( sensors_param.schweek[schedule_id], 2 ) ? "We " : ""
                , BIT_CHECK( sensors_param.schweek[schedule_id], 3 ) ? "Th " : ""
                , BIT_CHECK( sensors_param.schweek[schedule_id], 4 ) ? "Fr " : ""
                , BIT_CHECK( sensors_param.schweek[schedule_id], 5 ) ? "Sa " : ""
                , BIT_CHECK( sensors_param.schweek[schedule_id], 6 ) ? "Su " : ""
            );
        }
         os_sprintf(HTTPBUFF, "<tr><td colspan=5 align=right>[%s]</td></tr>", weeks);
  
    }
    
    os_sprintf(HTTPBUFF,"</table>");
}

void webfunc_print_script(char *pbuf)
{
    os_sprintf(HTTPBUFF, "<script type='text/javascript'>"


                        "window.onload=function()"
                        "{"
                            "let e=document.createElement('style');"
                            "e.innerText='"
                                                ".kk{border-radius:4px;margin:2px;width:60px;}"
                                                "';"
                            "document.head.appendChild(e)"
                        "};"

                        "function wm(t)"
                        "{"
                            "ajax_request('/valdes?int=%d'+'&set='+t,"
                                "function(res)"
                                "{"
                                    "let v=document.getElementsByClassName('wm');"
                                    "for(let i=0;i<v.length;i++)v[i].classList.replace('g_1','g_0');"
                                    "document.getElementById('v'+t).classList.add('g_1')"
                                "}"
                            ")"
                        "};"

                        "function schd(t)"
                        "{"
                            "ajax_request("
                                "'/valdes?int=%d'+'&set='+t,"
                                "function(res)"
                                    "{"
                                        "var n=1-parseInt(t);"
                                        "var sc=document.getElementById('sch');"
                                        "sc.classList.replace('g_'+n,'g_'+t);"
                                        "sc.innerHTML=sc.getAttribute('data-text');"
                                        "document.getElementById('ushd').setAttribute('data-val',n);"
                                    "}"
                            ")"
                        "}"

                        "</script>"
                    , VALDES_INDEX_WORK_MODE
                    , VALDES_INDEX_SCHEDULE 
    );  
}


//*****************************************************************************************************************
//****************** основные функции прошивки ********************************************************************
//*****************************************************************************************************************
#define ADDLISTSENS {200,LSENSFL0,"WorkMode", PARAM_NAME_WORKMODE, &WORKMODE,NULL}, \
                    {201,LSENSFL1,"Temperature","temp",&current_temp,NULL}, \
                    {202,LSENSFL0,"Schedule",PARAM_NAME_SCHEDULE,&schedule,NULL}, \
                    {203,LSENSFL0,"TempSet",PARAM_NAME_TEMPSET,&TEMPSET,NULL}, \
                    {204,LSENSFL0,"PumpMode",PARAM_NAME_PUMP_MODE,&PUMP_MODE,NULL}, \
                    {205,LSENSFL0,"FuelPump","fuelpump",&fpump_state,NULL}, \
                    {206,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelRate",  "fuelrate",     get_consump_total,NULL}, \
					{207,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelRateT", "fuelratet",    get_consump_today,NULL}, \
					{208,LSENSFL3|LSENS32BIT|LSENSFUNS,"FuelRateY", "fuelratey",    get_consump_prev,NULL}, \
					{209,LSENSFL0|LSENS32BIT,"FuelTime","fueltime",     &fpump_work_time,NULL}, \
					{210,LSENSFL0|LSENS32BIT,"FuelTimeT","fueltimet",   &fpump_today_time,NULL}, \
					{211,LSENSFL0|LSENS32BIT,"FuelTimeY","fueltimey",   &fpump_prev_time,NULL}, \
					{212,LSENSFL0,"FuelOnCnt","foncnt",&fpump_on_cnt,NULL}, \
					{213,LSENSFL0|LSENS32BIT,"FuelOnDur","fondur",&fpump_on_duration_prev,NULL}, 



// ***********************************************************************************
// ************ ПРИЕМ ДАННЫХ ПО MQTT *************************************************
// ***********************************************************************************
void mqtt_receive_data(char *topicBuf,char *dataBuf)
{ 
    ESP_LOGI( UTAG, "%s: %s:%s", __func__, topicBuf, dataBuf );
    char lwt[64];
    uint16_t lentopic = os_sprintf(lwt, "%s/%s" topicwrite "/", sensors_param.mqttlogin, sensors_param.hostname);
    char *topic = (char *)os_strstr( topicBuf, lwt);
    if ( topic != NULL ) 
    {
        topic += lentopic;
        //Тут topic не содержит логин и имя модуля.
        if ( !strcoll( topic, PARAM_NAME_WORKMODE ) ) 
        { 
            // событие прихода топика логин/имя_модуля/testtopic
            // обрабатываем полученное значение топика dataBuf , например через atoi
            uint8_t _work_mode = atoi( dataBuf );
            set_work_mode( _work_mode );          
        }
        else if ( !strcoll( topic, PARAM_NAME_SCHEDULE ) ) 
        { 
            // событие прихода топика логин/имя_модуля/testtopic
            // обрабатываем полученное значение топика dataBuf , например через atoi
            uint8_t _schedule = atoi( dataBuf );
            if ( _schedule == schedule ) return;

            if ( _schedule > 1 ) {
                switch_schedule();
            } else {
                schedule = _schedule;
                if ( schedule ) 
                {
                    set_tempset_by_schedule( schedule );
                } else {
                    // расписание выключено, установим глобальную уставку
                    THERMO_TEMP_SET(1, TEMPSET);
                    THERMO_TEMP_SET(2, TEMPSET);   
                }                
            }
            nvs_param_save_u32(SPACE_NAME, PARAM_NAME_SCHEDULE, &schedule);
        }
        else if ( !strcoll( topic, PARAM_NAME_TEMPSET ) ) 
        { 
            // событие прихода топика логин/имя_модуля/testtopic
            // обрабатываем полученное значение топика dataBuf , например через atoi
            if ( strchr( dataBuf, '.') == NULL)
            {
                TEMPSET = atoi( dataBuf ) ;
            } else {
                // float
                TEMPSET = (uint16_t)(atof(dataBuf) * 10);
            }

            if ( !schedule ) {
                THERMO_TEMP_SET(1, TEMPSET);
                THERMO_TEMP_SET(2, TEMPSET);   
            }          
        }    
        else if ( !strcoll( topic, PARAM_NAME_PUMP_MODE ) ) 
        { 
            uint8_t _mode = atoi( dataBuf );
            set_pump_mode( _mode );      
        }        
    }
}

void startfunc(){
    // выполняется один раз при старте модуля.

    ESP_LOGI(UTAG, "******************** VERSION = %s ****************", FW_VER);

    if ( nvs_param_load(SPACE_NAME, PARAM_NAME_WORKMODE, &work_mode) != ESP_OK ) work_mode = MODE_MANUAL;
    ESP_LOGW(UTAG, "Loaded work mode = %d", work_mode);

    if ( nvs_param_load(SPACE_NAME, PARAM_NAME_SCHEDULE, &schedule) != ESP_OK ) schedule = 0;
    ESP_LOGW(UTAG, "Loaded schedule = %d", schedule);

    uint8_t err = 0;

    if ( KOTEL1_GPIO == 0 || KOTEL1_GPIO >=255 ) { KOTEL1_GPIO = KOTEL1_GPIO_DEFAULT ; err = 1; }
    if ( KOTEL2_GPIO == 0 || KOTEL2_GPIO >= 255 ) { KOTEL2_GPIO = KOTEL2_GPIO_DEFAULT ; err = 1; }
    if ( PUMP1_GPIO == 0 || PUMP1_GPIO >= 255 ) { PUMP1_GPIO = PUMP1_GPIO_DEFAULT ; err = 1; }
    if ( PUMP2_GPIO == 0 || PUMP2_GPIO >= 255 ) { PUMP2_GPIO = PUMP2_GPIO_DEFAULT ; err = 1; }
    if ( ESC_GPIO == 0 || ESC_GPIO >= 255 ) { ESC_GPIO = ESC_GPIO_DEFAULT ; err = 1; }
    if ( VENT_GPIO == 0 || VENT_GPIO >= 255 ) { VENT_GPIO = VENT_GPIO_DEFAULT ; err = 1; }

    if ( NIGHT_TIME >= 23 ) { NIGHT_TIME = NIGHT_TIME_DEFAULT ; err = 1; }
    if ( DAY_TIME >= 23 ) { DAY_TIME = DAY_TIME_DEFAULT ; err = 1; }

    if ( KOTEL1_LED_GPIO == 0 || KOTEL1_LED_GPIO >= 255 ) { KOTEL1_LED_GPIO = 255 ; err = 1; }
    if ( KOTEL2_LED_GPIO == 0 || KOTEL2_LED_GPIO >= 255 ) { KOTEL2_LED_GPIO = 255 ; err = 1; }
    if ( KOTEL_LED_GPIO == 0 || KOTEL_LED_GPIO >= 255 ) { KOTEL_LED_GPIO = 255 ; err = 1; }
    if ( PUMP_LED_GPIO == 0 || PUMP_LED_GPIO >= 255 ) { PUMP_LED_GPIO = 255 ; err = 1; }
    if ( SCHEDULE_LED_GPIO == 0 || SCHEDULE_LED_GPIO >= 255 ) { SCHEDULE_LED_GPIO = 255 ; err = 1; }
    if ( VENT_LED_GPIO == 0 || VENT_LED_GPIO >= 255 ) { VENT_LED_GPIO = 255 ; err = 1; }

    if ( TEMPSET < 100 || TEMPSET > 300 ) { TEMPSET = 240 ; err = 1; }

    if ( BUZZER_GPIO == 0 || BUZZER_GPIO >=255 ) { BUZZER_GPIO = 255 ; err = 1; }
    if ( PUMP_MODE >= PUMP_MODE_MAX ) { PUMP_MODE = PUMP_MODE_1 ; err = 1; }

    if ( err == 1 ) SAVEOPT;

    // buzzer
    buzzer_init();

    mcp23017_init();

    set_active_kotel( work_mode );

    lcd_init2();
    // выключить подсветку черех Х сек
    turn_on_lcd_backlight( BACKLIGHT_GPIO, NULL);

    // читаем сохраненные данные по топливному насосу
    fuel_load_data();

    //nvs_param_load(SPACE_NAME, PARAM_NAME_PUMP_MODE, &pump_mode);

    cb_mqtt_funs = mqtt_receive_data;

}

void timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду

    // if ( timersrc % 600 == 0 ) {
    //     save_params_to_nvs();
    // }

    if ( timersrc % 30 == 0 ) {
        // выполнение кода каждые 30 секунд
        set_active_kotel(work_mode); 
    }

    if ( menu_idx != PAGE_MAIN && ( millis() - last_key_press >= MENU_EXIT_TIMEOUT )) 
    {
        menu_idx = PAGE_MAIN;
        lcd_pump_in_menu = 0;
        lcd_pump_options = 0;
    }

    // управление уставками по расписанию
    if ( timersrc % 30 == 0 ) {
        set_tempset_by_schedule(schedule); 
    }

    show_page( menu_idx );

    control_return_water_thermostats();
    
    // вентиляция
    GPIO_ALL( VENT_LED_GPIO, GPIO_ALL_GET( VENT_GPIO ) );

    // расписание
    GPIO_ALL( SCHEDULE_LED_GPIO, schedule );

    control_indications();

    // работа с топливным насосом
    detect_fuel_pump_work();
    fuel_consumption_calc();
    
    if ( timersrc % 1800 == 0 ) {  // каждые 30 мин
        fuel_save_data();
    }

    if ( reset_fuel )
    {
        fuel_reset_data();
        reset_fuel = 0;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}




void webfunc(char *pbuf) 
{
    webfunc_print_kotel_data(pbuf);
os_sprintf(HTTPBUFF,"<br>Режим насоса: %d</small>", PUMP_MODE); 
    // SCRIPT
    webfunc_print_script(pbuf);

    webfunc_print_fuel_pump_data(pbuf);

    
    os_sprintf(HTTPBUFF,"<br><small>Version: %s</small>", FW_VER); 
}