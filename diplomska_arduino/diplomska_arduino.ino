/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/ttgo-lora32-sx1276-arduino-ide/
*********/

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HttpsOTAUpdate.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

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
#define ACQUIRE 3
#define WATERING 4
#define WATERED_WAITING 5

const char *ssid = "Linksys EXT1";
const char *password = "Family@04";

// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

String serverName = "https://192.168.0.254:8443";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char *id = "123";

class Data {
  int max;
  int min;
  public:
  Data(int min, int max) {
    this->min = min;
    this->max = max;
  }
  
  int getMax() {
    return max;
  }

  int getMin() {
    return min;
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


void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  
  //Connect to Wifi
  WiFi.begin(ssid,password);

  //Start ntp server connection

  NTPClient timeClient(ntpUDP, "pool.ntp.org");
  timeClient.begin();
  timeClient.setUpdateInterval(24*3600*1000);
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

int STATE = START;

void loop() {
   
   Serial.println("Hello2");
  // switch (STATE) {
  //   case START:
  //     STATE = WAITING;
  //     break;
  //   case WAITING:
  //     doWaiting();
  //     break;
  //   case ACQUIRE:
  //     doAcquire();
  //     break;
  //   case WATERING:
  //     doWatering();
  //   default:
  //     break;
  // }
}

#define WAITING_INTERVAL 5000

IntervalWait waiting(WAITING_INTERVAL);

void doWaiting() {

  if (waiting.shouldWait()) {
    return;
  }

  Data data = getData();
  int moisture = getMoisture();
  if (data.getMin() != -1  && moisture > data.getMax()) {
    STATE = ACQUIRE;
  }
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
  Data data = getData();
  if (data.getMax() != -1 && moisture < data.getMin()) {
    releasePump();
    pinMode(VALVE, INPUT);
    STATE = WAITING;
  }
}



bool getPump() {
  if(WiFi.status()== WL_CONNECTED){ 
    HTTPClient http;

    String serverPath = serverName + "/acquire?id=" + id;

    http.begin(serverPath.c_str());

    int httpResponseCode = http.GET();

    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      String payload = http.getString();

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

Data getData() {
  if(WiFi.status()== WL_CONNECTED){ 
    HTTPClient http;

    String serverPath = serverName + "/data?id=" + id;
    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode >= 200 && httpResponseCode < 300) {
      String payload = http.getString();

      Serial.println(payload);

      StaticJsonDocument<200> doc;

      DeserializationError error = deserializeJson(doc, payload.c_str());

      if (error) {
        Serial.println("Deserilization failed :(");
        Serial.println(error.f_str());
        return Data(-1,-1);
      } 
      
      Data data(doc["min"], doc["max"]);

      Serial.print(data.getMin());
      Serial.print(" ");
      Serial.println(data.getMax());

      return data;
    }
  }
}


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