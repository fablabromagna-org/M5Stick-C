/*
StickBLE

Esempio di utilizzo di M5Stick per una connessione BLE.

Realizzato in Dicembre 2019 da Maurizio Conti 
maurizio.conti@fablabromagna.org

Licenza GPLv3

Testato su M5Stick-C 
- led ROSSO -> D10
- Pulsante A (Grosso) -> D37 

Elenca le seriali sul mac
ls /dev/tty.*

*/

#include <Arduino.h>
#include <M5StickC.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

#include <driver/adc.h>

// Mapping I/O
#define LED_ROSSO 10

// velocità di update delle notifiche BLE
#define TIMER_100 200

bool deviceConnected = false;
bool oldDeviceConnected;
int stato = 0;
bool btnStatus = false;

// serve come indice per il puntino sull'LCD
// quando è in attesa di connessione BLE
int waiting = 0; 
int waitingColor = TFT_WHITE; 

unsigned long timer100ms = 0;
unsigned long timer1ms = 0;

uint8_t localSliderValue = 0;
uint8_t oldLocalSliderValue = 0;

 // UUID BLE inventati da me...

// Uno mi serve per esporre il service
#define SERVICE_UUID "ABCD1234-0aaa-467a-9538-01f0652c74e8"

// All'interno del service ho diversi valori che scambio tra ESP32 e cellulare
// Ognuno di questi valori (characteristic per BLE...) ha un suo UUID (sempre inventato...)
#define SLIDER_VALUE_UUID "ABCD1235-0aaa-467a-9538-01f0652c74e8"
#define BUTTON_VALUE_UUID "ABCD1236-0aaa-467a-9538-01f0652c74e8"
#define ADC_VALUE_UUID "ABCD1237-0aaa-467a-9538-01f0652c74e8"

// Handler dei due valori scambiati
BLECharacteristic *sliderCharacteristic = NULL;
BLECharacteristic *buttonCharacteristic = NULL;
BLECharacteristic *adcCharacteristic = NULL;

// Gestione eventi server BLE
class MyServerCallbacks : public BLEServerCallbacks {

    // onConnect e onDisconnect, sono due metodi virtuali
    // dichiarati nella classe base.
    // Qui vengono ridefiniti per eseguire azioni utili
    // in questo progetto... ad esempio far lampeggaire led
    // durante la connessione etc...
    void onConnect(BLEServer* pServer) {

      // Quando lo smartphone si collega...

      // muovo la variabile locale che mi indica questo stato
      deviceConnected = true;

      // lo segnalo sulla seriale...
      Serial.println("BLE Client Connected");
      
      // lo segnalo con un blink sul led blu a bordo di ESP32
      digitalWrite( LED_ROSSO, HIGH );
      delay(10);
      digitalWrite( LED_ROSSO, LOW );
      delay(10);
    };

    void onDisconnect(BLEServer* pServer) {

      // Quando lo smartphone si scollega...

      // muovo la variabile locale che mi indica questo stato
      deviceConnected = false;

      // lo segnalo sulla seriale...
      Serial.println("BLE Client Disconnected");

    }
};

// Le characteristic BLE sono come registri remoti con una gestione a eventi.
// Per gestire questi eventi utilizziamo un CallBack lato esp32.
// La notifica parte lato smartphone muovendo uno slider.
// Il valore di questo slider arriva a esp32 il quale varia la luminosità di un led
class SliderValueCallbacks: public BLECharacteristicCallbacks {
    
    // Come per la onConnect-onDisconnect, qui abbiamo
    // alcuni metodi virtuali che vengono ridefiniti ai nostri scopi
    // sono onWrite, onRead, onNotify, onStatus 
    void onWrite(BLECharacteristic *pCharacteristic) {
      
      // Quando lato smartphone parte una notifica di scrittura,
      // ci troviamo qui...

      // Segnalo questo movimento di dati con un blink
      digitalWrite( LED_ROSSO, LOW );
      delay(10);
      digitalWrite( LED_ROSSO, HIGH );

      // Leggo quindi il nuovo valore che mi arriva dallo smartphone 
      // e lo conservo in una variabile locale
      uint8_t* v = pCharacteristic->getData();
      localSliderValue = v[0];

      // Segnalo poi sulla seriale (debug)
      Serial.print("New slider value ");
      Serial.print( localSliderValue );
      Serial.println( "." );      
      
      // Attivo il PWM sul led che cambierà di intensità a seconda della
      // posizione dello slider sullo smartphone
      //long lux = map( localSliderValue, 0, 100, 0, 255);
      //ledcWrite(0, lux);      
    }
};

void ClearScreen()
{
    M5.Lcd.fillScreen(TFT_GREEN);
    M5.Lcd.setTextColor(TFT_WHITE);  // Adding a background colour erases previous text automatically
    
    M5.Lcd.setCursor(3, 3);  
    M5.Lcd.setTextSize(2);
    M5.Lcd.print("FLR StickBLE");  

    M5.Lcd.setCursor(3, 20);  
}

void GestioneValoreAnalogicoLCD()
{
  //  Stampa il valore che arriva dallo smartphone...
  if( localSliderValue != oldLocalSliderValue ){
    oldLocalSliderValue = localSliderValue;
    ClearScreen();
    M5.Lcd.setTextColor(TFT_NAVY);  
    M5.Lcd.setCursor(3, 40);  
    M5.Lcd.print( String(localSliderValue) );  
  }

}

void GestioneLCD()
{
  // gestione colore del puntino
  if ( waiting == 10 ){
    waiting=0;
    
    if( waitingColor == TFT_BLUE ) {
      waitingColor = TFT_WHITE;
    }
    else {
      waitingColor = TFT_BLUE;
    } 
  }  
  M5.Lcd.setTextColor(waitingColor);  

  // gestione posizione (ritorna a capo)
  if( waiting == 0 ) {
    M5.Lcd.setCursor(3, 20);  
  }
  
  // Quando cambia da connesso/disconnesso, pulisce lo schermo
  if( deviceConnected != oldDeviceConnected )
  {
    oldDeviceConnected=deviceConnected;
    ClearScreen();
  }

  // Gestione messaggio: se stampa il puntino o il messaggio
  if ( deviceConnected ) {
    M5.Lcd.setCursor(3, 20);  
    M5.Lcd.print("Connesso!");  
  }
  else {
    M5.Lcd.print(".");  
  }

  waiting++;
  delay(150);

}

//
//
// Setup
//
//
void setup() {

  M5.begin();
  M5.MPU6886.Init();

  //M5.Lcd.setBrightness((uint8_t)100);
  M5.Lcd.setRotation(3);
  ClearScreen();

  delay(500);

  // Usiamo Led Rosso 
  pinMode( LED_ROSSO, OUTPUT );

  Serial.begin(115200);
  Serial.println("\n\nM5Stick Startup");

  // BLE Device -------------------------------------------------
  // racchiude le funzionalita BLE di ESP32 (Kolbam).
  BLEDevice::init( "Sensore Techno Back Brace" );
  Serial.println("Sensore TBB acceso!");

  // BLE Server -------------------------------------------------
  // Grazie a BLEDevice Creaiamo un server BLE con relativo callback
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // BLE Service ------------------------------------------------
  // Un server BLE espone servizi.
  // Un servizio a sua volta è un container con N characteristics.
  // Esponiamo lo UUID del service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  //
  // BLE Service ------------------------------------------------
  //
  // BLE Slider Characteristics ---------------------------------
  // Nel nostro service ci aggiungiamo due characteristic:
  // - una per ricevere il valore dello slider dallo smartphone
  // - una per comunicare allo smartphone lo stato del nostro pulsante esp32
  // 
  // Questa characteristic è di WRITE perchè può essere scritta da remoto 
  // ed è NOTIFY perchè può scatenare un evento lato esp32
  sliderCharacteristic = pService->createCharacteristic( 
      SLIDER_VALUE_UUID, 
      BLECharacteristic::PROPERTY_WRITE );
  
  // Attacco l'event handler per gestire le notifiche
  sliderCharacteristic->setCallbacks(new SliderValueCallbacks());

  // Nota:  
  // Non ho ben capito come usare un descrittore.
  // funziona anche senza ma se serve ecco come si usa...
  //sliderCharacteristic->addDescriptor(new BLE2902());

  // BLE button Characteristics ---------------------------------
  // Questa characteristic è di READ perchè può essere letta da remoto 
  buttonCharacteristic = pService->createCharacteristic( 
      BUTTON_VALUE_UUID, 
      BLECharacteristic::PROPERTY_READ);
 
  // BLE adc Characteristics ---------------------------------
  // Questa characteristic è di READ perchè può essere letta da remoto 
  adcCharacteristic = pService->createCharacteristic( 
      ADC_VALUE_UUID, 
      BLECharacteristic::PROPERTY_READ);

  // BLE Start -----------------------------------------------
  // Si parte!
  pService->start();

  // BLE Advertising -----------------------------------------
  // Pubblichiamo il nostro SERVICE perchè sia visibile.
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

  // Visualizza lo stato...
  GestioneLCD();
  GestioneValoreAnalogicoLCD();

  float accX = 0;
  float accY = 0;
  float accZ = 0;
  M5.MPU6886.getAccelData(&accX,&accY,&accZ);
 
  // Tick!
  if ( deviceConnected ) {

    // Le notifiche BLE ogni 100 mS (TIMER_100)  
    if( millis() > timer100ms ) {

      // Tempo scaduto, sono passati altri TIMER_100 ms
      // Ricarichiamo il timer della stufa...
      timer100ms = millis() + TIMER_100;

    	if(M5.BtnA.wasPressed()) {

        // aggiorniamo il valore della characteristic
        btnStatus = !btnStatus;
        uint16_t btnVal = (uint16_t)btnStatus;
        buttonCharacteristic->setValue( btnVal );
      
        // questa notifica scatena la callback lato App mobile
        buttonCharacteristic->notify();

        Serial.print("Bottone: ");
        Serial.println( btnVal );
      }
      
      //int adcVal = hall_sensor_read(); // Read HALL GPIO36
      //int adcVal = hallRead(); // Read HALL
      int adcVal = (int)((accX+1)*512);
      adcCharacteristic->setValue( adcVal );
      adcCharacteristic->notify();
      
      Serial.print("PosX: ");
      Serial.println( adcVal );
    }
  }
  else
  {
    delay(150);
    Serial.print("."); // keep alive
  }    
}
