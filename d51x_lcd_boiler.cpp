static os_timer_t esp_timer; // глобально объ€вим таймер esp_timer

const char str_boiler = "Boiler ";
const char str_pump = "Pump   ";
const char str_hotcab = "Hotcab ";
const char str_power = " Power ";
const char str_airt = "AirT:%s%02d.%01d*C H:%02d.%01d\%";
const char str_current_power = " %02d.%01dA  %04dW"; // " 10.6A  1534W"
const char str_max_power_load_date = "%04d %02d.%02d.%02d"; // "1534 22.12.17"
const char str_max_power_load_time = "%04d %02d:%02d:"; // "1534 13:12"
const char str_max_current_load_date = "%02d.%01d %02d.%02d.%02d"; // "15.4 22.12.17"
const char str_max_current_load_time = "%02d.%01d %02d:%02d:"; // "15.4 13:12"
const char str_page_header_2 = "***  Power Load  ***";
const char str_page_header_3 = "***** MAX Power	****";
const char str_page_header_4 = "**** MAX Current ***";

#define LCD_UPDATE_TIME = 2 // sec
#define GPIO_BOILER 16
#define GPIO_PUMP 15
#define GPIO_HOTCAB 14

// запуск таймера:
os_timer_disarm(&esp_timer);
os_timer_setfn(&esp_timer, (os_timer_func_t *)read_esp, NULL); // read_esp -функцию, которую нужно вызвать по таймеру.
os_timer_arm(&esp_timer, 1000, 1); // 1000 миллисекунд. 1 - многократно. 0 -однократно.

void ICACHE_FLASH_ATTR read_esp(){
	// тут пишем код, который будет вызыватьс€ по таймеру
	uint8_t   lcd_line = 0;
	char lcd_line_text[40] = "";
	
	memset(lcd_line_text, 0, 40);
	// ***************** page 1, line 1 ******************************
	lcd_line = 0;
	//boiler state
	strcpy(lcd_line_text, str_boiler);
	strcat(lcd_line_text, (GPIO_ALL_GET(GPIO_BOILER) == 1) ? "ON " : "OFF");
	strcat(lcd_line_text, " T:");
	int32_t t = data1wire[0];  // выц1
	if ( t < 0 ) {
		strcat(lcd_line_text, "+%02d.%01d*C");
	} else if ( t > 0 ) {
		strcat(lcd_line_text, "+%02d.%01d*C");
	} else {
		strcat(lcd_line_text, " %02d.%01d*C");
	}
	os_sprintf(lcd_line_text, (int)(t / 10), (int)(t % 10));
	LCD_print(lcd_line, lcd_line_text);
	
	
	// ***************** page 1, line 2 ******************************
	memset(lcd_line_text, 0, 40);
	lcd_line = 1;
	//pump state + power state
	strcpy(lcd_line_text, str_pump);
	strcat(lcd_line_text, (GPIO_ALL_GET(GPIO_PUMP) == 1) ? "ON " : "OFF");
	strcat(lcd_line_text, str_power);
	// get data from ADC 1
	
	LCD_print(lcd_line, lcd_line_text);	
}


void ICACHE_FLASH_ATTR startfunc(){
	// выполн€етс€ один раз при старте модул€.
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	// место дл€ вставки кода, который будет выполн€тс€ каждую 1 секунду.

	if(timersrc%30==0){
		// место дл€ вставки кода, который будет выполн€тс€ каждые 30 секунд.
		
	}
}

void webfunc(char *pbuf) {
	os_sprintf(HTTPBUFF,"Hello world"); // вывод данных на главной модул€
}