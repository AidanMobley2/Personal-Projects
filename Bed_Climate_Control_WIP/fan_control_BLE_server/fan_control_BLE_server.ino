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
#define DOWN_PIN        4  // was 27
#define BACKLIGHT_PIN   23

Preferences last_level;

bool deviceConnected = false;
bool deviceConnected_prev = false;

byte up_curr_val, down_curr_val, back_curr_val;
byte up_prev_val = 0;
byte down_prev_val = 0;
byte back_prev_val = 0;
byte level = 10;
byte backlight_val = 0;
int data;

// Setting up variables related to OLED display
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   32
#define SCREEN_ADDRESS  0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool display_working = true;
bool update_display = true;
bool display_off = false;
unsigned long int display_on_time;

//bool first_loop = true;

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

  last_level.begin("my_app", false);  // start partition with namespace "my_app" in read/write mode
  // Remove all preferences under the opened namespace
  //preferences.clear();
  // Or remove the level key only
  //preferences.remove("level");
  level = last_level.getInt("level", 10);
  last_level.end();

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

  display.clearDisplay();
  display.setTextSize(2);           // creating the splash screen
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Lvl:");
  // display.setCursor(38, 22);
  // display.println("Slap");
  // display.setTextSize(1);
  // display.setCursor(14, 45);
  // display.println("By: Aidan Mobley");
  // display.setCursor(20, 55);
  // display.println("Shake to start");
  display.display(); 

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
    update_display = true;
  }

  // Checking if the buttons transition from low to high
  if((up_curr_val == 1) & (up_prev_val == 0) & (level < 40)){
    level++;
    last_level.begin("my_app", false);  // start partition with namespace "my_app" in read/write mode
    last_level.putInt("level", level);
    last_level.end();

    display_on_time = millis();
    update_display = true;
  }
  else if((down_curr_val == 1) & (down_prev_val == 0) & (level > 0)){
    level--;
    last_level.begin("my_app", false);  // start partition with namespace "my_app" in read/write mode
    last_level.putInt("level", level);
    last_level.end();

    display_on_time = millis();
    update_display = true;
  }
  // Setting the current values to previous values for the next check
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

  if(display_on_time+15000 < millis() & !display_off){
    update_display = true;
  }

  // Update the display whenever applicable 
  if(display_working & update_display){
    if(display_on_time+15000 > millis()){
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
    else{
      display.clearDisplay();
      display.display();
      display_off = true;
    }
    update_display = false;
  }
 
  delay(100);
}
