/*
 * About: Client for Heart Rate Sensor
 *      Reading HRM Characteritic Value from HR sensor
 *      More info about the payload structuring is provided in the HR sensor server code.
 */

#include "BLEDevice.h"

//BLE Heart Rate Sensor Name
#define bleServerName "kakarot"

/* UUID's of the service, characteristic that we want to read*/
#define HR_SERVICE_UUID            BLEUUID((uint16_t)0x180D)
#define HRM_CHARACTERISTIC_UUID    BLEUUID((uint16_t)0x2A37)

//connection flags
static boolean deviceFound = false;

//device object stored in this after scanner founds it
static BLEAdvertisedDevice* foundDevice;
 
//declare characteristic that we want to read
static BLERemoteCharacteristic* hrmCharacteristic;

//Activate/Deactivate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

//Variables to store Health Rate Measureent Value
char* HRMV; //raw data from HRM
uint8_t flags,heart_rate;
bool hr_format_16bit,sensor_contact_support, sensor_contact_status,energy_exapended_status,rr_interval;

//global counter
uint16_t notifyCounter=0;



//Connect to the BLE Server that has the name, Service, and Characteristics
BLEClient* connectToServer(BLEAdvertisedDevice*);

//function to read notificationsfrom HR service
void ReadHRM(BLEAdvertisedDevice*,uint8_t);

//function that parses the flags in HRMV
void parseFlags(uint8_t);


//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
  void onResult(BLEAdvertisedDevice advertisedDevice) 
  {
    if (advertisedDevice.getName() == bleServerName) 
    { 
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      foundDevice = new BLEAdvertisedDevice(advertisedDevice);
      deviceFound = true;
      
      Serial.print("Device found:  ");
      Serial.println(advertisedDevice.toString().c_str());
    }
  }
};

//When the BLE Server sends a new hrm reading with the notify property
static void hrmNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store hrm value
  HRMV = (char*)pData;

  //parse flags
  flags=uint8_t(HRMV[0]);
  parseFlags(flags);

  //parse heart rate
  heart_rate=uint8_t(HRMV[1]);

  //notification counter increment
  notifyCounter++;

  Serial.println("-------------------------------------------------------");
  Serial.println("hr_format_16bit         "+String(hr_format_16bit));
  Serial.println("sensor_contact_status   "+String(sensor_contact_status));
  Serial.println("sensor_contact_support  "+String(sensor_contact_support));
  Serial.println("energy_exapended_status "+String(energy_exapended_status));
  Serial.println("rr_interval             "+String(rr_interval));
  Serial.println("Heart Rate(bpm): "+String(heart_rate));
  Serial.println("-------------------------------------------------------");
}

void setup() {
  
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  //Init BLE device
  BLEDevice::init("");
 
  //get scanner object
  BLEScan* pBLEScan = BLEDevice::getScan();

  //set our delegate or callback
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  //active scan enabled
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);


  //Read notifications from HRM characteristic
  if(deviceFound==true){
    ReadHRM(foundDevice,5); //reading total 5 notifications
  }
}

void loop() {}

void ReadHRM(BLEAdvertisedDevice* device, uint8_t totalReadings){
  //Attempting to connect to the discovered device
  BLEClient* myDevice = connectToServer(device);
  
  if(myDevice!=NULL){
      Serial.print("Connected to the BLE Server: ");
      Serial.println(device->getName().c_str());
      
      //Activate the Notify property of each Characteristic
      hrmCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      Serial.println("Notifications ON");

      //Wait till we recieve total notifications
      while(notifyCounter<totalReadings)Serial.print("");
      
      //Deactivate Notify service
      hrmCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOff, 2, false);
      Serial.println("Notifications OFF");

      //disconnect the device
      myDevice->disconnect();
      Serial.println("Device disconnected.");
    }
}


BLEClient* connectToServer(BLEAdvertisedDevice* device) {
  BLEClient* pClient = BLEDevice::createClient();

  if (pClient->connect(device)){ // Connect to the remote BLE Server.
      Serial.println(" - Connected to server");
      // Obtain a reference to the service we are after in the remote BLE server.
      BLERemoteService* pRemoteService = pClient->getService(HR_SERVICE_UUID);
      if (pRemoteService == nullptr) {
        Serial.print("Failed to find given service UUID: ");
        Serial.println(HR_SERVICE_UUID.toString().c_str());
        pClient->disconnect();
        return NULL;
      }
      Serial.println(" - Service Found!");
     
      // Obtain a reference to the characteristics in the service of the remote BLE server.
      hrmCharacteristic = pRemoteService->getCharacteristic(HRM_CHARACTERISTIC_UUID);
    
      if (hrmCharacteristic == nullptr) {
        Serial.print("Failed to find given characteristic UUID");
        pClient->disconnect();
        return NULL;
      }
      Serial.println(" - Characteristic Found!");
     
      //Assign callback functions for the Characteristics
      hrmCharacteristic->registerForNotify(hrmNotifyCallback);
      
      return pClient;
  }
  else{
    Serial.println("Failed to Connect to device");
    return NULL;
  }
}

void parseFlags(uint8_t flags){
  hr_format_16bit = flags & 1;
  sensor_contact_status = flags & 1<<1;
  sensor_contact_support = flags & 1<<2;
  energy_exapended_status = flags & 1<<3;
  rr_interval = flags & 1<<4;
}
