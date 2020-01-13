#include <Arduino.h>
#include <M5StickC.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <driver/adc.h>

#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

void ClearScreen()
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE);  // Adding a background colour erases previous text automatically
    
    M5.Lcd.setCursor(3, 3);  
    M5.Lcd.setTextSize( 3 );
}


void setup()   {

  Serial.begin( 57600 );

  M5.begin();
  M5.MPU6886.Init();

  //M5.Lcd.setBrightness((uint8_t)100);
  M5.Lcd.setRotation(3);
  ClearScreen();

  if ( !cap.begin(0x5A) ) {
    M5.Lcd.print("Touch not found");  
    while(1);
  }

  M5.Lcd.print("Touch OK");  
  delay(200);
}

int values[20];

void loop() {

  int soglia = 20;

  ClearScreen();

  for( int idx=1 ; idx<8 ; idx++ ) {
    values[idx] = cap.baselineData(idx) - cap.filteredData(idx);

    if( values[idx] > soglia ) {
      M5.Lcd.setTextColor(TFT_WHITE);
    }
    else {
      M5.Lcd.setTextColor(TFT_RED);
    }

    M5.Lcd.print( String( idx ) );  

  }
	M5.update();
  delay(40);
}
