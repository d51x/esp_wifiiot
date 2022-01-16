static const char* UTAG = "MCP23017";

#define MCP23017_INTA_PIN 4     // pin esp


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

#define B(bit_no)         (1 << (bit_no))
#define BIT_CLEAR(reg, bit_no)   (reg) &= ~B(bit_no)
#define BIT_SET(reg, bit_no)   (reg) |= B(bit_no)
#define BIT_CHECK(reg, bit_no)   ( (reg) & B(bit_no) )
#define BIT_TRIGGER(reg, bit_no)   (reg) ^= B(bit_no)

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

TaskHandle_t mcp23017_task;
QueueHandle_t mcp23017_queue;

typedef void (*interrupt_cb)(void *arg, uint8_t *state);

typedef struct mcp23017_pin_isr {
        uint8_t pin;
        interrupt_cb pin_cb;
        gpio_int_type_t intr_type;
        void *args;
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
                                if (( state == 1 && pin_isr[ j ].intr_type == GPIO_INTR_POSEDGE) || 
                                    ( state == 0 && pin_isr[ j ].intr_type == GPIO_INTR_NEGEDGE) || 
                                                    pin_isr[ j ].intr_type == GPIO_INTR_ANYEDGE )
                                {
                                    // execute callback
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

esp_err_t mcp23017_isr_handler_add(uint8_t pin, gpio_int_type_t intr_type, interrupt_cb cb, void *args)
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
    return ESP_OK;     
}

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

void startfunc(){
    // выполняется один раз при старте модуля.

    // установить прерывания пинов
    //MCPwrite_reg16(0, GPINTENA, MCP23017_GPIO0 | MCP23017_GPIO1 | MCP23017_GPIO2 | MCP23017_GPIO3 | MCP23017_GPIO4 | MCP23017_GPIO5); // 0b0000111000000000
    MCPwrite_reg16(0, GPINTENA, 0b0000000000111111); // 0b0000111000000000

    // условия сработки прерывания на ногах
    MCPwrite_reg16(0, INTCONA, 0);  // при нулях

    // дефолтные значения ног, прерывание сработает, если на ноге сигнал отличается от дефолтного, если на пинах значение отличается от  заданного ниже (DEFVAL  = 1 )
    MCPwrite_reg16(0, DEFVALA, 0b1111111111111111);    

    // инвертируем 13 пин
    //MCPwrite_reg16(0, IPOLA, 0b0001000100000000); // 0b0000111000000000

    // установить прерывания на GPIO
        // configure interrupts 
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
    mcp23017_isr_handler_add( 0, GPIO_INTR_NEGEDGE, mcp23017_button_isr_cb, 208);
    mcp23017_isr_handler_add( 1, GPIO_INTR_NEGEDGE, mcp23017_button_isr_cb, 209);
    mcp23017_isr_handler_add( 2, GPIO_INTR_NEGEDGE, mcp23017_button_isr_cb, 210);
    mcp23017_isr_handler_add( 3, GPIO_INTR_NEGEDGE, mcp23017_button_isr_cb, 211);

    // датчик движения
    //mcp23017_isr_handler_add( 4, GPIO_INTR_ANYEDGE, mcp23017_pir_sensor_cb, 212);
    
    // датчик геркон
    //mcp23017_isr_handler_add( 5, GPIO_INTR_ANYEDGE, mcp23017_pir_sensor_cb, 213);

    //mcp23017_isr_handler_add( 5, GPIO_INTR_ANYEDGE, mcp23017_pir_sensor_cb, 199); // display backlight

    mcp23017_queue = xQueueCreate(5, sizeof(uint16_t) * 2);
    xTaskCreate( mcp23017_isr_cb, "mcp23017_isr", 1024, NULL, 10, &mcp23017_task);

}

void timerfunc(uint32_t  timersrc) {
    // выполнение кода каждую 1 секунду
    if ( timersrc % 30 == 0 ) {
        // выполнение кода каждые 30 секунд
    }

    char str[20];
    sprintf(str, "%02d:%02d:%02d %02d.%02d.%02d, %d"
                , time_loc.hour
                , time_loc.min
                , time_loc.sec

                , time_loc.day
                , time_loc.month
                , time_loc.year
                , time_loc.dow + 1 );

        //time_loc.dow - день недели, 0 - понедельник.

    LCD_print(0, str);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void webfunc(char *pbuf) {
    os_sprintf(HTTPBUFF,"<br>test"); // вывод данных на главной модуля
}