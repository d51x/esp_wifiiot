//https://ipc2u.ru/articles/prostye-resheniya/modbus-rtu/
// http://geekmatic.in.ua/arduino_nano_i2c_chapter2
#include <PZEM004T.h>
#include <TimerOne.h>
#include <EEPROM.h>

#include <Wire.h>
// SCL - A5
// SDA - A4

#define DEBUG

#ifdef DEBUG
#include <SoftwareSerial.h>
#endif

#define DEVICE_ADDR 0x10

#define CMD_NO_COMMAND 0xFF;
#define CMD_GET_GPIO_MODE 0x00
#define CMD_SET_GPIO_MODE 0x01
#define CMD_GET_DUTY 0x02
#define CMD_SET_DUTY 0x03
#define CMD_GET_PZEM_DATA 0x04
#define CMD_GET_UPTIME 0x05



#define EEPROM_GPIO_MODE_ADDR 0x00
#define EEPROM_DUTY_ADDR 0x01

volatile uint8_t duty_changed = 0;
volatile uint8_t gpio_mode_changed = 0;
volatile uint32_t uptime = 0;
volatile uint8_t command = 255;
/*
 * registers
 * reg: 00 - gpio_mode
 * reg: 01 - duty
 * reg: 02 - float - voltage
 * reg: 03 - float - current
 * reg: 04 - float - power
 * reg: 05 - float - energy
 */
enum  {
  GPIO_MODE_NORMAL, // 0
  GPIO_MODE_DUTY    // 1
} gpio_mode_e;

#define PZEM_RX 2
#define PZEM_TX 3

#define RELAY_PIN 13

#define DUTY 100
#define DUTY_TIME 10  //msec
#define PZEM_READ_INTERVAL 1000 // msec

#define RESPONSE_DATA_SIZE 18


//PZEM004T pzem(PZEM_RX,PZEM_TX); 
//IPAddress ip(192,168,1,1);

#ifdef DEBUG
  SoftwareSerial mySerial(10, 11);
#endif

unsigned long time_pzem = 0;
unsigned long time_process_mode = 0;
unsigned long time_process_duty = 0;

volatile float pzem_data[4] = {0.0, 0.0, 0.0, 0.0};

volatile uint8_t  duty = DUTY;
volatile uint8_t gpio_mode;

int reg = duty;
int er = 50;

int mode= 0; // 0 - volt, 1 - curr, 2 - power, 3 - counter
int mode_interval = 0; //  1,10,20 - volt, 2,4,6,8 - curr, 3,5,7,9 - pow, 60 - counter


void gpio_set(uint8_t pin, uint8_t st) {
      if ( st ) {
        if ( pin < 8 )
          bitSet(PORTD, pin);
        else 
          bitSet(PORTB, pin ^ 8);
    } else {
      if ( pin < 8 )
        bitClear(PORTD, pin);
      else  
        bitClear(PORTB, pin ^ 8);
    }  
}

void ssr_pwm3(){
  bool state = false;
  //delayMicroseconds(5);
  delay(1);
  er = er - reg;
  if ( er < 0) {
    er = er + 99;
    state = true;
  }
  else
  {
    state = false;
  }

  gpio_set(RELAY_PIN, state);
      
}

void save_duty(uint8_t _duty) {
  uint8_t value = EEPROM.read(EEPROM_DUTY_ADDR);  
  if (value == _duty) return;
  EEPROM.write( EEPROM_DUTY_ADDR, _duty );
}

void save_gpio_mode(uint8_t _gpio_mode) {
  uint8_t value = EEPROM.read(EEPROM_GPIO_MODE_ADDR);  
  if (value == _gpio_mode) return;
  EEPROM.write( EEPROM_GPIO_MODE_ADDR, _gpio_mode );  
}

uint8_t load_duty() {
  uint8_t val = EEPROM.read(EEPROM_DUTY_ADDR);  
  if ( val > 100 ) val = 100;
  return val;
}

uint8_t load_gpio_mode() {
  uint8_t val = EEPROM.read(EEPROM_GPIO_MODE_ADDR);  
  if ( val > 1 ) val = 1; 
  return val;
}


void init_duty_timer() {

              #ifdef DEBUG
                mySerial.println("\ninit_duty_timer... ");
            #endif
            Serial.println("\ninit_duty_timer... ");
    Timer1.initialize( DUTY_TIME*1000); 
    Timer1.attachInterrupt(ssr_pwm3);  
}

void deinit_duty_timer(){
              #ifdef DEBUG
                mySerial.println("\ndeinit_duty_timer... ");
                
            #endif
  Serial.println("\ndeinit_duty_timer... ");
  Timer1.stop();
  Timer1.detachInterrupt();

                
                uint8_t st = duty > 0 ? 1 : 0 ;

                Serial.print("\nset gpio to "); Serial.println(st);
  gpio_set( RELAY_PIN, st);
}

void setup() {
  // put your setup code here, to run once:
  duty = load_duty();
  gpio_mode = load_gpio_mode();
  
  Serial.begin(9600);
  Wire.begin( DEVICE_ADDR  );  
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
#ifdef DEBUG  
  mySerial.begin(9600);
  mySerial.println("\nsystem started");
#endif
  Serial.println("\nsystem started");
  //uart_pzem.begin(9600);
  //pzem.setAddress(ip);

  pinMode(RELAY_PIN, OUTPUT);

  
  if ( gpio_mode == GPIO_MODE_DUTY ) {
    init_duty_timer();
  }
}


void get_pzem_data() {
  if ( millis() - time_pzem >= PZEM_READ_INTERVAL ) {
    Serial.print(".");
    // читаем pzem
    if ( (mode_interval == 1 || mode_interval % 10 == 0) && mode_interval != 60 ) {
      mode = 0;
    } else if ( mode_interval % 2 == 0 && mode_interval % 10 > 0 ) {
      mode = 1;
    } else if ( mode_interval % 2 > 0 && mode_interval != 1 ) {
      mode = 2;
    } else if ( mode_interval == 60 ) {
      mode = 3;
    }
    mode_interval++;
    if (mode_interval > 60 ) mode_interval = 0;

    float temp  = 0;
    if ( mode == 0 ) {
         #ifdef DEBUG 
            temp = random(1900, 2400) / 10.0;
         #else
            //temp = pzem.voltage(ip);
         #endif
         
         if ( temp >= 0.0 ) {
          pzem_data[0] = temp;   
         }
    } else if ( mode == 1 ) {
      #ifdef DEBUG
        temp = random(0, 2000) / 100.0;
      #else
        //temp = pzem.current(ip);
      #endif
      
      if ( temp >= 0.0 ) {
        pzem_data[1] = temp;
      }
    } else if ( mode == 2 ) {
        #ifdef DEBUG
          temp = pzem_data[0] * pzem_data[1];
        #else
          //temp = pzem.power(ip);
        #endif

        if ( temp >= 0.0 ) {
          pzem_data[2] = temp;    
        }
    } else {   
        #ifdef DEBUG
          pzem_data[3] += random(1, 10);
        #else
          //pzem_data[3] = pzem.energy(ip);
        #endif 
    }

    time_pzem = millis();
  }  
}

void process_duty(){
  if ( millis() - time_process_duty < 100 ) return;
  if ( duty_changed ) {
    Serial.println("\n duty_changed... ");
    if ( duty > 100 ) duty = 100;
    reg = duty;
    if ( gpio_mode == GPIO_MODE_NORMAL ) {
                 uint8_t st = duty > 0 ? 1 : 0 ;

                Serial.print("\nset gpio to "); Serial.println(st);
          gpio_set( RELAY_PIN, st);     
    }
    save_duty( duty ); 
    duty_changed = 0;
  }
  time_process_duty = millis();  
}

void process_gpio_mode(){
  if ( millis() - time_process_mode < 100 ) return;
   if ( gpio_mode_changed ) {
      Serial.println("\n gpio_mode_changed... ");
      
      if ( gpio_mode == GPIO_MODE_NORMAL ) {
          deinit_duty_timer();
      } else {
        init_duty_timer();
      }    
      save_gpio_mode( gpio_mode );
      gpio_mode_changed = 0;
   }
   time_process_mode = millis();
}

void loop() {
  uptime = millis()/ 1000;
  get_pzem_data();
  process_gpio_mode();
  process_duty();
}

// master write
void receiveEvent(int howMany) {
  if ( howMany < 1 ) return;

  uint8_t *buf = malloc( howMany );
  uint8_t idx = 0;
  while ( Wire.available()) { // loop through all but the last
    buf[idx] = Wire.read(); // receive byte as a character
    idx++;
  }
  
  command = buf[0];
 //Serial.print("command = 0x"); Serial.println(command, HEX );
  if ( howMany > 1) {
    uint8_t val = buf[1];
    //Serial.print("buf[1] = "); Serial.println( buf[1] );
    if ( command == CMD_SET_GPIO_MODE ) {
      gpio_mode_changed = val != gpio_mode;
      gpio_mode = val;
      //Serial.print("mode changed = "); Serial.println(  gpio_mode_changed );
    } else if ( command == CMD_SET_DUTY ) {
      duty_changed = val != duty;
      duty = val;
      //Serial.print("duty changed = "); Serial.println(  duty_changed );
    } 
  }

  free(buf);
}

// master read
void requestEvent(){
  uint8_t crc = 0;
  if ( command == CMD_GET_GPIO_MODE ) {
    Wire.write( gpio_mode);
    // crc 
    crc = (DEVICE_ADDR + command + gpio_mode) & 0xFF;
    Wire.write( crc );
  } else if ( command == CMD_GET_DUTY ) {
    Wire.write( duty );
    // crc
    crc = (DEVICE_ADDR + command + duty) & 0xFF;
  } else if ( command == CMD_GET_PZEM_DATA ) {
    uint8_t *buf = (uint8_t *) &pzem_data;
    //uint8_t b[17];
    Wire.write( buf, sizeof(float) * 4 );
    // crc
    //crc = (DEVICE_ADDR + command);
    //for ( uint8_t i=0; i<16; i++ ) {
    //  crc += buf[i];
    //}
    //crc = crc & 0xFF;
    //memcpy(&b[0], buf, 16);
    //b[16] = crc; 
    //Wire.write( &b[0], 17 );
    
  } else if ( command == CMD_GET_UPTIME ) {
    uint8_t *buf = (uint8_t *)&uptime;
    Wire.write( buf, sizeof(uint32_t) );
    // crc
    crc = (DEVICE_ADDR + command);
    for ( uint8_t i=0; i<4; i++ ) {
      crc += buf[i];
    }
    crc = crc & 0xFF;
    Wire.write( crc );    
  }
  command = CMD_NO_COMMAND;
}
