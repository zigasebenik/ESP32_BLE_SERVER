
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <ESP32_MailClient.h>


const char* ssid = "tam110";
const char* password = "12345678";

#define emailSenderAccount    "tam110.message@gmail.com"    
#define emailSenderPassword   "tam110verification"
#define emailRecipient        "ziga.sebenik39@gmail.com"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "TAM110-VERIFICATION_CODE"

SMTPData smtpData;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;



#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


class MyBLEServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


class MyBLECharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("WRITE value: ");
        Serial.print(value.c_str());

        Serial.println();
        Serial.println("*********");
      }
    }

    void onRead(BLECharacteristic* pCharacteristic) {
		  std::string msg = pCharacteristic->getValue();
		  if (msg.length() > 0) {
        Serial.println("*********");
        Serial.print("READ value: ");
        Serial.println(msg.c_str());
        Serial.println("*********");
      }
	}
};


class MySecurity: public BLESecurityCallbacks {

	uint32_t onPassKeyRequest(){
        Serial.println("PassKeyRequest");
		return 393939;
	}

	void onPassKeyNotify(uint32_t pass_key){
        Serial.printf("On passkey Notify number:%d\n", pass_key);
	}

	bool onSecurityRequest(){
	    Serial.println("On Security Request");
		return true;
	}

	void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
        
		Serial.println("Starting BLE work!");
		if(cmpl.success){
			uint16_t length;
			esp_ble_gap_get_whitelist_size(&length);
            Serial.printf("size: %d", length);
		}
        else {
            Serial.println("onAuthentication not Complete!");
            }
	}

    bool onConfirmPIN(uint32_t pin){
        Serial.printf("PIN: %d\n", pin);
        return true;
    }
};

void setup() {
    Serial.begin(115200);

    // Create the BLE Device
    std::string deviceName = "TAM-110-50b0bd5c-c67b";
    BLEDevice::init(deviceName);


    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MySecurity());

    // Create the BLE Server
    pServer = BLEDevice::createServer();


    pServer->setCallbacks(new MyBLEServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);
  

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID,
                    BLECharacteristic::PROPERTY_READ   |
                    BLECharacteristic::PROPERTY_WRITE  |
                    BLECharacteristic::PROPERTY_NOTIFY |
                    BLECharacteristic::PROPERTY_INDICATE
                );

    pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    pCharacteristic->setCallbacks(new MyBLECharacteristicCallbacks());


    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();

    BLESecurity *pSecurity = new BLESecurity();  
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_ONLY);
    pSecurity->setCapability(ESP_IO_CAP_OUT);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    //pSecurity->setStaticPIN(393939);

    Serial.println("Server started");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(200);
        counter ++;

        if(counter == 10)
            break;
    }

    Serial.println();
    Serial.println("WiFi connected.");
    Serial.println();
    Serial.println("Preparing to send email");
    Serial.println();


    smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);
    smtpData.setSender("ESP32", emailSenderAccount);
    smtpData.setPriority("High");
    smtpData.setSubject(emailSubject);
    smtpData.setMessage("Hello World! - Sent from ESP32 board", false);
    smtpData.addRecipient(emailRecipient);


    //Start sending Email, can be set callback function to track the status
    if (!MailClient.sendMail(smtpData))
        Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

    smtpData.empty();
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        pCharacteristic->setValue((uint8_t*)&value, sizeof(uint32_t));
        pCharacteristic->notify();
        value++;

        delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
