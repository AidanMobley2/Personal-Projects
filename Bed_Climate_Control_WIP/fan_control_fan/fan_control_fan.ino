#include <LiquidCrystal_PCF8574.h>
#include <BLEDevice.h>

#define PRINT           0

#define OUT_PIN         23
#define DIM_PIN        27

#define SERVER_NAME   "Big Fan"

static BLEUUID ServiceUUID("00a9cede-ba07-453c-8a6f-f1ddb24e23ac");
static BLEUUID levelCharacteristicUUID("4a116374-2b2c-4ad7-bb00-3b59ca3d2e68");

static bool doConnect = false;
static bool connected = false;
bool was_connected = false;
String connected_d = "Not Connected";

static BLEAddress *pServerAddress;

static BLERemoteCharacteristic* levelCharacteristic;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    #if PRINT
      Serial.println("Disconnected");
    #endif
  }
};

bool connectToServer(BLEAddress pAddress) {
  BLEClient* pClient = BLEDevice::createClient();

  pClient->setClientCallbacks(new MyClientCallback());
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  #if PRINT
    Serial.println(" - Connected to server");
  #endif
  //pClient->setMTU(517);  //set client to request maximum MTU from server (default is 23 otherwise)
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(ServiceUUID);
  if (pRemoteService == nullptr) {
    #if PRINT
      Serial.print("Failed to find our service UUID: ");
    #endif
    pClient->disconnect();
    return false;
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  levelCharacteristic = pRemoteService->getCharacteristic(levelCharacteristicUUID);
  if (levelCharacteristic == nullptr) {
    #if PRINT
      Serial.print("Failed to find our characteristic UUID");
    #endif
    pClient->disconnect();
    return false;
  }
  #if PRINT
    Serial.println(" - Found our characteristics");
  #endif

  if(levelCharacteristic->canRead()){
    #if PRINT
      String value = levelCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    #endif
  }
 
  return true;
}
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(ServiceUUID)) { //Check if the name of the advertiser matches   (advertisedDevice.getName() == SERVER_NAME)
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      #if PRINT
        Serial.println("Device found. Connecting!");
      #endif
    }
  }
};

int level, prev_level = 10;
int out_level;
int out_val;

uint16_t value;
uint16_t prev_value = 0;

byte backlight_val = 0;

LiquidCrystal_PCF8574 lcd(0x27);

//long int start_time;
//int count = 0;
bool first_loop = true;


void setup() {
  // Start serial communication
  #if PRINT
    Serial.begin(9600);
    while(!Serial & millis()<5000){}
  #endif

  // Setting pinModes for all pins used
  pinMode(OUT_PIN, OUTPUT);
  pinMode(DIM_PIN, OUTPUT);

  // Setting fan pwm frequency to 25 kHz because the fan prefers this frequency
  analogWriteFrequency(OUT_PIN, 25000);
  analogWriteFrequency(DIM_PIN, 5000);

  analogWrite(OUT_PIN, 0);
  analogWrite(DIM_PIN, 10);

  // Setting up the LCD
  lcd.begin(16, 2);
  delay(1000);
  //lcd.setBacklight(0);
  if(backlight_val == 1)
    lcd.setBacklight(1);
  else
    lcd.setBacklight(0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.setCursor(0, 1);
  lcd.print(connected_d);

  // Init BLE device
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  //pBLEScan->setInterval(1349);
  //pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  
  if(doConnect == true) {
    if(connectToServer(*pServerAddress)) {
      #if PRINT
        Serial.println("We are now connected to the BLE Server.");
      #endif
      prev_value = levelCharacteristic->readUInt16();
      prev_level = prev_value/10;
      connected = true;
      connected_d = "Connected    ";
      lcd.setCursor(0, 1);
      lcd.print(connected_d);
    } else {
      #if PRINT
        Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
      #endif
    }
    doConnect = false;
  }

  // If the server disconnects, start scanning for a server again
  if(!connected){
    connected_d = "Not Connected";
    lcd.setCursor(0, 1);
    lcd.print(connected_d);
    doConnect = true;
    BLEDevice::getScan()->start(0);
  }
 
  else{
    get_data_read();

    // Only updates if a button was pressed on the remote
    if(value != prev_value | first_loop){
      // Sets the backlight on or off
      lcd.setBacklight(backlight_val);

      #if PRINT
        Serial.println(level);
      #endif

      // Calculates the pwm value to send to the fan
      out_val = level*6.375;
      analogWrite(OUT_PIN, out_val);

      String d_level = "";
      d_level += String(level);
      d_level += " ";

      lcd.setCursor(7, 0);
      lcd.print(d_level);
    }
    prev_value = value;
    first_loop = false;
  }
  //was_connected = connected;

  delay(100);
}
// Gets the data the sever recorded and decodes it into the backlight and fan level values
void get_data_read(){
  value = levelCharacteristic->readUInt16();
  #if PRINT
    Serial.print(value);
    Serial.print('\t');
  #endif

  // Gets the backlight value
  backlight_val = value%10;

  // Isolates the fan speed level
  uint16_t val = value/10;
  if((val < prev_level-1) || (val > prev_level+1))
    level = prev_level;
  else
    level = val;
  
  prev_level = level;
}