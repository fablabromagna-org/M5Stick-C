/*
StickBLE
Esempio di utilizzo minimo di M5Stick per una connessione BLE.

Realizzato in Dicembre 2019 da Maurizio Conti 
maurizio.conti@fablabromagna.org

Licenza GPLv3
Testato su M5Stick-C 

A volte USB non vede M5Stick... verifica seriali
ls /dev/tty.*

*/

#include <Arduino.h>
#include <M5StickC.h>
#include <BLEDevice.h>
#include <BLEServer.h>

void Stampa( int val );

// Mapping I/O
#define LED_ROSSO 10

bool deviceConnected = false;
bool oldDeviceConnected;

uint8_t localSliderValue = 0;
uint8_t oldLocalSliderValue = 0;

// UUID BLE
#define SERVICE_UUID "ABCD1234-0aaa-467a-9538-01f0652c74e8"
#define SLIDER_VALUE_UUID "ABCD1235-0aaa-467a-9538-01f0652c74e8"
BLECharacteristic *sliderCharacteristic = NULL;

// Gestione eventi server BLE
class MyServerCallbacks : public BLEServerCallbacks {
  
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE Client Connected");
      
      // Accendo LCD e stampo valore iniziale
      Stampa( 0 );
    };

    void onDisconnect(BLEServer* pServer) {

      deviceConnected = false;
      Serial.println("BLE Client Disconnected");

      // Spengo LCD
      M5.Lcd.begin();
    }
};

// Gestione eventi Characteristics
class SliderValueCallbacks: public BLECharacteristicCallbacks {
    
    void onWrite(BLECharacteristic *pCharacteristic) {

      uint8_t* v = pCharacteristic->getData();
      localSliderValue = v[0];

      // Stampo il valore sull'LCD
      Stampa( localSliderValue );

      // Segnalo poi sulla seriale (debug)
      Serial.print("New slider value ");
      Serial.print( localSliderValue );
      Serial.println( "." );            
    }
};


//
//
// Setup
//
//
void setup() {

  M5.begin();
  M5.Lcd.begin();
  M5.Lcd.setRotation(3);

  // Blink su Led Rosso 
  pinMode( LED_ROSSO, OUTPUT );
  for( int cnt=0 ; cnt<10 ; cnt++ ){
    digitalWrite( LED_ROSSO, cnt & 1 );
    delay(100);
  }

  Serial.begin(115200);
  Serial.println("\n\nM5Stick Startup");

  // BLE Device -------------------------------------------------
  BLEDevice::init( "Sensore Techno Back Brace" );
  Serial.println("Sensore TBB acceso!");

  // BLE Server -------------------------------------------------
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // All'interno del server ci abita uno o più BLE Service ------
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Un Service contiene N BLE Characteristic -------------------
  sliderCharacteristic = pService->createCharacteristic( 
      SLIDER_VALUE_UUID, 
      BLECharacteristic::PROPERTY_WRITE );
  sliderCharacteristic->setCallbacks(new SliderValueCallbacks());

  // Per descrivere una characteristic, in BLE non basta una string.
  // Ci sono strutture dati ben precise e codificate.
  // Ad esempio la  2901 (characteristic user description)
  BLEDescriptor sliderDescriptor(BLEUUID((uint16_t)0x2901));  
  sliderDescriptor.setValue("Posizione dello slider da 0 a 100");
  sliderCharacteristic->addDescriptor(&sliderDescriptor);

  pService->start();

  // BLE Advertising -----------------------------------------
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println( "Partiti..." );
}

//
//
//  Loop
//
//
void loop() {

	M5.update();
  if( !deviceConnected )
    Serial.print("."); // keep alive

  delay(200);
}

//
// Stampa valore su LCD
//
void Stampa( int val )
{
    M5.Lcd.fillScreen(TFT_NAVY);
    M5.Lcd.setTextColor(TFT_WHITE);
        
    M5.Lcd.setCursor(3, 3);  
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("FLR StickBLE");  

    M5.Lcd.setCursor(15, 30);  
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(TFT_YELLOW);  
    M5.Lcd.print(localSliderValue);  
}
