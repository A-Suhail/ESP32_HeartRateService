
/*
 * Firmware:- Heart Rate Sensor
 * Services Provided:-
 *    1. Heart Rate Service
 *       -  Heart Rate Measurement
 *          A. Characteristic Behaviour
 *            -> Flags : 8 BITS
 *              1. BIT 0 : Heart Rate Value Format bit
 *                - indicates data format of HRMV is unint8 or unint16/format is unint16
 *              2. BIT 1,2 : Sensor Contact Status bits
 *                - If the Sensor Contact feature is supported and if the device detects 
 *                       no or poor contact with the skin, the Sensor Contact Status bit (bit 
 *                       1 of the Flags field) shall be set to 0.Otherwise it shall be set to 1.
 *                - indicate whether or not, the Sensor Contact feature is supported (bit 2)
 *              3. BIT 3 : Energy Expended Status bit
 *                - indicates whether or not, the Energy Expended field is present in the 
 *                        Heart Rate Measurement characteristic.
 *              4. BIT 4 : RR-Interval Bit
 *                - The RR-Interval bit indicates whether or not RR-Interval values are present 
 *                  in the Heart Rate Measurement characteristic.
 *              5. BIT 5-8: RESERVED
 *              
 *            -> Heart Rate Measurement Value Field (HRMV)
 *              1. 8 BITS : Heart Rate Value
 *                - If the Heart Rate Measurement Value is less than or equal to 255 bpm a UINT8 
 *                  format should be used for power savings.
 *              2. 16 BITS : Heart Rate Value
 *                - If the Heart Rate Measurement Value exceeds 255 bpm a UINT16 format shall be used.
 *              3. 16 BITS : Energy Expended Field
 *                - The Energy Expended field represents the accumulated energy expended in kilo Joules 
 *                  since the last time it was reset.
 *                - Energy Expended is a UINT16.
 *          B. Characteristic Descriptors
 *            - The Client Characteristic Configuration descriptor shall be included in the 
 *              Heart Rate Measurement characteristic. (CCCD)
 *              
 *       -  Body Sensor Location
 *          - The Body Sensor Location characteristic of the device is used to describe the intended 
 *            location of the heart rate measurement for the device.
 *            
 *       -  Heart Rate Control Point : The Heart Rate Control Point characteristic is used to enable 
 *                                     a Client to write control points to a Server to control behavior.
 *          A. Characteristic Behaviour
 *            - The Heart Rate Control Point characteristic sets a control point value when written.
 *            
 *    2. Device Information Service
 *    
 */

 //ONLY HEART RATE MEASUREMENT HAS BEEN IMPLEMENTED : FLAGS + HR VALUE

#include <BLEDevice.h>

//#define SERVICE_UUID        "0000180D-0000-1000-8000-00805F9B34FB"
//#define CHARACTERISTIC_UUID "00002A37-0000-1000-8000-00805F9B34FB"

//Standard UUID declaration for HR service and HRM characteristic
#define HR_SERVICE_UUID            BLEUUID((uint16_t)0x180D)
#define HRM_CHARACTERISTIC_UUID    BLEUUID((uint16_t)0x2A37)

//device connection flag
bool deviceConnected = false;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

//Flags for HRM value
bool bit16_HRV_Format = false;
bool sensor_contact = false;
bool sensor_contact_support = false;
bool energy_expend_support = true;
bool rr_interval_support = false;

//heart rate value variable
uint8_t heart_rate=0;

//final payload 4 bytes only as of now 
uint32_t payload=0;

//this stores value of cccd use to detect notification on/off
uint8_t *cccd;

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Client Connected");
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    Serial.println("Client Disconnected");
    deviceConnected = false;
  }
};

// Heart Rate Measurement Characteristic
BLECharacteristic HrmCharacteristic(
                                    HRM_CHARACTERISTIC_UUID,
                                    BLECharacteristic::PROPERTY_READ|
                                    BLECharacteristic::PROPERTY_WRITE|
                                    BLECharacteristic::PROPERTY_NOTIFY
                                    );
                                    
//Client Characteristic Configuration Descriptor descriptor for HRM
BLEDescriptor CCCD(BLEUUID((uint16_t)0x2902));

//Initialise our payload with flags
void PayloadInit(){
  payload=0;
  payload |= bit16_HRV_Format;
  payload |= sensor_contact<<1;
  payload |= sensor_contact_support<<2;
  payload |= energy_expend_support<<3;
  payload |= rr_interval_support<<4;
}

void setup() 
{
  Serial.begin(115200);
  
  //initialise BLE environment
  BLEDevice::init("Heart Rate Sensor");

  //create server and assign callback for connect and disconnect
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  //create service and add HRM characteristic to HR service
  BLEService* HrService = pServer->createService(HR_SERVICE_UUID);
  HrService->addCharacteristic(&HrmCharacteristic);


  //Set Initial value of CCCD to 0,0
  uint8_t v[] = {0x00, 0x00};
  uint8_t *pv = v;
  CCCD.setValue(pv,2);

  //add descriptor to HRM characteristic
  HrmCharacteristic.addDescriptor(&CCCD);

  //start the service
  HrService->start();

  //start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(HR_SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting for Client...");
}

void loop()
{
  if (deviceConnected) {
      if ((millis() - lastTime) > timerDelay) {

        //get the current value of cccd to check status of notification
        cccd = CCCD.getValue();
        if(*cccd==0)Serial.println("Notifications are OFF");
        else        Serial.println("Notifications are ON");

        //randomly creating heart rate
        heart_rate=random(85,100);
  
        //Reset and initialise payload with flags and add heart rate
        PayloadInit();
        payload |= heart_rate<<8;
        Serial.println("Heart Rate(bpm): "+String(heart_rate));

        //set value of HRM characteristic to the payload and send notification
        HrmCharacteristic.setValue(payload);
        HrmCharacteristic.notify();
  
        lastTime = millis();
      }
    }
}
