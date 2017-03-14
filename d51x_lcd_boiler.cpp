#define LCD_UPDATE_TIME  2 // sec
//#define GPIO_BOILER 16
//#define GPIO_PUMP 15
//#define GPIO_HOTCAB 14

//#define ADC_PAGE 820

#define page1_line1 "Boiler %s %6d.%1d°"
#define page1_line2 "Pump   %s Power %s"
#define page1_line3 "Hotcab %s %6d.%1d°"
#define page1_line4 "Hum %2d.%1d%%  Air %2d.%1d°"

#define str_current_power "%s%2d.%1dA  %4dW" // " 10.6A  1534W"
#define str_max_power_load_date "%s%4d %02d.%02d.%02d" // "1534 22.12.17"
#define str_max_power_load_time "%%s4d %02d:%02d:" // "1534 13:12"
#define str_max_current_load_date "%s%2d.%1d %02d.%02d.%02d" // "15.4 22.12.17"
#define str_max_current_load_time "%s%2d.%1d %02d:%02d" // "15.4 13:12"
#define str_page_header_2 "***  Power Load  ***"
#define str_page_header_3 "***** MAX Power	****"
#define str_page_header_4 "**** MAX Current ***"

#define ADC_divider_A 0,296
#define ADC_divider_mA 296
#define ADC_zero 128

#define MIN_CUR_PUMP 200

#define PAGES_COUNT 4

uint8_t page = 0;
uint16_t adc_page = 0;
uint8_t gpio_boiler = 0;
uint8_t gpio_hotcab = 0;
uint8_t gpio_pump = 0;

int16_t adc0 = 0;
int16_t adc1 = 0;
int16_t adc2 = 0;
int16_t adc3 = 0;

int16_t max_adc0 = 0;
int16_t max_adc1 = 0;
int16_t max_adc2 = 0;
int16_t max_adc3 = 0;

uint8_t max_adc0_hour = 0;
uint8_t max_adc0_min = 0;
uint8_t max_adc0_day = 0;
uint8_t max_adc0_month = 0;

uint8_t max_adc1_hour = 0;
uint8_t max_adc1_min = 0;
uint8_t max_adc1_day = 0;
uint8_t max_adc1_month = 0;

uint8_t max_adc2_hour = 0;
uint8_t max_adc2_min = 0;
uint8_t max_adc2_day = 0;
uint8_t max_adc2_month = 0;

// Имя переменной sensors_param.cfgdes[X] , где X - номер переменной начиная от нуля. Тип переменной int32_t .

void ICACHE_FLASH_ATTR update_LCD(uint32_t  timersrc) {
	
	uint8_t show_min = 0;
	if(timersrc%4==0){
		// выполнение кода каждые 4 секунд
		show_min = 1;
	} else {
		show_min = 0;
	}

	uint8_t   lcd_line = 0;
	char lcd_line_text[40] = "";
	int16_t temp = 0;
	
 	if ( (analogRead() > (adc_page - 20)) && (analogRead() < (adc_page + 20)) ) {
		page++;
		if (page > PAGES_COUNT - 1) page = 0;
	} 
	
	adc0 = abs(ADCdata[1] - 128)*296;
	//if (adc0 < 0 ) adc0 *= -1;
	if (adc0 > max_adc0 ) { 
		max_adc0 = adc0;
		max_adc0_hour = time_loc.hour;
		max_adc0_min = time_loc.min;
		//max_adc0_day = time_loc.day;
		//max_adc0_month = time_loc.month;
	}
	
	adc1 = abs(ADCdata[3] - 128)*296;
	//if (adc1 < 0 ) adc1 *= -1;		
	if (adc1 > max_adc1 ) { 
		max_adc1 = adc1;
		max_adc1_hour = time_loc.hour;
		max_adc1_min = time_loc.min;
		//max_adc1_day = time_loc.day;
		//max_adc1_month = time_loc.month;
		
	}
	
	adc2 = abs(ADCdata[0] - 128)*296;
	//if (adc2 < 0 ) adc2 *= -1;		
	if (adc2 > max_adc2 ) { 
		max_adc2 = adc2;
		max_adc2_hour = time_loc.hour;
		max_adc2_min = time_loc.min;
		//max_adc2_day = time_loc.day;
		//max_adc2_month = time_loc.month;
		
	}
	
	switch (page) {
		case 0: 
			// ***************** page 1, line 1 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 0;
			os_sprintf(lcd_line_text, page1_line2, (abs(adc0-ADC_zero) > MIN_CUR_PUMP) ? "ON " : "OFF", (GPIO_ALL_GET(gpio_pump) == 1) ? "ON " : "OFF");
			LCD_print(lcd_line, lcd_line_text);	
	
			// ***************** page 1, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 1;
			temp = data1wire[0];  // выц1
			os_sprintf(lcd_line_text, page1_line1, (GPIO_ALL_GET(gpio_boiler) == 1) ? "ON " : "OFF", (int)(temp / 10), (int)(temp % 10));
			LCD_print(lcd_line, lcd_line_text);		
	

			// ***************** page 1, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 2;
			temp = data1wire[1];  // 
			temp = analogRead();
			os_sprintf(lcd_line_text, page1_line3, (GPIO_ALL_GET(gpio_hotcab) == 1) ? "ON " : "OFF", (int)(temp / 10), (int)(temp % 10));
			LCD_print(lcd_line, lcd_line_text);		
			
			// ***************** page 1, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);	
			lcd_line = 3;
			temp = dht_t1;
			uint16_t hum = dht_h1;
			os_sprintf(lcd_line_text, page1_line4, (int)(hum / 10), (int)(hum % 10), (int)(temp / 10), (int)(temp % 10));
			LCD_print(lcd_line, lcd_line_text);	
			break;	
		case 1:
			os_memset(lcd_line_text, 0, 40);	
			LCD_print(0, str_page_header_2);
			// ***************** page 2, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);
			os_sprintf(lcd_line_text, str_current_power, "Boiler  ", (int)(adc0/1000), (int)((adc0 % 1000)/100), (int)(adc0*220/1000)); 
			LCD_print(1, lcd_line_text);	
			// ***************** page 2, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);
			os_sprintf(lcd_line_text, str_current_power, "Pump    ", (int)(adc1/1000), (int)((adc1 % 1000)/100), (int)(adc1*220/1000)); 
			LCD_print(2, lcd_line_text);				
			// ***************** page 2, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);
			os_sprintf(lcd_line_text, str_current_power, "Hotcab  ", (int)(adc2/1000), (int)((adc2 % 1000)/100), (int)(adc2*220/1000)); 
			LCD_print(3, lcd_line_text);				
			break;
		case 2:
			os_memset(lcd_line_text, 0, 40);	
			LCD_print(0, str_page_header_3);
			// ***************** page 3, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);
			if (show_min == 0) {
				os_sprintf(lcd_line_text, str_max_power_load_date, "Boiler ", (int)(max_adc0/1000), (int)((max_adc0 % 1000)/100), max_adc0_day, max_adc0_month); 
			} else {
				os_sprintf(lcd_line_text, str_max_power_load_time, "Boiler ", (int)(max_adc0/1000), (int)((max_adc0 % 1000)/100), max_adc0_hour, max_adc0_min); 
			}			
			LCD_print(1, lcd_line_text);	
			// ***************** page 3, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);
			if (show_min == 0) {			
				os_sprintf(lcd_line_text, str_max_power_load_date, "Pump   ", (int)(max_adc1/1000), (int)((max_adc1 % 1000)/100), max_adc1_day, max_adc1_month); 
			} else {
				os_sprintf(lcd_line_text, str_max_power_load_time, "Pump   ", (int)(max_adc1/1000), (int)((max_adc1 % 1000)/100), max_adc1_hour, max_adc1_min); 				
			}
			LCD_print(2, lcd_line_text);				
			// ***************** page 3, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);
			if (show_min == 0) {			
				os_sprintf(lcd_line_text, str_max_power_load_date, "Hotcab ", (int)(max_adc2/1000), (int)((max_adc2 % 1000)/100), max_adc2_day, max_adc2_month); 
			} else {
				os_sprintf(lcd_line_text, str_max_power_load_time, "Hotcab ", (int)(max_adc2/1000), (int)((max_adc2 % 1000)/100), max_adc2_hour, max_adc2_min); 				
			}
			LCD_print(3, lcd_line_text);				

			break;			
		case 3:
			os_memset(lcd_line_text, 0, 40);	
			LCD_print(0, str_page_header_4);
			// ***************** page 4, line 2 ******************************
			os_memset(lcd_line_text, 0, 40);
			if (show_min == 0) {			
				os_sprintf(lcd_line_text, str_max_current_load_date, "Boiler ", (int)(max_adc0/1000), (int)((max_adc0 % 1000)/100), max_adc0_day, max_adc0_month); 
			} else {
				os_sprintf(lcd_line_text, str_max_current_load_time, "Boiler ", (int)(max_adc0/1000), (int)((max_adc0 % 1000)/100), max_adc0_hour, max_adc0_min); 				
			}
			LCD_print(1, lcd_line_text);	
			// ***************** page 4, line 3 ******************************
			os_memset(lcd_line_text, 0, 40);
			if (show_min == 0) {				
				os_sprintf(lcd_line_text, str_max_current_load_date, "Pump   ", (int)(max_adc1/1000), (int)((max_adc1 % 1000)/100), max_adc1_day, max_adc1_month); 
			} else {
				os_sprintf(lcd_line_text, str_max_current_load_time, "Pump   ", (int)(max_adc1/1000), (int)((max_adc1 % 1000)/100), max_adc1_hour, max_adc1_min); 				
			}
			LCD_print(2, lcd_line_text);				
			// ***************** page 4, line 4 ******************************
			os_memset(lcd_line_text, 0, 40);
			if (show_min == 0) {		
				os_sprintf(lcd_line_text, str_max_current_load_date, "Hotcab ", (int)(max_adc2/1000), (int)((max_adc2 % 1000)/100), max_adc2_day, max_adc2_month); 
			} else {
				os_sprintf(lcd_line_text, str_max_current_load_time, "Hotcab ", (int)(max_adc2/1000), (int)((max_adc2 % 1000)/100), max_adc2_hour, max_adc2_min); 				
			}
			LCD_print(3, lcd_line_text);	
			break;				
	}	
			
}


void ICACHE_FLASH_ATTR
startfunc(){
// выполняется один раз при старте модуля.
	adc_page = sensors_param.cfgdes[0];
	gpio_boiler = sensors_param.cfgdes[1];
	gpio_hotcab = sensors_param.cfgdes[2];
	gpio_pump = sensors_param.cfgdes[3];
}

void ICACHE_FLASH_ATTR
 timerfunc(uint32_t  timersrc) {
// выполнение кода каждую 1 секунду
 update_LCD(timersrc);

if(timersrc%30==0){
// выполнение кода каждые 30 секунд
}
}

void webfunc(char *pbuf) {
os_sprintf(HTTPBUFF,"<br>PCF8591 = AIN0: %d\tAIN1: %d\tAIN2: %d\tAIN3: %d", adc0, adc1, adc2, adc3 ); // вывод данных на главной модуля

}