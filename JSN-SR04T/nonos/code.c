#include "../moduls/uart_register.h"
#include "../moduls/uart.h"
#include "../moduls/uart.c" // ??????

#define DEBUG

#define FW_VER "1.1"

#define DELAYED_START					60   //sec
#define UART_READ_TIMEOUT					1000  // влияет на результаты чтения из юсарт
#define SONAR_READ_DELAY 	1000

#define millis() (uint32_t) (micros() / 1000ULL)

#define COMMAND				0x55
#define RESPONSE_SIZE 		4

#ifdef DEBUG
	static char logstr[100];
#endif

uint8_t delayed_counter = DELAYED_START;
static volatile os_timer_t read_sonar_timer;
static volatile os_timer_t system_start_timer;
	
volatile uint8_t uart_ready = 1;
uint16_t distance = 0;

void system_start_cb( );
void read_sonar_cb();	

void send_buffer(uint8_t *buffer, uint8_t len);
void read_buffer();


// UART1 TX GPIO2 Enable output debug
#ifdef DEBUG
void ICACHE_FLASH_ATTR uart1_tx_buffer(uint8_t *buffer, uint8_t sz) {
	uint8_t i;
	for (i = 0; i < sz; i++) {
		while (true)
		{
			uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(UART1)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S);
			if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
				break;
			}
		}
		WRITE_PERI_REG(UART_FIFO(UART1) , buffer[i]);
	}
}
#endif


void send_buffer(uint8_t *buffer, uint8_t len){
	uart0_tx_buffer(buffer, len);
}

void read_buffer(){	
	static char rx_buf[125];
	static uint8_t len = 0;

	uint32_t ts = micros();
	
	WRITE_PERI_REG(UART_INT_CLR(UART0),UART_RXFIFO_FULL_INT_CLR);
	while ( READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S) 
			&& ( micros() - ts < UART_READ_TIMEOUT*1000)) 
	{
		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		uint8_t read = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		rx_buf[len] = read; // buffer[i++] = read;
		len++;
		ts = micros();
		
		
		#ifdef DEBUG
			os_bzero(logstr, 100);
			os_sprintf(logstr, "%02X ", read);
			uart1_tx_buffer(logstr, os_strlen(logstr));
		#endif
	
		if ( RESPONSE_SIZE == len) {
			// что то прочитали
			// validate
			#ifdef DEBUG
				os_bzero(logstr, 100);
				os_sprintf(logstr, "\n response OK \n ");
				uart1_tx_buffer(logstr, os_strlen(logstr));
			#endif
		
			if ( rx_buf[0] == 0xFF) 
			{
				// check crc
				uint16_t crc = (rx_buf[0] + rx_buf[1] + rx_buf[2]) & 0xFF;
				if ( crc == rx_buf[3] ) {
					distance = ( (rx_buf[1] << 8 ) + rx_buf[2]);
					
					#ifdef DEBUG
					os_bzero(logstr, 100);
					os_sprintf(logstr, "\n distance: %d \n ", distance);
					uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif				
				} else {
					#ifdef DEBUG
						os_bzero(logstr, 100);
						os_sprintf(logstr, "\n CRC FAIL \n ");
						uart1_tx_buffer(logstr, os_strlen(logstr));
					#endif				
				}
			} else {
				#ifdef DEBUG
					os_bzero(logstr, 100);
					os_sprintf(logstr, "\n FAILE: incorrect responce \n ");
					uart1_tx_buffer(logstr, os_strlen(logstr));
				#endif
			}				
			len = 0;
			uart_ready = 1;			
			break;			
		}
	}				
}

void ICACHE_FLASH_ATTR startfunc(){
	// выполняется один раз при старте модуля.
	uart_init(BIT_RATE_9600);	  
	ETS_UART_INTR_ATTACH(read_buffer, NULL);

	#ifdef DEBUG
		uart_config(UART1);
		uart_div_modify(UART1,	UART_CLK_FREQ	/BIT_RATE_9600);

		os_install_putc1((void *)uart1_tx_buffer);
	#endif

	// запуск таймера, чтобы мой основной код начал работать через Х секунд после старта, чтобы успеть запустить прошивку
	os_timer_disarm(&system_start_timer);
	os_timer_setfn(&system_start_timer, (os_timer_func_t *)system_start_cb, NULL);
	os_timer_arm(&system_start_timer, DELAYED_START * 1000, 0);
}

void ICACHE_FLASH_ATTR timerfunc(uint32_t  timersrc) {
	if(timersrc%30==0){
		// выполнение кода каждые 30 секунд
	}
		
	if ( delayed_counter > 0 ) { 
		delayed_counter--;	
	}	
}

void system_start_cb( ){
	os_timer_disarm(&read_sonar_timer);
	os_timer_setfn(&read_sonar_timer, (os_timer_func_t *)read_sonar_cb, NULL);
	os_timer_arm(&read_sonar_timer, 20, 0); // будет рестартовать сам себя
}

void read_sonar_cb(){
	if ( uart_ready ) {
		// send command into uart
		uart_ready = 0;
		distance = 0; // ???? обнулять?
		uint8_t cmd = COMMAND;
		send_buffer(&cmd, 1);
	}
	
	os_timer_disarm(&read_sonar_timer);
	os_timer_setfn(&read_sonar_timer, (os_timer_func_t *)read_sonar_cb, NULL);
	os_timer_arm(&read_sonar_timer, SONAR_READ_DELAY, 0);	
	
}


void webfunc(char *pbuf) {

	if ( delayed_counter > 0 ) {
		os_sprintf(HTTPBUFF,"<br>До начала чтения данных счетчика осталось %d секунд", delayed_counter);
	}

	os_sprintf(HTTPBUFF,"<br><b>Расстояние:</b> %d мм", 	distance);
	os_sprintf(HTTPBUFF,"<br><b>Версия:</b> %s", FW_VER);
}