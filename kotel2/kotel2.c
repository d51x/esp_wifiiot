#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

#define FW_VER "1.18"
/*
0    1        2          3              4           5                   6          7      8        9      10       11     12       13     14       15     16
Авто,Разрешен,Период/сек,Гистерезис/x10,Уставка/x10,Задержка насоса/сек,Расписание,ЧЧММ-1,Уст1/x10,ЧЧММ-2,Уст2/x10,ЧЧММ-3,Уст3/x10,ЧЧММ-4,Уст4/x10,ЧЧММ-5,Уст5/x10
используется 2 термостата
cfg0 (valdes0)   режим работы управления котлами:
    0 - ручной, термостаты не включены, можно кнопками вкл/выкл gpio котлов
    1 - авто, используются термостаты в автоматическом режиме
    2 - активный котел 1, используется термостат
    3 - активный котел 2, используется термостат

cfg1 (valdes1)   состояние термостата
cfg2    период срабатывания термостата в секундах
cfg3    гистерезис х10
cfg4 (valdes2)   глобальная устав, принимаем из внешней системы

cfg5    задержка выключения насоса электрокотла, сек
cfg6    расписание (0 - используется, 1 - не используется)

cfg7	часы-минуты Т1
cfg8	уставка для Т1
	
cfg9	часы минуты Т2
cfg10	уставка для Т2
	
cfg11	часы минуты Т3
cfg12	уставка для Т3
	
cfg13	часы минуты Т4
cfg14	уставка для Т4
	
cfg15	часы минуты Т5
cfg16	уставка для Т5

*/


#define GPIO_ON 1
#define GPIO_OFF 0

#define KOTEL1_GPIO 12 // kiturami
//#define KOTEL2_GPIO 14 // protherm
#define KOTEL2_GPIO 13 // protherm
//#define KOTEL_NIGHT_MODE_GPIO 16  // ESC режим для Протерма
#define KOTEL_NIGHT_MODE_GPIO 2  // ESC режим для Протерма

#define NIGHT_MODE_START_TIME 23
#define NIGHT_MODE_END_TIME 7

#define THERMOSTAT_PERIOD 40
#define THERMOSTAT_TEMPSET 250
#define THERMOSTAT_HYSTERESIS 5

#define PUMP_DELAY 300
#define KOTEL_ELECTRO_MIN_TEMP -100 // -10  x10
#define KOTEL_DIESEL_MAX_TEMP  50 // 5  x10

typedef enum {
    KOTEL_NONE,
    KOTEL_DIESEL,
    KOTEL_ELECTRO
} kotel_type_t;

kotel_type_t active_kotel = 0; // 0 - нет активного котла, 1 - дизельный, 2 - электрический

#define CFG_INDEX_WORK_MODE 0
#define VALDES_INDEX_WORK_MODE 0 

#define CFG_INDEX_THERMO_ENABLED 1
#define VALDES_INDEX_THERMO_ENABLED 1 

#define CFG_INDEX_THERMO_PERIOD 2
#define VALDES_INDEX_THERMO_PERIOD -1 

#define CFG_INDEX_THERMO_HYSTERESIS 3
#define VALDES_INDEX_THERMO_HYSTERESIS -1 

#define CFG_INDEX_THERMO_TEMPSET 4
#define VALDES_INDEX_THERMO_TEMPSET 2

#define CFG_INDEX_PUMP_DELAY 5
#define VALDES_INDEX_PUMP_DELAY -1

#define CFG_INDEX_SCHEDULE_ENABLED 6
#define VALDES_INDEX_SCHEDULE_ENABLED -1

#define CFG_TEMPSET_SCHEDULE_START_IDX 7
#define VALDES_TEMPSET_SCHEDULE_START_IDX -1
#define CFG_TEMPSET_SCHEDULE_COUNT 5

uint8_t work_mode = 0; // режим работы (cfg0)
uint16_t pump_delay = 300;
uint8_t kotel_gpio = 255;

int16_t kotel_electro_min_temp;
int16_t kotel_diesel_max_temp;

typedef struct schedules_tempset {
    int32_t hhmm;
    uint8_t hour;
    uint8_t min;  
    uint16_t tempset;   // x10
} schedules_tempset_t;

schedules_tempset_t schedules_tempset[CFG_TEMPSET_SCHEDULE_COUNT];
uint8_t schedule_enabled = 0;

//************************ объект Термостат и функции ***********************************************************
typedef void (* func_therm)(void *args);  
typedef void *thermostat_handle_t;  

typedef struct thermostat {
    uint8_t enabled;
    uint8_t state;
    uint8_t period;
    int16_t tempset;// уставка х10
    uint8_t hysteresis;// гистерезис х10
    int16_t value; // текущая температура х10
    func_therm process;       // сама функция термостата, вызывает коллбеки load_off и load_on
    void *args_process;
    func_therm load_off;
    void *args_on;
    func_therm load_on;
    void *args_off;
    os_timer_t tmr;
} thermostat_t;


void ICACHE_FLASH_ATTR thermostat_enable(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    //thermo->state = 1;
    thermo->enabled = 1;
}

void ICACHE_FLASH_ATTR thermostat_disable(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    //thermo->state = 0;
    thermo->enabled = 0;
}

void ICACHE_FLASH_ATTR thermostat_start(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    os_timer_disarm( &thermo->tmr );
	os_timer_setfn( &thermo->tmr, (os_timer_func_t *)thermo->process, NULL);
	os_timer_arm( &thermo->tmr, thermo->period * 1000, 1);
}

void ICACHE_FLASH_ATTR thermostat_stop(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    os_timer_disarm( &thermo->tmr );
}

void ICACHE_FLASH_ATTR thermostat_set_load_on_cb(thermostat_handle_t thermo_h, func_therm _cb, void *_args)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;    
    if ( _cb == NULL ) return;
    thermo->load_on = _cb;
    thermo->args_on = _args;
}

void ICACHE_FLASH_ATTR thermostat_set_load_off_cb(thermostat_handle_t thermo_h, func_therm _cb, void *_args)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;    
    if ( _cb == NULL ) return;
    thermo->load_off = _cb;
    thermo->args_off = _args;
}

void ICACHE_FLASH_ATTR thermostat_process(thermostat_handle_t thermo_h)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    if ( thermo->enabled == 0 ) return; // термостат выключен

    if ( thermo->value >= thermo->tempset + thermo->hysteresis )
    {
        if ( thermo->load_off ) thermo->load_off( thermo->args_off );
        thermo->state = 0;
    } 
    else if ( thermo->value <= thermo->tempset - thermo->hysteresis )
    {
        if ( thermo->load_on ) thermo->load_on( thermo->args_on );
        thermo->state = 1;
    }
}

thermostat_handle_t ICACHE_FLASH_ATTR thermostat_create(uint8_t _period, int16_t _tempset, uint8_t _hysteresis)
{
    //thermostat_t *thermo;
    thermostat_t *_thermo = (thermostat_t *) os_malloc(sizeof(thermostat_t));
    _thermo->state = 0;
    _thermo->enabled = 0;
    _thermo->period = _period;
    _thermo->tempset = _tempset;
    _thermo->hysteresis = _hysteresis;
    //thermo->process = thermostat_process;
    //thermo->arg_process = thermo;
    _thermo->load_off = NULL;
    _thermo->args_off = NULL;
    _thermo->load_on = NULL;
    _thermo->args_on = NULL;
    _thermo->value = 0;
    return (thermostat_handle_t) _thermo;
}

void ICACHE_FLASH_ATTR thermostat_set_value(thermostat_handle_t thermo_h, int16_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->value = _value;
}

void ICACHE_FLASH_ATTR thermostat_set_tempset(thermostat_handle_t thermo_h, int16_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->tempset = _value;
}

void ICACHE_FLASH_ATTR thermostat_set_hysteresis(thermostat_handle_t thermo_h, uint8_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->hysteresis = _value;
}

void ICACHE_FLASH_ATTR thermostat_set_period(thermostat_handle_t thermo_h, uint8_t _value)
{
    thermostat_t *thermo = (thermostat_t *) thermo_h;
    thermo->period = _value;
}

//**************************************************************************************************************

thermostat_handle_t thermo_h;
thermostat_t *thermo;

int16_t ICACHE_FLASH_ATTR get_temp() 
{
    int16_t _temp = 240; // взять с датчика, или из valdes, или расчитать среднее или минимальное по датчикам
    _temp = vsens[1][0];
    //vsens[1][1]
    //vsens[2][1]
    //vsens[3][1]
    return _temp;
}

// функция изменения значения уставки по времени
// функция должна отрабатывать каждую минуту для корректировки уставки по расписанию
// TODO: rename to get_schedule_index, потом температуру брать по индексу, или глобальную, если -1 индекс.
uint16_t ICACHE_FLASH_ATTR calc_tempset(uint16_t g_tempset) 
{
    uint16_t _tempset = g_tempset; // выставить глобальную уставку
    if ( !schedule_enabled ) return _tempset;

    uint8_t i;

    uint16_t local_minutes = time_loc.hour*60 + time_loc.min;

    for ( i = 0; i < CFG_TEMPSET_SCHEDULE_COUNT; i++)
    {
        //if ( schedules_tempset[i].hour == 0 && schedules_tempset[i].min == 0  )
        //    continue;

        uint16_t schedule_minutes = schedules_tempset[i].hour*60 + schedules_tempset[i].min;
        uint16_t schedule_minutes_next = 23*60+59;
        uint16_t schedule_minutes_prev = 0;
        if ( i > 0 ) schedule_minutes_prev = schedules_tempset[i-1].hour*60 + schedules_tempset[i-1].min;
        if ( i < CFG_TEMPSET_SCHEDULE_COUNT-1 ) schedule_minutes_next = schedules_tempset[i+1].hour*60 + schedules_tempset[i+1].min;

        bool is_active = local_minutes >= schedule_minutes
                        && local_minutes > schedule_minutes_prev
                        && local_minutes < schedule_minutes_next;

        if ( is_active )
        {
            _tempset = schedules_tempset[i].tempset;
            break;
        }
        
    }

    return _tempset;
}

kotel_type_t ICACHE_FLASH_ATTR select_kotel()
{
    kotel_type_t _kotel = KOTEL_NONE;

    if ( work_mode == 0)  {
        return _kotel;
    }

    if ( work_mode == 1) {
        // auto режим
        if ( time_loc.hour >= NIGHT_MODE_END_TIME && time_loc.hour < NIGHT_MODE_START_TIME ) {
            // день, Т1
            _kotel = KOTEL_DIESEL; 
            kotel_gpio = KOTEL1_GPIO;
            GPIO_ALL(KOTEL_NIGHT_MODE_GPIO, GPIO_OFF);
            GPIO_ALL(KOTEL2_GPIO, GPIO_OFF);
        } else {
            // ночь, Т2
            _kotel = KOTEL_ELECTRO; 
            kotel_gpio = KOTEL2_GPIO;
            GPIO_ALL(KOTEL_NIGHT_MODE_GPIO, GPIO_ON);
            GPIO_ALL(KOTEL1_GPIO, GPIO_OFF);
        }   
    } else if ( work_mode == 2 ) {
        _kotel = KOTEL_DIESEL;
        kotel_gpio = KOTEL1_GPIO;
    } else if ( work_mode == 3 ) {
        _kotel = KOTEL_ELECTRO;
        kotel_gpio = KOTEL2_GPIO;
    }

    return _kotel;
}

void ICACHE_FLASH_ATTR load_off(void *args)
{
    // функция отключения нагрузки
    //GPIO_ALL_M(kotel_gpio, GPIO_OFF);
    GPIO_ALL(kotel_gpio, GPIO_OFF);
}

void ICACHE_FLASH_ATTR load_on(void *args)
{
    // функция включения нагрузки
    //GPIO_ALL_M(kotel_gpio, GPIO_ON);   
    GPIO_ALL(kotel_gpio, GPIO_ON);   
}



void ICACHE_FLASH_ATTR startfunc()
{
    // выполняется один раз при старте модуля.
    // прочитать настройки из конфига
    thermo_h = thermostat_create(THERMOSTAT_PERIOD, THERMOSTAT_TEMPSET, THERMOSTAT_HYSTERESIS);
    thermo = (thermostat_t *) thermo_h;
    thermostat_set_load_off_cb(thermo_h, load_off, NULL);
    thermostat_set_load_on_cb(thermo_h, load_on, NULL);
    //thermostat_start(thermo_h);
}


void ICACHE_FLASH_ATTR mqtt_send_valuedes(int8_t idx, int32_t value)
{
    if ( idx < 0 ) return;
    valdes[idx] = value;
    // чтобы все корректно работало и значене не поменялось обратно через mqtt, нужно отправить новое значение
    #ifdef MQTTD
    if ( sensors_param.mqtten && mqtt_client != NULL) {
        system_soft_wdt_feed();
        os_memset(payload, 0, 20);
        os_sprintf(payload,"%d", value);
        char topic[12];
        sprintf(topic, "valuesdes%d", idx)
        MQTT_Publish(mqtt_client, topic, payload, os_strlen(payload), 2, 0, 1);
        os_delay_us(20);
    }
    #endif  
}

bool ICACHE_FLASH_ATTR handle_config_param(uint8_t cfg_idx, int8_t valdes_idx, int32_t *param, int32_t def_val, int32_t min_val, int32_t max_val)
{
    uint32_t val = 0; 

    // сначала смотрим опцию, поменялось ли там значение
    val =  (sensors_param.cfgdes[cfg_idx] < min_val || sensors_param.cfgdes[cfg_idx] > max_val) ? def_val : sensors_param.cfgdes[cfg_idx];  
    if ( val != *param ) {  // значение изменилось, в опции новое значение
        *param = val;
        mqtt_send_valuedes(valdes_idx, val);
    }

    // теперь смотрим valdes, поменялось ли там значение
    if ( valdes_idx > 0 )
    {    
        val = ( valdes[valdes_idx] < min_val || valdes[valdes_idx] > max_val ) ? def_val : valdes[valdes_idx]; // различные проверки, можно убрать
        if ( val != *param ) {
            // прилетело новое значение, сохранить
            *param = val;
            sensors_param.cfgdes[cfg_idx] = val;    // в опцию пропишем новое значени
            return true;                          // флаг, что надо сохранить изменени опции во флеш
        }  
    }
    return false;  
}

void ICACHE_FLASH_ATTR handle_params() {
    // читаем параметры из настроек (cfgdes) или получаем их через valuedes
    bool need_save = false;
    int32_t val;
    val = work_mode;
    need_save |= handle_config_param(CFG_INDEX_WORK_MODE,               VALDES_INDEX_WORK_MODE,                 &val,                 0, 0, 3);
    work_mode = val;

    val = thermo->enabled;
    need_save |= handle_config_param(CFG_INDEX_THERMO_ENABLED,            VALDES_INDEX_THERMO_ENABLED,              &val,           0, 0, 1);
    thermo->enabled = val;

    val = thermo->period;
    need_save |= handle_config_param(CFG_INDEX_THERMO_PERIOD,           VALDES_INDEX_THERMO_PERIOD,             &val,          THERMOSTAT_PERIOD, 5, 600);
    thermo->period = val;

    val = thermo->hysteresis;
    need_save |= handle_config_param(CFG_INDEX_THERMO_HYSTERESIS,       VALDES_INDEX_THERMO_HYSTERESIS,         &val,      THERMOSTAT_HYSTERESIS, 1, 100);
    thermo->hysteresis = val;

    val = thermo->tempset;
    need_save |= handle_config_param(CFG_INDEX_THERMO_TEMPSET,          VALDES_INDEX_THERMO_TEMPSET,            &val,         THERMOSTAT_TEMPSET, 50, 300);
    thermo->tempset = val;

    val = pump_delay;
    need_save |= handle_config_param(CFG_INDEX_PUMP_DELAY,  VALDES_INDEX_PUMP_DELAY,    &val,    PUMP_DELAY, 0, 1200);
    pump_delay = val;

    val = schedule_enabled;
    need_save |= handle_config_param(CFG_INDEX_SCHEDULE_ENABLED,  VALDES_INDEX_SCHEDULE_ENABLED,    &val,    0, 0, 2);
    schedule_enabled = val;
    
    uint8_t i;
    for ( i = 0; i < CFG_TEMPSET_SCHEDULE_COUNT; i++) {
        val = schedules_tempset[i].hhmm;
        need_save |= handle_config_param(CFG_TEMPSET_SCHEDULE_START_IDX + i*2,     VALDES_TEMPSET_SCHEDULE_START_IDX, &val, 0, 0, 2400);
        schedules_tempset[i].hhmm = val;

        val = schedules_tempset[i].tempset;
        need_save |= handle_config_param(CFG_TEMPSET_SCHEDULE_START_IDX + 1 + i*2, VALDES_TEMPSET_SCHEDULE_START_IDX, &val, 0, 50, 300);
        schedules_tempset[i].tempset = val;

        schedules_tempset[i].hour = (uint8_t)( schedules_tempset[i].hhmm / 100);
        schedules_tempset[i].min =  (uint8_t)( schedules_tempset[i].hhmm % 100);
    }

    if ( need_save ) {
        system_soft_wdt_feed();
        SAVEOPT;
    }
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) 
{
    handle_params();

    // управление состоянием термостата
    if ( work_mode == 0)  {
        thermostat_disable(thermo_h);
    } else {
        thermostat_enable(thermo_h);
    }    

    // вычисление уставки по таблице
    //uint16_t tempset = calc_tempset(valdes[2]);
    uint16_t tempset = calc_tempset( thermo->tempset );
    thermostat_set_tempset(thermo_h, tempset);

    if ( timersrc > 60 ) {
        // определение активного котла
        active_kotel = select_kotel();

        if ( timersrc % thermo->period == 0 )
        {
            // сработка термостата с заданным интервалом
            int16_t temp = get_temp();
            thermostat_set_value(thermo_h, temp);
            thermostat_process(thermo_h);
        }

    }


    if ( timersrc % 60 == 0 )
    {
        // выполнение кода каждую минуту



        // отправить новую уставку по mqtt

    }
}

void webfunc(char *pbuf) 
{
    os_sprintf(HTTPBUFF,"<br>Активный котел: ");
    if ( active_kotel == KOTEL_DIESEL )
        os_sprintf(HTTPBUFF,"дизельный");
    else if ( active_kotel == KOTEL_ELECTRO )
        os_sprintf(HTTPBUFF,"электрический");
    else 
        os_sprintf(HTTPBUFF,"-------------");


    os_sprintf(HTTPBUFF,"<br>Режим: "); 
    if (work_mode == 0)
        os_sprintf(HTTPBUFF,"Ручной"); 
    else if ( work_mode == 1)
        os_sprintf(HTTPBUFF,"Авто"); 
    else if ( work_mode == 2)
        os_sprintf(HTTPBUFF,"всегда дизель"); 
    else if ( work_mode == 3)
        os_sprintf(HTTPBUFF,"всегда электрический"); 
    else
        os_sprintf(HTTPBUFF,"<br>Режим: -------"); 
    
    os_sprintf(HTTPBUFF,"<br>Термостат: %s", thermo->enabled ? "Вкл" : "Выкл"); 
    os_sprintf(HTTPBUFF,"<br>Состояние: %s", thermo->state ? "Нагрев" : "Ожидание"); 
    os_sprintf(HTTPBUFF,"<br>Режим ESC: %s", GPIO_ALL_GET(KOTEL_NIGHT_MODE_GPIO) ? "Да" : "Нет"); 

    os_sprintf(HTTPBUFF,"<br>Период: %d сек", thermo->period); 
    os_sprintf(HTTPBUFF,"<br>Уставка: %d.%d °C", (uint16_t)(thermo->tempset / 10), thermo->tempset % 10); 
    os_sprintf(HTTPBUFF,"<br>Гистерезис: %d.%d °C", (uint16_t)(thermo->hysteresis / 10), thermo->hysteresis % 10); 
    os_sprintf(HTTPBUFF,"<br>Комната: %d.%d °C", (int16_t)(thermo->value / 10), thermo->value % 10); 
    
    os_sprintf(HTTPBUFF,"<br><br>Улица: %d.%d °C", (int16_t)(vsens[3][0] / 10), vsens[3][0] % 10); 

    uint16_t local_minutes = time_loc.hour*60 + time_loc.min;
    os_sprintf(HTTPBUFF,"<br><br>Текущие минуты: %d", local_minutes); 
    os_sprintf(HTTPBUFF,"<br><br>Расписание: "); 
    os_sprintf(HTTPBUFF,"%s", schedule_enabled ? "Вкл" : "Выкл"); 

    //if ( schedule_enabled ) {
        os_sprintf(HTTPBUFF,"<table>"); 
        os_sprintf(HTTPBUFF,"<tr><th>#</th><th>Время</th><th>Уставка</th></tr>"); 
        uint8_t i;
        for (i=0; i < CFG_TEMPSET_SCHEDULE_COUNT; i++)
        {
           
            uint16_t schedule_minutes = schedules_tempset[i].hour*60 + schedules_tempset[i].min;
            uint16_t schedule_minutes_next = 23*60+59;
            uint16_t schedule_minutes_prev = 0;
            if ( i > 0 ) schedule_minutes_prev = schedules_tempset[i-1].hour*60 + schedules_tempset[i-1].min;
            if ( i <= CFG_TEMPSET_SCHEDULE_COUNT-2 ) schedule_minutes_next = schedules_tempset[i+1].hour*60 + schedules_tempset[i+1].min;

            bool is_active = local_minutes >= schedule_minutes
                          && local_minutes > schedule_minutes_prev
                          && local_minutes < schedule_minutes_next;

            os_sprintf(HTTPBUFF,"<tr %s><td>%d.</td><td>&nbsp;с %02d:%02d</td><td>%d.%d °C</td><td>%s</td><td>%d -> %d <- %d</td></tr>"
                    , is_active  ? "style='color: red'" : ""
                    , i+1
                    , schedules_tempset[i].hour
                    , schedules_tempset[i].min
                    , (uint16_t)(schedules_tempset[i].tempset / 10)
                    , schedules_tempset[i].tempset % 10 
                    , is_active ? "активно" : ""
                    , schedule_minutes
                    , local_minutes
                    , schedule_minutes_next); 
        }

        os_sprintf(HTTPBUFF,"</table>"); 
    //}
    os_sprintf(HTTPBUFF,"<br><small>Прошивка: %s</small>", FW_VER); 

}