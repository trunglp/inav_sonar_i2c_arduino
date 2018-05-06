/*
Code modify by trunglp
From Pawe Spychalski
Pin A3 connect to Triger
Pin A2 connect to Echo
*/

#include "WireMW.h"

#define USE_HCSR04
#define I2C_SLAVE_ADDRESS 0x14
static uint32_t _sonar_timer = 0;

#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif

#define STATUS_OK 0
#define STATUS_OUT_OF_RANGE 1
#define LED_PIN 13 

uint8_t i2c_regs[] =
{
    0, //status
    0, //older 8 of distance
    0, //younger 8 of distance
};
const byte reg_size = sizeof(i2c_regs);
volatile byte reg_position = 0;


void requestEvent()
{ 
 Wire.write(i2c_regs,3); 
}


void receiveEvent(uint8_t bytesReceived) {
    if (bytesReceived < 1) {
        // Sanity-check
        return;
    }

    if (bytesReceived > TWI_RX_BUFFER_SIZE)
    {       
        return;
    }
    reg_position = Wire.read();
    bytesReceived--;
    if (!bytesReceived) {
        // This write was only to set the buffer for next read
        return;
    }
    
    // Everything above 1 byte is something we do not care, so just get it from bus as send to /dev/null
    while(bytesReceived--) 
    {
      Wire.read();
    }
}



void blink_sonar_update()
{

  uint32_t now = millis();


  if(_sonar_timer < now)//update sonar readings every 50ms
  {
   _sonar_timer = now + 50;
   Sonar_update();
  }

    
  
}


volatile uint32_t Sonar_starTime = 0;
volatile uint32_t Sonar_echoTime = 0;
volatile uint16_t Sonar_waiting_echo = 0;

void Sonar_init()
{
  // Pin change interrupt control register - enables interrupt vectors
  PCICR  |= (1<<PCIE1); // Port C
 
  // Pin change mask registers decide which pins are enabled as triggers
  PCMSK1 |= (1<<PCINT10); // pin 2 PC2
  
  DDRC |= 0x08; //triggerpin PC3 as output

  Sonar_update();

}



ISR(PCINT1_vect) {

    //uint8_t pin = PINC;

    if (PINC & 1<<PCINT10) {     //indicates if the bit 0 of the arduino port [B0-B7] is at a high state
      Sonar_starTime = micros();
    }
    else {
      Sonar_echoTime = micros() - Sonar_starTime; // Echo time in microseconds
     ///Serial.println("helo:" + Sonar_echoTime);

      reg_position = Sonar_echoTime / 58;
      if ((Sonar_echoTime <= 700*58) && (reg_position < 400) ){     // valid distance
        reg_position = Sonar_echoTime / 58;
        Serial.println(reg_position);


        
          i2c_regs[0] = STATUS_OK;

    
   
          if (reg_position <10) {
            digitalWrite(LED_PIN, HIGH);
          } else {
            digitalWrite(LED_PIN, LOW);
          }

          i2c_regs[1] = reg_position >> 8;
          i2c_regs[2] = reg_position & 0xFF;


    
      }
      else
      {
      // No valid data
        reg_position = -1;
        //Serial.println(-1);
        i2c_regs[0] = STATUS_OUT_OF_RANGE;
        Serial.println("error");
        
      }
      Sonar_waiting_echo = 0;
    }
}


void Sonar_update()
{
 

  if (Sonar_waiting_echo == 0)
  {
    // Send 2ms LOW pulse to ensure we get a nice clean pulse
    PORTC &= ~(0x08);//PC3 low    
    delayMicroseconds(2);
   
    // send 10 microsecond pulse
    PORTC |= (0x08);//PC3 high 
    // wait 10 microseconds before turning off
    delayMicroseconds(10);
    // stop sending the pulse
    PORTC &= ~(0x08);//PC3 low
   
    Sonar_waiting_echo = 1;
  }

}


void setup() {
Serial.begin(38400);

Sonar_init();

  //Start I2C communication routines
  Wire.begin(I2C_SLAVE_ADDRESS);               // DO NOT FORGET TO COMPILE WITH 400KHz!!! else change TWBR Speed to 100khz on Host !!! Address 0x40 write 0x41 read
  Wire.onRequest(requestEvent);          // Set up event handlers
  Wire.onReceive(receiveEvent);

}

void loop(){
  blink_sonar_update(); 
}
