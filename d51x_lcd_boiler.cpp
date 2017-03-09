#define LCD_UPDATE_TIME  2 // sec
#define GPIO_BOILER 16
#define GPIO_PUMP 15
#define GPIO_HOTCAB 14

#define ADC_PAGE 820

#define page1_line1 "Boiler %s %6d.%1d°"
#define page1_line2 "Pump   %s Power %s"
#define page1_line3 "Hotcab %s %6d.%1d°"
#define page1_line4 "Hum %2d.%1d%%  Air %2d.%1d°"

#define str_current_power " %02d.%01dA  %04dW" // " 10.6A  1534W"
#define str_max_power_load_date "%04d %02d.%02d.%02d" // "1534 22.12.17"
#define str_max_power_load_time "%04d %02d:%02d:" // "1534 13:12"
#define str_max_current_load_date "%02d.%01d %02d.%02d.%02d" // "15.4 22.12.17"
#define str_max_current_load_time "%02d.%01d %02d:%02d:" // "15.4 13:12"
#define str_page_header_2 "***  Power Load  ***"
#define str_page_header_3 "***** MAX Power	****"
#define str_page_header_4 "**** MAX Current ***"

#define PAGES_COUNT 4
uint8_t page = 0;

void update_LCD() {
	uint8_t   lcd_line = 0;
	char lcd_line_text[40] = "";
	int16_t temp = 0;
	
/* 	if ( (analogRead() > (ADC_PAGE - 10)) && (analogRead() < (ADC_PAGE + 10)) ) {
		page++;
		if (page > PAGES_COUNT - 1) page = 0;
	} */
	
	switch (page) {
		case 0: 
			// ***************** page 1, line 1 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 0;
			os_sprintf(lcd_line_text, page1_line2, (GPIO_ALL_GET(GPIO_PUMP) == 1) ? "ON " : "OFF", (GPIO_ALL_GET(GPIO_PUMP) == 1) ? "ON " : "OFF");
			LCD_print(lcd_line, lcd_line_text);	
	
			// ***************** page 1, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 1;
			temp = data1wire[0];  // выц1
			os_sprintf(lcd_line_text, page1_line1, (GPIO_ALL_GET(GPIO_BOILER) == 1) ? "ON " : "OFF", (int)(temp / 10), (int)(temp % 10));
			LCD_print(lcd_line, lcd_line_text);		
	

			// ***************** page 1, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 2;
			temp = data1wire[1];  // 
			os_sprintf(lcd_line_text, page1_line3, (GPIO_ALL_GET(GPIO_HOTCAB) == 1) ? "ON " : "OFF", (int)(temp / 10), (int)(temp % 10));
			LCD_print(lcd_line, lcd_line_text);		
			
			// ***************** page 1, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 3;
			temp = dht_t1;
			uint16_t hum = dht_h1;
			os_sprintf(lcd_line_text, page1_line4, (int)(temp / 10), (int)(temp % 10), (int)(hum / 10), (int)(hum % 10));
			LCD_print(lcd_line, lcd_line_text);	
			break;	
		case 1:
			os_memset(lcd_line_text, 0, 40);	
			LCD_print(2, lcd_line_text);	
			LCD_print(3, lcd_line_text);	
			break;
		case 2:
			os_memset(lcd_line_text, 0, 40);	
			LCD_print(0, lcd_line_text);	
			LCD_print(1, lcd_line_text);	
			break;			
	}	
			
}


void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду

 update_LCD();

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>analogRead() = %d", analogRead()); // вывод данных на главной модуля
}