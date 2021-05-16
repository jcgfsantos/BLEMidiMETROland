
/** Example without display

Connectes to the Roland piano and detects volume level, metronome status and beat

https://github.com/jcgfsantos/BLEMidiMETROland
jcgfsantos

*/

#include <NimBLEDevice.h>

//#define CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU 255

#define SERVICE_UUID        "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
#define CHARACTERISTIC_UUID "7772e5db-3868-4112-a1a9-f2669d106bf3"

void scanEndedCB(NimBLEScanResults results);

static NimBLEAdvertisedDevice* advDevice;
static NimBLEClient* pClient;
static NimBLERemoteService* pSvc;
static NimBLERemoteCharacteristic* pChr;
static NimBLERemoteDescriptor* pDsc;

static uint8_t header;
static uint8_t tmestamp;

static uint8_t midibuff[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static long ibuff=0;
static boolean isbuffer=false;

static int imsg=1;

static unsigned long delt = 2000;

static bool doConnect = false;
static uint32_t scanTime = 0; /** 0 = scan forever */

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
      Serial.println("Connected");
      /** After connection we should change the parameters if we don't need fast response times.
          These settings are 150ms interval, 0 latency, 450ms timout.
          Timeout should be a multiple of the interval, minimum is 100ms.
          I find a multiple of 3-5 * the interval works best for quick response/reconnect.
          Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
      */
      pClient->updateConnParams(120, 120, 0, 60);
    };

    void onDisconnect(NimBLEClient* pClient) {
      Serial.print(pClient->getPeerAddress().toString().c_str());
      Serial.println(" Disconnected - Starting scan");
      NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
    };

    /** Called when the peripheral requests a change to the connection parameters.
        Return true to accept and apply them or false to reject and keep
        the currently used parameters. Default will return true.
    */
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
      if (params->itvl_min < 24) { /** 1.25ms units */
        return false;
      } else if (params->itvl_max > 40) { /** 1.25ms units */
        return false;
      } else if (params->latency > 2) { /** Number of intervals allowed to skip */
        return false;
      } else if (params->supervision_timeout > 100) { /** 10ms units */
        return false;
      }

      return true;
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest() {
      Serial.println("Client Passkey Request");
      /** return the passkey to send to the server */
      return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key) {
      Serial.print("The passkey YES/NO number: ");
      Serial.println(pass_key);
      /** Return false if passkeys don't match. */
      return true;
    };

    /** Pairing process complete, we can check the results in ble_gap_conn_desc */
    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      if (!desc->sec_state.encrypted) {
        Serial.println("Encrypt connection failed - disconnecting");
        /** Find the client with the connection handle provided in desc */
        NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
        return;
      }
    };
};


/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      Serial.print("Advertised Device found: ");
      Serial.println(advertisedDevice->toString().c_str());
      if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID)))
      {
        Serial.println("Found Our Service");
        /** stop scan before connecting */
        NimBLEDevice::getScan()->stop();
        /** Save the device reference in a global for the client to use*/
        advDevice = advertisedDevice;
        /** Ready to connect now */
        doConnect = true;
      }
    };
};

void sysexid(){
  String addr="";
  addr=(midibuff[10] < 16 ? "0" : "");
  addr=addr+String(midibuff[10],HEX);
  addr=addr+(midibuff[11] < 16 ? "0" : "");
  addr=addr+String(midibuff[11],HEX);
  addr=addr+(midibuff[12] < 16 ? "0" : "");
  addr=addr+String(midibuff[12],HEX);
  addr=addr+(midibuff[13] < 16 ? "0" : "");
  addr=addr+String(midibuff[13],HEX);
  addr.toUpperCase();

  if (addr.equals("0100010F")){
    // metronome status
    Serial.print("Metronome is ");
    if (midibuff[14]==1){
      Serial.println("on");  
    } else {
      Serial.println("off");  
    }
  } else if (addr.equals("01000108")){
    // metronome sequencerTempoRO
    Serial.print("Metronome beat is: ");
    Serial.println(midibuff[14]*128+midibuff[15]);
  } else if (addr.equals("01000213")){
    // masterVolume
    Serial.print("Mater volume is: ");
    Serial.println(midibuff[14]);
  }
}

void buffntfy(String ntfy){
  String tmp;
  ntfy.toUpperCase();
//  Serial.print("BUFFER: ");
//  Serial.println(ntfy);
  for (int i = 0; i < ntfy.length(); i=i+2){
    tmp=ntfy.substring(i, i+2);
    midibuff[ibuff]=(int)strtol(tmp.c_str(), NULL, 16);
//    Serial.print("String: ");
//    Serial.print(tmp);
//    Serial.print(" gives atoi: ");
//    Serial.print(midibuff[ibuff] < 16 ? "0" : "");
//    Serial.println(midibuff[ibuff]);
    ibuff++;
    if (i<2 && ibuff>1) {
      ibuff--;
    }
    if (tmp.equals("F0")) {
      isbuffer=true;
    } else if (tmp.equals("F7")) {
      isbuffer=false;
      sysexid();
    }
  }

  if (!isbuffer){
    Serial.print("Notification value: ");
    for (int i = 0; i < ibuff; i++){
      Serial.print(midibuff[i] < 16 ? "0" : "");
      Serial.print(midibuff[i],HEX);
      midibuff[i]=0x00;
    }  
    Serial.println("");
    ibuff=0;
  }  
    
}

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  std::string str = (isNotify == true) ? "Notification" : "Indication";
  str += " from ";
  /** NimBLEAddress and NimBLEUUID have std::string operators */
  str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
  str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
  str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
  //str += ", Value = " + std::string((char*)pData, length);
  //Serial.println(str.c_str());
  str += ", Value = ";
  Serial.print(str.c_str());

  uint8_t* hexpl;
  char* rply = NimBLEUtils::buildHexData(hexpl, pData, length);
  //Serial.print("Value notify: ");
  Serial.println(String(rply));

  buffntfy(rply);
}

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results) {
  Serial.println("Scan Ended");
}


/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;


/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer() {
  //NimBLEClient* pClient = nullptr;

  pClient = NimBLEDevice::createClient(NimBLEAddress("f9:ab:e0:26:6f:79", 1));
  NimBLEDevice::setMTU(128);
  Serial.println("Checking connection...");
  if (!pClient->isConnected()) {
    bool isconnected=false;
    while(!isconnected){
      pClient->connect();
      delay(100);
      if (pClient->isConnected()) {
        isconnected=true;
      } else {
        Serial.println("Trying to connect... waiting 5 seconds...");
        delay(5000);
      }
    }    
    Serial.println("Connected");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());

    pSvc = pClient->getService(SERVICE_UUID);
    if (pSvc) {    /** make sure it's not null */
      pChr = pSvc->getCharacteristic(CHARACTERISTIC_UUID);
    }

    return true;
  } else {
    Serial.println("Already connected");
    return true;
  }
}

void regsubscribe() {

  if (pChr->canNotify()) {
    if (!pChr->subscribe(true, notifyCB)) {
      /** Disconnect if subscribe failed */
      Serial.println("SUBSCRIPTION FAILED. DISCONNECTING...");
      pClient->disconnect();
    }
  }
}

void sendmidiMessage(const uint8_t* data, size_t length) {
  /** Now we can read/write/subscribe the charateristics of the services we are interested in */

  Serial.print("Length to write is: ");
  Serial.println(length);

  Serial.print("Data to write is: ");
  for (int i = 0; i < length; i++){
    Serial.print(data[i] < 16 ? "0" : "");      
    Serial.print(data[i], HEX);
  }
  Serial.println();

  if (pChr) {    /** make sure it's not null */
    if (pChr->writeValue( data, length)) {
      Serial.print("Wrote new value to: ");
      Serial.println(pChr->getUUID().toString().c_str());
    }
  }

  delay(100);
}

void readmidiMessage(){
  std::string rply = pChr->readValue();  
  Serial.print("Reply Read: ");
  for(int i = 0; i < rply.length(); i++) {
    Serial.print(rply[i] < 16 ? "0" : "");  
    Serial.print(rply[i],HEX);
  }
  Serial.println();
}

void getheadtmps(uint8_t* header,uint8_t* tmestamp){
  auto currentTimeStamp = millis() & 0x01FFF;
  
  *header = ((currentTimeStamp >> 7) & 0x3F) | 0x80;        // 6 bits plus MSB
  *tmestamp = (currentTimeStamp & 0x7F) | 0x80;            // 7 bits plus MSB
}

void setup () {
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");
  /** Initialize NimBLE, no device name spcified as we are not advertising */
  NimBLEDevice::init("");

  //NimBLEDevice::setMTU(128);//128bytes
  //Serial.println("receiving size MTU");
  //Serial.println(NimBLEDevice::getMTU());

  /** Set the IO capabilities of the device, each option will trigger a different pairing method.
      BLE_HS_IO_KEYBOARD_ONLY    - Passkey pairing
      BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
      BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
  */
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

  /** 2 different ways to set security - both calls achieve the same result.
      no bonding, no man in the middle protection, secure connections.

      These are the default values, only shown here for demonstration.
  */
  //NimBLEDevice::setSecurityAuth(false, false, true);
  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */

  if (connectToServer()) {
    Serial.println("Success! we are connected!");
  } else {
    Serial.println("Failed to connect");
  }
  delay(150);

  regsubscribe();

  delay(150);

  readmidiMessage();
  Serial.println("Change app mode");
  getheadtmps(&header,&tmestamp);
  uint8_t midiMessage[]={header, tmestamp,0xF0,0x41,0x10,0x00,0x00,0x00,0x28,0x12,0x01,0x00,0x03,0x00,0x00,0x01,0x7B,0xF7}; // change app mode to 0,1  
  //uint8_t midiMessage[] = { header, tmestamp, 0x90, 0x46, 0x50 }; // Use this for note on
  uint8_t* msg1 = midiMessage;
  sendmidiMessage(msg1, sizeof(midiMessage));
  readmidiMessage();

  Serial.println("Connect command");
  getheadtmps(&header,&tmestamp);
  uint8_t midiMessage2[]={header, tmestamp,0xF0,0x41,0x10,0x00,0x00,0x00,0x28,0x12,0x01,0x00,0x03,0x06,0x01,0x75,0xF7}; // connect command
  //uint8_t midiMessage2[] = { header, tmestamp, 0x80, 0x46, 0x50 }; //Use this for note off
  uint8_t* msg2 = midiMessage2;
  sendmidiMessage(msg2, sizeof(midiMessage2));
  readmidiMessage();

  Serial.println("Get sequencer TempoRO");
  getheadtmps(&header,&tmestamp);
  uint8_t midiMessage3[]={header, tmestamp,0xF0,0x41,0x10,0x00,0x00,0x00,0x28,0x11,0x01,0x00,0x01,0x08,0x00,0x00,0x02,0x74,0xF7}; // get sequencer TempoRO
  uint8_t* msg3 = midiMessage3;
  sendmidiMessage(msg3,sizeof(midiMessage3));
  readmidiMessage();

  delay(1000);

  readmidiMessage();
  Serial.println("Change app mode");
  getheadtmps(&header,&tmestamp);
  uint8_t midiMessage4[]={header, tmestamp,0xF0,0x41,0x10,0x00,0x00,0x00,0x28,0x12,0x01,0x00,0x03,0x00,0x00,0x01,0x7B,0xF7}; // change app mode to 0,1  
  uint8_t* msg4 = midiMessage4;
  sendmidiMessage(msg4, sizeof(midiMessage4));
  readmidiMessage();

  Serial.println("Connect command");
  getheadtmps(&header,&tmestamp);
  uint8_t midiMessage5[]={header, tmestamp,0xF0,0x41,0x10,0x00,0x00,0x00,0x28,0x12,0x01,0x00,0x03,0x06,0x01,0x75,0xF7}; // connect command
  uint8_t* msg5 = midiMessage5;
  sendmidiMessage(msg5, sizeof(midiMessage5));
  readmidiMessage();

  Serial.println("Get sequencer TempoRO");
  getheadtmps(&header,&tmestamp);
  uint8_t midiMessage6[]={header, tmestamp,0xF0,0x41,0x10,0x00,0x00,0x00,0x28,0x11,0x01,0x00,0x01,0x08,0x00,0x02,0x74,0xF7}; // get sequencer TempoRO
  uint8_t* msg6 = midiMessage6;
  sendmidiMessage(msg6,sizeof(midiMessage6));
  readmidiMessage();

}

void loop () {

  delay(30000);
}
