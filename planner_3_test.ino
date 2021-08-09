// ---------------------------------------------------------------- /
// Arduino I2C Scanner
// Re-writed by Arbi Abdul Jabbaar
// Using Arduino IDE 1.8.7
// Using GY-87 module for the target
// Tested on 10 September 2019
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.

//The AS5600 sensor is addressd at 0x36
// ---------------------------------------------------------------- /

#include <Wire.h>     //include Wire.h library
#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1305.h>
#include <EEPROM.h>

// Used for software SPI
#define OLED_CLK 13
#define OLED_MOSI 11

// Used for software or hardware SPI
#define OLED_CS 4
#define OLED_DC 8

// Used for I2C or SPI
#define OLED_RESET 9

#define UP_SW 14
#define DOWN_SW 15
#define CAL_SW 3
#define GO_SW 2

// define the resoluation of a sensor count 8.5" / 4096
#define CNT 0.002
#define AS5600 0x36

byte agc,st,rhb,rlb,hb,lb;
byte conf_l,conf_h;

byte address = AS5600;
byte i2c_return = 0;
int ref_value=0;
byte ep1,ep2;
int eeprom_correct;
float height_in,height_mm;
//int nDevices = 0;
int sensor_cnt;
String str;
float planner_set;
int sw_up;

bool up_sw,down_sw,cal_sw,go_sw;

// software SPI
//Adafruit_SSD1305 display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
// hardware SPI - use 7Mhz (7000000UL) or lower because the screen is rated for 4MHz, or it will remain blank!
Adafruit_SSD1305 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS, 7000000UL);


void setup()
{
  Wire.begin();
  Serial.begin(9600); // The baudrate of Serial monitor is set in 9600
  while (!Serial); // Waiting for Serial Monitor
  Serial.println("\nbegin");

  ep1 = EEPROM.read(0);
  ep2 = EEPROM.read(1);
  
  Serial.print(ep1);
  Serial.print("   ");
  Serial.println(ep2);
  
  eeprom_correct = ep1*256 + ep2;

  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);

  pinMode(14,INPUT_PULLUP);
  pinMode(15,INPUT_PULLUP);

  if ( ! display.begin(0x3C) ) { //for SPI no device addess is required
    Serial.println("Unable to initialize OLED");
    while (1) yield();
  }

// set hystersis value on sensor
  Wire.beginTransmission(address);
  Wire.write(0x08);
  Wire.write(0x0C);
  Wire.endTransmission();
  
//read sensor conf registers
  Wire.beginTransmission(address);
  Wire.write(0x07);
  Wire.endTransmission();
  Wire.requestFrom(0x36,2);

  conf_h = Wire.read();
  conf_l = Wire.read();

  // init done
  display.setRotation(2);
  display.display(); // show splashscreen
  delay(1000);
  display.setTextColor(WHITE);
/*
  display.clearDisplay();
    // text display tests
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.display();
  delay(3000);
*/
   Wire.beginTransmission(address);
   byte i2c_return = Wire.endTransmission();

   switch (i2c_return){
   case 0:  
      str = "sensor ok";
      break;
    case 4:
      str = "no sensor found";
      break;
    default:
      str = "sensor error";
      break;
   }
   
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println(str);
    display.display();
    delay(2000);

    read_5600();
    
}

void loop()
{
   int sensor_cnt = 0;
       
   read_switches();

   read_5600();
      
   sensor_cnt = sensor_cnt + (rhb*256 + rlb);   

   height_in = (eeprom_correct - sensor_cnt) * CNT + 0.75;

   height_mm = height_in * 25.4;

   if(sw_up)
   {
     planner_set = height_in + 0.1;
   }
   else
   {
    
   }



   update_display();

  // when the switch is pushed the planner should be set for a height of 0.75"
  // this will store the sensor values to EEPROM and display 0.75"
  if(cal_sw)
  { cal_sw = false;
    ref_value = sensor_cnt;
    EEPROM.write(0,rhb);
    EEPROM.write(1,rlb);
    eeprom_correct = rhb*256 + rlb;
  }
//  Serial.println("end loop");
  delay(500); // wait 5 seconds for the next I2C scan
}

void read_5600() {

     Wire.beginTransmission(address);
     Wire.write(0x0B);
     Wire.endTransmission();
     Wire.requestFrom(0x36,1);
     st = Wire.read();

     Wire.beginTransmission(address);
     Wire.write(0x1A);
     Wire.endTransmission();
     Wire.requestFrom(0x36,1);
     agc = Wire.read();

     Wire.beginTransmission(address);
     Wire.write(0x0C);
     Wire.endTransmission();
     Wire.requestFrom(0x36,4);

     rhb = Wire.read();
     rlb = Wire.read();
     hb = Wire.read();
     lb = Wire.read();
}

void update_display() {

     display.clearDisplay();
     display.setTextSize(3);
     display.setCursor(0,0);
     display.print(height_in,3);

     display.println("\"");
 
     display.setTextSize(2);
     display.print(height_mm,2);
     display.println("mm");
     display.setCursor(0,57);
     display.setTextSize(1);
     display.print(hb*256+lb);
     display.print("  ");
     display.print(st,HEX);
     display.print("  ");
     display.print(agc,HEX);
     display.print("  ");
     display.print(conf_h,HEX);
     display.print("  ");
     display.println(conf_l,HEX);
 
     display.display();
}

void read_switches() {

  if(!digitalRead(UP_SW))
  { delay(50);
    if (!digitalRead(UP_SW))
      up_sw = true;
  }else
    up_sw = false;

  if(!digitalRead(DOWN_SW))
  { delay(50);
    if (!digitalRead(DOWN_SW))
      down_sw = true;
  }else
    down_sw = false;

  if(!digitalRead(GO_SW))
  { delay(50);
    if (!digitalRead(GO_SW))
      go_sw = true;
  }else
    go_sw = false;

  if(!digitalRead(CAL_SW))
  { delay(50);
    if (!digitalRead(CAL_SW))
      cal_sw = true;
  }else
    cal_sw = false;
}
