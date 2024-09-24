/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara

    link to non-volatile storage stuff https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Preferences.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define PRINT         0

// SDA is 21, SCL is 22
#define SERVER_NAME     "Big Fan"
#define UP_PIN          18
#define DOWN_PIN        4
#define BACKLIGHT_PIN   23

Preferences last_data;

bool deviceConnected = false;
bool deviceConnected_prev = false;

byte up_curr_val, down_curr_val, back_curr_val;
byte up_prev_val = 0;
byte down_prev_val = 0;
byte back_prev_val = 0;
byte level;
byte backlight_val;
int data;

// Setting up variables related to OLED display
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32
#define SCREEN_ADDRESS  0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool display_working = true;
bool display_off = false;
unsigned long int display_on_time;
bool button_pressed = true;

#define SERVICE_UUID        "00a9cede-ba07-453c-8a6f-f1ddb24e23ac"
BLECharacteristic levelCharacteristic("4a116374-2b2c-4ad7-bb00-3b59ca3d2e68", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);  // BLECharacteristic::PROPERTY_NOTIFY


class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setup() {
  #if PRINT
    Serial.begin(9600);
    Serial.println("Starting BLE work!");
  #endif

  last_data.begin("my_app", false);  // start partition with namespace "my_app" in read/write mode
  // Remove all preferences under the opened namespace
  //preferences.clear();
  // Or remove the level key only
  //preferences.remove("level");
  data = last_data.getInt("data", 100);
  last_data.end();
  backlight_val = data%10;
  level = data/10;

  pinMode(UP_PIN, INPUT_PULLDOWN);
  pinMode(DOWN_PIN, INPUT_PULLDOWN);
  pinMode(BACKLIGHT_PIN, INPUT_PULLDOWN);

  // Initializing OLED display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    display_working = false;
  }

  BLEDevice::init(SERVER_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pService->addCharacteristic(&levelCharacteristic);
  
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  if(display_working){
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Lvl:");
    display.display(); 
  }

  display_on_time = millis();
}

void loop() {
  // put your main code here, to run repeatedly:  
  if(deviceConnected_prev & !deviceConnected){
    deviceConnected_prev = deviceConnected;
    delay(500);
    BLEDevice::startAdvertising();
  }
  
  deviceConnected_prev = deviceConnected;

  up_curr_val = digitalRead(UP_PIN);
  down_curr_val = digitalRead(DOWN_PIN);
  back_curr_val = digitalRead(BACKLIGHT_PIN);

  // Toggles the backlight on the client's LCD when pressed
  if((back_curr_val == 1) & (back_prev_val == 0)){
    if(backlight_val == 0)
      backlight_val = 1;
    else
      backlight_val = 0;
    
    display_on_time = millis();
    button_pressed = true;
  }

  // Checking if the buttons transition from low to high
  if((up_curr_val == 1) & (up_prev_val == 0) & (level < 40)){
    level++;

    display_on_time = millis();
    button_pressed = true;
  }
  else if((down_curr_val == 1) & (down_prev_val == 0) & (level > 0)){
    level--;

    display_on_time = millis();
    button_pressed = true;
  }
  // Setting the previous values to the current values for the next check
  up_prev_val = up_curr_val;
  down_prev_val = down_curr_val;
  back_prev_val = back_curr_val;

  #if PRINT
    Serial.print(up_curr_val); Serial.print('\t');
    Serial.print(down_curr_val); Serial.print('\t');
    Serial.println(level);
  #endif

  if(deviceConnected){
    data = level*10 + backlight_val;
    
    levelCharacteristic.setValue(data);
  }
  
  // Update the display whenever applicable 
  // The temperature setting is a placeholder for now
  if(display_working){
    if((display_on_time+15000 > millis()) & button_pressed){
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Lvl:");
      display.setCursor(50, 0); 
      display.println(level);
      display.setCursor(0, 16);
      display.println("Tmp:");
      display.setCursor(50, 16);
      display.println("65");  // temp
      display.display();
      display_off = false;
    }
    else if(display_on_time+15000 < millis() & !display_off){
      display.clearDisplay();
      display.display();
      display_off = true;
    }
  }
  // Update the data stored in flash memory whenever a button is pressed
  if(button_pressed){
    last_data.begin("my_app", false);
    last_data.putInt("data", data);
    last_data.end();

    button_pressed = false;
  }
 
  delay(100);
}
