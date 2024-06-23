/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/ttgo-lora32-sx1276-arduino-ide/
*********/

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <HttpsOTAUpdate.h>

#include <ArduinoJson.h>
#include <EEPROM.h>

#include <WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <WiFiClientSecure.h>
#include <HTTPClient.h>

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 866E6

// //OLED pins
// #define OLED_SDA 4
// #define OLED_SCL 15 
// #define OLED_RST 16
// #define SCREEN_WIDTH 128 // OLED display width, in pixels
// #define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define VALVE 23
#define SOIL 0
#define DATA 34

#define START 1
#define WAITING 2
#define UPDATING 3
#define ACQUIRE 4
#define WATERING 5
#define WATERED_WAITING 6

class Data {
  int maxWatering = -1;
  int minWatering = -1;
  int minSensorReading = -1;
  int maxSensorReading = -1;

  public:
  Data() {}
  
  int getMaxWatering() {
    return maxWatering;
  }

  int getMinWatering() {
    return minWatering;
  }

  int getMinSensorReading() {
    return minSensorReading;
  }

  int getMaxSensorReading() {
    return maxSensorReading;
  }

  void setMaxWatering(int maxWatering) {
    this->maxWatering = maxWatering;
  }

  void setMinWatering(int minWatering) {
    this->minWatering = minWatering;
  }

  void setMinSensorReading(int minSensorReading) {
    this->minSensorReading = minSensorReading;
  }

  void setMaxSensorReading(int maxSensorReading) {
    this->maxSensorReading = maxSensorReading;
  }
};

class IntervalWait {
private:
  unsigned long interval;
  unsigned long lastExecution;
public:
  IntervalWait(unsigned long interval) {
    this->interval = interval;
    this->lastExecution = lastExecution = 0;
  }

  bool shouldWait() {

    unsigned long time = millis();
    if (time - lastExecution > interval) {
      lastExecution = time;
      return false;
    }
    return true;
  }
};

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
char *webUsername;
char *webPassword;
char *id;
char *ssid;
char *password;
char *certifacate;
char *hash;
int hashStart;
char newHash[45];

String serverName = "https://192.168.0.254:8443";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600,  24*3600*1000);

WiFiClientSecure client;
static HttpsOTAStatus_t otastatus;

Data data;

StaticJsonDocument<1000> doc;

void HttpEvent(HttpEvent_t *event) {
  switch (event->event_id) {
    case HTTP_EVENT_ERROR: Serial.println("Http Event Error"); break;
    case HTTP_EVENT_ON_CONNECTED: Serial.println("Http Event On Connected"); break;
    case HTTP_EVENT_HEADER_SENT: Serial.println("Http Event Header Sent"); break;
    case HTTP_EVENT_ON_HEADER: Serial.printf("Http Event On Header, key=%s, value=%s\n", event->header_key, event->header_value); break;
    case HTTP_EVENT_ON_DATA: break;
    case HTTP_EVENT_ON_FINISH: Serial.println("Http Event On Finish"); break;
    case HTTP_EVENT_DISCONNECTED: Serial.println("Http Event Disconnected"); break;
  }
}


void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  readEEPROM();
  
  //Connect to Wifi
  connectToWifi();

  //Start ntp server connection
  timeClient.begin();
  timeClient.update();

  // Setup secure wifi client
  client.setCACert(certifacate);

  // Setup OTA
  HttpsOTA.onHttpEvent(HttpEvent);

  // //reset OLED display via software
  // pinMode(OLED_RST, OUTPUT);
  // digitalWrite(OLED_RST, LOW);
  // delay(20);
  // digitalWrite(OLED_RST, HIGH);
  // Serial.println("XDD");
  // initialize OLED
  // Wire.begin(OLED_SDA, OLED_SCL);
  // if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
  //   Serial.println(F("SSD1306 allocation failed"));
  //   for(;;); // Don't proceed, loop forever
  // }
  pinMode(SOIL, OUTPUT);
  pinMode(DATA, ANALOG);
}

void readEEPROM() {

  EEPROM.begin(4096);
  int variables = EEPROM.readInt(0);
  Serial.println(variables);
  int startingAddress = variables * 4 + 4;
  int webUsernameLength = EEPROM.readInt(4);
  int webPasswordLength = EEPROM.readInt(8);
  int idLength = EEPROM.readInt(12);
  int ssidLength = EEPROM.readInt(16);
  int passwordLength = EEPROM.readInt(20);
  int certificateLength = EEPROM.readInt(24);
  int hashLength = EEPROM.readInt(28);
  Serial.println(webUsernameLength);
  Serial.println(webPasswordLength);
  Serial.println(idLength);
  Serial.println(ssidLength);
  Serial.println(passwordLength);
  Serial.println(certificateLength);
  Serial.println(hashLength);
  webUsername = (char *) malloc(webUsernameLength + 1);
  webPassword = (char *) malloc(webPasswordLength + 1);
  id = (char *) malloc(idLength + 1);
  ssid = (char *) malloc(ssidLength + 1);
  password = (char *) malloc(passwordLength + 1);
  certifacate = (char *) malloc(certificateLength + 1);
  hash = (char *) malloc(hashLength + 1);

  readString(startingAddress, webUsername, webUsernameLength);
  readString(startingAddress + webUsernameLength, webPassword, webPasswordLength);
  readString(startingAddress + webUsernameLength + webPasswordLength, id, idLength);
  readString(startingAddress + webUsernameLength + webPasswordLength + idLength, ssid, ssidLength);
  readString(startingAddress + webUsernameLength + webPasswordLength + idLength + ssidLength, password, passwordLength);
  readString(startingAddress + webUsernameLength + webPasswordLength + idLength + ssidLength + passwordLength, certifacate, certificateLength);
  hashStart = startingAddress + webUsernameLength + webPasswordLength + idLength + ssidLength + passwordLength + certificateLength;
  readString(startingAddress + webUsernameLength + webPasswordLength + idLength + ssidLength + passwordLength + certificateLength, hash, hashLength);

  Serial.println(webUsername);
  Serial.println(webPassword);
  Serial.println(id);
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(certifacate);
  Serial.println(hash);
  EEPROM.end();
}

void readString(int startingAddress, char *output, int length) {
  for (int i = startingAddress; i < startingAddress + length; i++ ){
    output[i - startingAddress] = (char) EEPROM.read(i);
  }
  output[length] = '\0';
}

void connectToWifi() {
  //Connect to Wifi
  Serial.print("Attempting to connect to SSID: ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
}

int STATE = START;

void loop() {

  switch (STATE) {
    case START:
      checkDataUpdate();
      STATE = WAITING;
      break;
    case WAITING:
      doCheckUpdate();
      doWaiting();
      break;
    case UPDATING:
      doUpdate();
      break;
    case ACQUIRE:
      Serial.println("Looping...");
      delay(10000);
      //doAcquire();
      break;
    case WATERING:
      //doWatering();
    default:
      break;
  }
}

#define UPDATE_INTERVAL 2 * 60 * 1000

IntervalWait checkUpdate(UPDATE_INTERVAL);

void doCheckUpdate() {

  if(checkUpdate.shouldWait()) {
    return;
  }

  checkCodeUpdate();
  checkDataUpdate();
}

void checkCodeUpdate() {
  
  getLatestUpdateHash(newHash);

  if (strcmp(hash, newHash) != 0) {
    STATE = UPDATING;
    String url = String("https://");
    url += webUsername;
    url += ":";
    url += webPassword;
    url += "@192.168.0.254:8443/update/latest";
    Serial.println(url.c_str());
    HttpsOTA.begin(url.c_str(), certifacate);
  }
}

void getLatestUpdateHash(char *hashBuffer) {

  WiFiClientSecure client;
  client.setCACert(certifacate);
  HTTPClient https;
  
  https.begin(client, "https://192.168.0.254:8443/update/latest/hash");
  https.setAuthorization("arduino_user", "arduino_user");

  Serial.println("xd");

  int httpCode = https.GET();
  Serial.println(httpCode);
  Serial.println(https.errorToString(httpCode));

  if (httpCode >= 200 && httpCode < 300) {
    String payload = https.getString();
    payload.toCharArray(hashBuffer, 45);
    Serial.println(hashBuffer);
  }
  https.end();
}


void checkDataUpdate() {
  HTTPClient https;

  String serverPath = serverName + "/api/plants/data/" + id;
  https.begin(client, serverPath.c_str());
  https.setAuthorization("arduino_user", "arduino_user");
  int httpResponseCode = https.GET();
  Serial.println(httpResponseCode);
  if (httpResponseCode >= 200 && httpResponseCode < 300) {
    String payload = https.getString();

    Serial.println(payload);

    DeserializationError error = deserializeJson(doc, payload.c_str());

    if (error) {
      Serial.println("Deserilization failed :(");
      Serial.println(error.f_str());
    } 

    data.setMinWatering(doc["minWatering"]);
    data.setMaxWatering(doc["maxWatering"]);
    data.setMinSensorReading(doc["minSensorReading"]);
    data.setMaxSensorReading(doc["maxSensorReading"]);

    Serial.print(data.getMinWatering());
    Serial.print(" ");
    Serial.print(data.getMaxWatering());
    Serial.print(" ");
    Serial.print(data.getMinSensorReading());
    Serial.print(" ");
    Serial.print(data.getMaxSensorReading());
    Serial.println();
  }
  https.end();
}


#define WAITING_INTERVAL 5000

IntervalWait waiting(WAITING_INTERVAL);

void doWaiting() {

  if (waiting.shouldWait()) {
    return;
  }

  int moisture = getMoisture();
  if (data.getMinWatering() != -1  && moisture > data.getMaxWatering()) {
    STATE = ACQUIRE;
  }
}

void doUpdate() {

  otastatus = HttpsOTA.status();
  if (otastatus == HTTPS_OTA_SUCCESS) {
    Serial.println("Firmware written successfully. To reboot device, call API ESP.restart() or PUSH restart button on device");
    strcpy(hash, newHash);
    persistNewHash();
    ESP.restart();
  } else if (otastatus == HTTPS_OTA_FAIL) {
    Serial.println("Firmware Upgrade Fail");
    STATE = WAITING;
  }
}

void persistNewHash() {
  EEPROM.begin(44);
  writeString(hashStart, newHash, strlen(newHash));
  EEPROM.commit();
}

void writeString(int startingAddress, const char *input, int length) {
  for (int i = startingAddress; i < startingAddress + length; i++ ){
    Serial.print(input[i - startingAddress]);
    EEPROM.write(i, input[i - startingAddress]);
  }
  Serial.println();
}


#define ACQUIRED_WAITING 1000

IntervalWait acquire(ACQUIRED_WAITING);

void doAcquire() {

  if(acquire.shouldWait()) {
    return;
  }

  bool result = getPump();
  if (result) {
    STATE = WATERING;
    pinMode(VALVE, OUTPUT);
  }
}


void doWatering() {

  int moisture = getMoisture();
  if (moisture < data.getMinWatering()) {
    releasePump();
    pinMode(VALVE, INPUT);
    STATE = WAITING;
  }
}



bool getPump() {
  if(WiFi.status()== WL_CONNECTED){ 
    HTTPClient https;

    String serverPath = serverName + "/acquire?id=" + id;

    https.begin(serverPath.c_str());

    int httpResponseCode = https.GET();

    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      String payload = https.getString();

      Serial.println(payload);

      StaticJsonDocument<200> doc;

      DeserializationError error = deserializeJson(doc, payload.c_str());

      if (error) {
        Serial.println("Deserilization failed :(");
        Serial.println(error.f_str());
        return false;
      } 

      return doc["available"] == "yes";
    }
  }

  return false;
}

void releasePump() {
  if(WiFi.status()== WL_CONNECTED){ 
    HTTPClient http;

    String serverPath = serverName + "/release?id=" + id;

    http.begin(serverPath.c_str());

    int httpResponseCode = http.GET();

    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      String payload = http.getString();

      Serial.println(payload);
    }
  }
}

// Data getData() {
//   if(WiFi.status()== WL_CONNECTED){ 
//     HTTPClient http;

//     String serverPath = serverName + "/data/ b " + id;
//     http.begin(serverPath.c_str());
//     int httpResponseCode = http.GET();
//     if (httpResponseCode >= 200 && httpResponseCode < 300) {
//       String payload = http.getString();

//       Serial.println(payload);

//       StaticJsonDocument<200> doc;

//       DeserializationError error = deserializeJson(doc, payload.c_str());

//       if (error) {
//         Serial.println("Deserilization failed :(");
//         Serial.println(error.f_str());
//         return Data(-1,-1);
//       } 
      
//       Data data(doc["min"], doc["max"]);

//       Serial.print(data.getMin());
//       Serial.print(" ");
//       Serial.println(data.getMax());

//       return data;
//     }
//   }
// }


// void displayMoisture(int moisture) {
//   display.clearDisplay();
//   display.setCursor(0,0);
//   display.println("MOISTURE SENSOR");
//   display.setCursor(0,20);
//   display.setTextSize(1);
//   display.print("Current moisture:");
//   display.setCursor(0,30);
//   display.print("Moisture:");
//   display.setCursor(50,30); 
//   display.display();
// }

int getMoisture() {
  digitalWrite(SOIL, HIGH);
  delay(10);
  int arr[11];// = { 200, 420, 421, 424, 425, 426, 423, 430, 423, 421, 450 };
  for(int i = 0; i < 11; i++) {
    arr[i] = analogRead(DATA);
  }

  digitalWrite(SOIL, LOW);

  findOutliersWithIQR(arr);

  for (int i = 0; i < 11; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(arr[i]);
  }

  return customAverage(arr);
}

void findOutliersWithIQR(int *arr) {

  qsort(arr, 11, sizeof(arr[0]), sort_asc);

  int median = arr[5];
  int q1 = arr[2];
  int q3 = arr[8];
  int iqr = q3 - q1;
  int upper = q3 + (1.5 * iqr);
  int lower = q1 - (1.5 * iqr);

  for (int i = 0; i < 11; i++) {
    if (arr[i] < lower || arr[i] > upper) {
      arr[i] = -1;
      Serial.print(i);
      Serial.print(": ");
      Serial.println(arr[i]);
    }
  }
}

int sort_asc(const void *cmp1, const void *cmp2) {
  // Need to cast the void * to int *
  int a = *((int *)cmp1);
  int b = *((int *)cmp2);
  // The comparison
  return a < b ? -1 : (a > b ? 1 : 0);
  // A simpler, probably faster way:
  //return b - a;
}

int customAverage(int *arr) {
  int sum = 0;
  int count = 0;
  for (int i = 0; i < 11; i++) {
    if (arr[i] != -1) {
      sum += arr[i];
      ++count;
    }
  }
  return sum / count;
}