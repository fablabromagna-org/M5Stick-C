#include <Arduino.h>
#include <M5StickC.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <driver/adc.h>

#include <Wire.h>
#include "Adafruit_MPR121.h"
#include "Artnet.h"

// I2C and MPR121 Stuff
TwoWire tw = TwoWire( 0x5A );
Adafruit_MPR121 mpr121 = Adafruit_MPR121();


// WiFi stuff
const char* ssid = "TP-Link_2128";
const char* pwd = "65041758";
const IPAddress ip(192, 168, 0, 60);
const IPAddress gateway(192, 168, 1, 1);
const IPAddress subnet(255, 255, 255, 0);

ArtnetSender artnet;
uint32_t universe = 1;

const uint16_t size = 512;
uint8_t data[size];
uint8_t value = 0;

void ClearScreen()
{
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE);  // Adding a background colour erases previous text automatically
    
    M5.Lcd.setCursor(3, 3);  
    M5.Lcd.setTextSize( 3 );
}

//
// Setup
//
void setup()   {

  M5.begin();
  M5.MPU6886.Init();

  //M5.Lcd.setBrightness((uint8_t)100);
  M5.Lcd.setRotation(3);
  ClearScreen();

  // Su M5Stick-C la porta grove è usabile come I2C
  // pin esterno grove (giallo) SCL su M5StickC = 33
  // pin interno grove (bianco) SDA su M5StickC = 32
  // E' necessario creare un oggetto TwoWire e configurarlo su pin diversi dal solito
  tw.begin( 32, 33 );
  
  // e poi lo si passa alla lib di gestione del MPR121
  if ( !mpr121.begin( 0x5A, &tw ) ) {
    M5.Lcd.print("Touch not found");  
    while(1);
  }

  M5.Lcd.print("Touch OK");  
  delay(50);

    // WiFi stuff
  WiFi.begin(ssid, pwd);
  //WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  Serial.print("WiFi connected, IP = "); Serial.println(WiFi.localIP());

  artnet.begin("192.168.0.62");

  ClearScreen();
  M5.Lcd.print(WiFi.localIP());  
  delay(1000);

}

int values[20];
int soglia = 20;


//
// Loop principale
//
void loop() {
  
  ClearScreen();

  for( int idx=1 ; idx<8 ; idx++ ) {
    values[idx] = mpr121.baselineData(idx) - mpr121.filteredData(idx);

    if( values[idx] > soglia ) {
      M5.Lcd.setTextColor(TFT_WHITE);
      
      //value = 255;
      //memset(data, value, size);
      data[idx] = 0xff;
      artnet.set(idx, data, size);
      artnet.streaming(); 
    }
    else {
      M5.Lcd.setTextColor(TFT_RED);

      data[idx] = 0x00;
      artnet.set(idx, data, size);
      artnet.streaming(); 
    }

    M5.Lcd.print( String( idx ) );  
  }
   
	M5.update();


  delay(40);
}
