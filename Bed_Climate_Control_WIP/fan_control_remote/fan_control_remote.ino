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

#define PRINT           1

// SDA is 21, SCL is 22
#define SERVER_NAME     "Big Fan"
#define UP_PIN          15
#define DOWN_PIN        4
#define BACKLIGHT_PIN   12

// Setting up deep sleep
#define PIN_BITMASK     0x9010   // pin 4, 12, and 15 right now
#define TIME_TILL_SLEEP 15000

unsigned long int button_time;

// time the screen stays on after button is pressed in milliseconds
#define SCREEN_TIME     10000

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

void increment_from_wakeup(){
  // I found this in a Random Nerd Tutorial about deep sleep
  int GPIO_reason = esp_sleep_get_ext1_wakeup_status();
  int pin = log(GPIO_reason)/log(2);

  #if PRINT
    Serial.printf("%d\n", pin);
  #endif

  if(pin == UP_PIN){
    level++;
  }
  else if(pin == DOWN_PIN){
    level--;
  }
  else if(pin == BACKLIGHT_PIN){
    if(backlight_val == 0)
      backlight_val = 1;
    else
      backlight_val = 0;
  }
}

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

  pinMode(UP_PIN, INPUT);
  pinMode(DOWN_PIN, INPUT);
  pinMode(BACKLIGHT_PIN, INPUT);

  // Setting up pins to wake from deep sleep
  esp_sleep_enable_ext1_wakeup(PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);

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

  increment_from_wakeup();

  button_time = millis();
}

void loop() {
  // If the client disconnects, start advertising again
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
    
    button_pressed = true;
  }

  // Checking if the buttons transition from low to high
  if((up_curr_val == 1) & (up_prev_val == 0) & (level < 40)){
    level++;

    button_pressed = true;
  }
  else if((down_curr_val == 1) & (down_prev_val == 0) & (level > 0)){
    level--;

    button_pressed = true;
  }
  // Setting the previous values to the current values for the next check
  up_prev_val = up_curr_val;
  down_prev_val = down_curr_val;
  back_prev_val = back_curr_val;

  data = level*10 + backlight_val;

  #if PRINT
    Serial.printf("%d\t%d\t%d\t%d\n", up_curr_val, down_curr_val, level, display_working);
  #endif

  // Update the data stored in flash memory whenever a button is pressed
  if(button_pressed){
    last_data.begin("my_app", false);
    last_data.putInt("data", data);
    last_data.end();

    button_time = millis();
  }
  
  // Update the display whenever applicable 
  // The temperature setting is a placeholder for now
  if(display_working){
    if((button_time+SCREEN_TIME > millis()) & button_pressed){
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
    else if(button_time+SCREEN_TIME < millis() & !display_off){
      display.clearDisplay();
      display.display();
      display_off = true;
    }
  }

  // Put the ESP32 into deep sleep 15 seconds after pressing a button
  if(button_time+TIME_TILL_SLEEP < millis()){
    if(display_working & !display_off){
      display.clearDisplay();
      display.display();
    }
    esp_deep_sleep_start();
  }

  if(deviceConnected & button_pressed){
    levelCharacteristic.setValue(data);
  }

  button_pressed = false;

  delay(10);
}
