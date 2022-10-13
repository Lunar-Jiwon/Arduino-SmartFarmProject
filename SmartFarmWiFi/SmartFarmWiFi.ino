/**
2022년 10월, 웹 대시보드 앱과 연동을 위한 아두이노 코드
사용언어 : C++, Javascript
사용 플랫폼 : Firebase RealTimeDatabase

사용 라이브러리 :
ArduinoJson@5.13.5

보드 매니저 :
ESP8266@3

*/

/* 스마트팜에서 입출력 센서를 사용하기 위한 라이브러리를 가져옵니다. */
#include <Servo.h>
#include "DHT.h"
#include <Wire.h>
#include "LiquidCrystal_I/2C.h"
#include "RTCli/b.h"
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "ArduinoJson.h"

/* Firebase와 ESP8266 초기설정 */
#define DATABASE_URL ""  // 데이터베이스 주소
#define API_KEY ""                   // 웹API키

#define WIFI_SSID ""  // 연결 할 네트워크 명
#define WIFI_PASSWORD ""   // 연결 할 네트워크 비밀번호

#define DATABASE_FARM_KEY "farms/dyms/plant"

/* 스마트팜의 각 센서 핀을 정의 */
#define DHTPIN 12             // 온습도 핀
#define DHTTYPE DHT11         // 보드에 장착된 온습도 센서의 모델명
#define SERVOPIN 9            // 창문(서보모터) 핀
#define LIGHTPIN 4            // 천장 조명 핀
#define FAN_PIN 32            // 팬 핀
#define WATER_PUMP_PIN 31     // 펌프 핀
#define RGB_R 4               // 큰 LED의 빨간색 핀
#define RGB_G 35              // 큰 LED의 초록색 핀
#define RGB_B 36              // 큰 LED의 파란색 핀
#define SEND_DELAY 1000       // 센서값 전송 간격(단위 : ms)
#define WINDOW_OPEN_ANGLE 80  // 창문이 열렸을때의 서보모터 각도
#define WINDOW_CLOSE_ANGLE 0  // 창문이 열렸을때의 서보모터 각도

FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

DHT dht(DHTPIN, /DHTTYPE);
Servo servo;/
LiquidCrystal_I/2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;/

bool isSignUp = false;

bool isFan = false; // 팬이 돌아가고 있는지 여부를 판단합니다.
bool isWindow = false; // 창문이 열려있는지 확인합니다.
bool isPump = false; // 펌프 작동여부
bool isRTC = false;

// 정수형 변수
int lightBright = 0; // LED 라이트의 밝기를 저장합니다
int cdc = 0; // 조도센서의 값을 저장합니다
int humid = 0; // 온습도 센서의 습도값을 저장합니다
int temp = 0; // 온습도 센서의 온도값을 저장합니다

// 문자형 변수
String date = "2022-07-09,15:44";
String message = "";

// 조건변수의 변수
// 조도항목
bool isCdcAuto = false;
bool isCdcAutoUp = false;
bool isCdcAutoOn = false;
int cdcAuto = 0;
// 온도항목
bool isTempAuto = false;
bool isTempAutoUp = false;
bool isTempAutoOn = false;
int tempAuto = 0;
// 습도항목
bool isHumiAuto = false;
bool isHumiAutoUp = false;
bool isHumiAutoOn = false;
int humiAuto = 0;

// 시간제어 변수
// 조명
bool isLightTime1 = false;
String lightTimeStart1 = "";
String lightTimeEnd1 = "";

bool isLightTime2 = false;
String lightTimeStart2 = "";
String lightTimeEnd2 = "";

bool isLightTime3 = false;
String lightTimeStart3 = "";
String lightTimeEnd3 = "";
//
bool isWindowTime1 = false;
String windowTimeStart1 = "";
String windowTimeEnd1 = "";

bool isWindowTime2 = false;
String windowTimeStart2 = "";
String windowTimeEnd2 = "";

bool isWindowTime3 = false;
String windowTimeStart3 = "";
String windowTimeEnd3 = "";
//

bool isFanTime1 = false;
String fanTimeStart1 = "";
String fanTimeEnd1 = "";

bool isFanTime2 = false;
String fanTimeStart2 = "";
String fanTimeEnd2 = "";

bool isFanTime3 = false;
String fanTimeStart3 = "";
String fanTimeEnd3 = "";


void setup() {
  Serial.begin(115200);
  Serial.println("[INFO] Starting...");
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("[INFO] 네트워크와 연결중입니다");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("[INFO] 네트워크와 연결되었습니다");

  /* Firebase 로그인 전 설정값 설정 */
  config.api_key = API_KEY;            // API 키 설정
  config.database_url = DATABASE_URL;  // 데이터베이스 URL설정

  /* Firebase 로그인 시도 */
  Serial.println("[INFO] Firebase에 로그인을 시도합니다");
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("[INFO] Firebase 로그인이 완료되었습니다");
    isSignUp = true;
  } else {
    Serial.printf("[ERROR] Firebase 로그인 오류 : %c\n", config.signer.signupError.message.c_str());
    return;
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
#if defined(ESP8266)
  stream.setBSSLBufferSize(2048, 512);  // RX :2048 / TX :512
#endif

  if (!Firebase.RTDB.beginStream(&stream, "/farms/dyms"))
    Serial.printf("[ERROR] 실시간 쿼리에 오류가 발생했습니다, %s\n\n", stream.errorReason().c_str());
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);
  Serial.println("[INFO] Firebase 초기 설정 완료");
  Serial.println("[INFO] RTC 모듈 시작");
  startRTC();

  

  
}

void loop() {
  conditionCheck();
  updateSensorValue();
  delay(500);
}

void updateSensorValue(){
  FirebaseJson updateData;

  temp = dht.readTemperature(false);
  humid = dht.readHumidity(false);
  cdc = map(analogRead(A0),0,1023,0,100);
  updateData.set("Fan",isFan);
  updateData.set("Temp",temp);
  updateData.set("LightCDC",cdc);
  updateData.set("Humid",humid);
  updateData.set("Light",lightBright > 0 ? true : false);
  updateData.set("Windows",isWindow);
  if(Firebase.RTDB.updateNode(&fbdo,"farms/dyms/sensorStatus",&updateData));
  else Serial.println(fbdo.errorReason());
  
}

void streamTimeoutCallback(bool timeout) {
  if (timeout)
    Serial.println("[WARN] 실시간 수신 대기시간이 초과되었습니다, 재연결을 시도합니다\n");

  if (!stream.httpConnected())
    Serial.printf("[ERROR] 에러 코드 : %d, 상세내용 : %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

/*
  데이터베이스 실시간 수신에 필요한 콜백함수
  데이터베이스에서 변경사항이 발생하면 함수가 실행됨.
*/
void streamCallback(FirebaseStream data) {
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, data.jsonString());
  if (error) {
    Serial.print("[ERROR] 데이터 역직렬화 실패 : ");
    Serial.println(error.c_str());
    return;
  }
  JsonObject sensorStatus = doc["sensorStatus"];
  bool sensorStatus_Fan = sensorStatus["Fan"];           // false
  int sensorStatus_Humid = sensorStatus["Humid"];        // 0
  int sensorStatus_Light = sensorStatus["Light"];        // 0
  int sensorStatus_LightCDC = sensorStatus["LightCDC"];  // 0
  bool sensorStatus_Pump = sensorStatus["Pump"];         // false
  int sensorStatus_Temp = sensorStatus["Temp"];          // 0
  bool sensorStatus_Windows = sensorStatus["Windows"];   // false

  const char* startDay = doc["startDay"];  // "2022-10-01"

  for (JsonPair timeControl_item : doc["timeControl"].as<JsonObject>()) {
    const char* timeControl_item_key = timeControl_item.key().c_str();  // "Fan", "Light", "Pump", "Window"
    for (JsonObject timeControl_item_value_schedule : timeControl_item.value()["schedules"].as<JsonArray>()) {
      const char* timeControl_item_value_schedule_endTime = timeControl_item_value_schedule["endTime"];
      bool timeControl_item_value_schedule_isEnabled = timeControl_item_value_schedule["isEnabled"];  // false, ...
      const char* timeControl_item_value_schedule_startTime = timeControl_item_value_schedule["startTime"];
    }
  }

  for (JsonPair varControl_item : doc["varControl"].as<JsonObject>()) {
    const char* varControl_item_key = varControl_item.key().c_str();  // "FanControl", "LightControl", ...

    const char* varControl_item_value_command = varControl_item.value()["command"];  // "true/true/5", ...
    bool varControl_item_value_isEnabled = varControl_item.value()["isEnabled"];     // true, false, true, true
  }


  controlFan(doc["sensorStatus"]["Fan"]);
  controlLight(doc["sensorStatus"]["Light"]);
  // controlServo(doc["sensorStatus"]["Windows"]);
 
  isCdcAutoOn = getAutoOn(doc,"LightControl");
  isCdcAutoUp = getAutoUp(doc,"LightControl");
  isCdcAuto = getIsAuto(doc,"LightControl");
  cdcAuto = getAutoValue(doc,"LightControl");

  isTempAutoOn = getAutoOn(doc,"WindowControl");
  isTempAutoUp = getAutoUp(doc,"WindowControl");
  isTempAuto = getIsAuto(doc,"WindowControl");
  tempAuto = getAutoValue(doc,"WindowControl");

  isHumiAutoOn = getAutoOn(doc,"FanControl");
  isHumiAutoUp = getAutoUp(doc,"FanControl");
  isHumiAuto = getIsAuto(doc,"FanControl");
  humiAuto = getAutoValue(doc,"FanControl");
  

  isLightTime1 = getIsTimerEnabled(doc,"Light",0);
  isLightTime2 = getIsTimerEnabled(doc,"Light",1);
  isLightTime3 = getIsTimerEnabled(doc,"Light",2);
  //
  isFanTime1 = getIsTimerEnabled(doc,"Fan",0);
  isFanTime2 = getIsTimerEnabled(doc,"Fan",1);
  isFanTime3 = getIsTimerEnabled(doc,"Fan",2);
  //
  isWindowTime1 = getIsTimerEnabled(doc,"Window",0);
  isWindowTime2 = getIsTimerEnabled(doc,"Window",1);
  isWindowTime3 = getIsTimerEnabled(doc,"Window",2);

  lightTimeStart1 = getStartTime(doc,"Light",0);
  lightTimeStart2 = getStartTime(doc,"Light",1);
  lightTimeStart3 = getStartTime(doc,"Light",2);
  //
  fanTimeStart1 = getStartTime(doc,"Fan",0);
  fanTimeStart2 = getStartTime(doc,"Fan",1);
  fanTimeStart3 = getStartTime(doc,"Fan",2);
  //
  windowTimeStart1 = getStartTime(doc,"Window",0);
  windowTimeStart2 = getStartTime(doc,"Window",1);
  windowTimeStart3 = getStartTime(doc,"Window",2);
  
  lightTimeEnd1 = getEndTime(doc,"Light",0);
  lightTimeEnd2 = getEndTime(doc,"Light",1);
  lightTimeEnd3 = getEndTime(doc,"Light",2);
  //
  fanTimeEnd1 = getEndTime(doc,"Fan",0);
  fanTimeEnd2 = getEndTime(doc,"Fan",1);
  fanTimeEnd3 = getEndTime(doc,"Fan",2);
  //
  windowTimeEnd1 = getEndTime(doc,"Window",0);
  windowTimeEnd2 = getEndTime(doc,"Window",1);
  windowTimeEnd3 = getEndTime(doc,"Window",2);
}

// 활성화 여부
int getAutoOn(JsonDocument& doc, String sensorName){
  return String(doc["varControl"][sensorName]["command"]).substring(String(doc["varControl"][sensorName]["command"]).indexOf("/")+1,String(doc["varControl"][sensorName]["command"]).indexOf("-")) == "true" ? true : false;
}
// UP 이상 이하
int getAutoUp(JsonDocument& doc, String sensorName){
  return String(doc["varControl"][sensorName]["command"]).substring(0,String(doc["varControl"][sensorName]["command"]).indexOf("/")) == "true" ? true : false;
}
// Enabled 조건활성화여부
int getIsAuto(JsonDocument& doc, String sensorName){
  return String(doc["varControl"][sensorName]["isEnabled"]) == "true" ? true : false;
}
// 값
int getAutoValue(JsonDocument& doc, String sensorName){
  return String(doc["varControl"]["sensorName"]["command"]).substring(String(doc["varControl"]["LightControl"]["command"]).indexOf("-")+1).toInt();
}

// 타이머 활성화여부
bool getIsTimerEnabled(JsonDocument& doc, String sensorName, int number){
  return String(doc["timeControl"][sensorName]["schedules"][number]["isEnabled"]) == "false" ? false : true;
}

String getStartTime(JsonDocument& doc, String sensorName, int number){
  return String(doc["timeControl"][sensorName]["schedules"][number]["startTime"]);
}

String getEndTime(JsonDocument& doc, String sensorName, int number){
  return String(doc["timeControl"][sensorName]["schedules"][number]["endTime"]);
}

/* 
  RTC 모듈 확인
*/

void startRTC(){
  Serial.println("[INFO] RTC 모듈을 준비합니다");
  if(!rtc.begin()){
     Serial.println("[ERROR] RTC 모듈을 찾지 못했습니다");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    isRTC = false;
    return;
  }
   Serial.println("[INFO] RTC 모듈의 시간을 컴퓨터에 저장된 시간으로 설정합니다");
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  isRTC = true;
  return;
}

void controlLight(int value){
  lightBright = value;
  analogWrite(LIGHTPIN,map(value,0,100,0,255));
}

void controlFan(bool value){
  isFan = value;
  digitalWrite(FAN_PIN,value);
}

void controlServo(bool value){
  isWindow = value;
  servo.write(value);
}

void timeCheck(){
  controlLight(timeSensorCheck(isLightTime1, lightTimeStart1,lightTimeEnd1) ? 100 : 0);
  controlLight(timeSensorCheck(isLightTime2, lightTimeStart1,lightTimeEnd2) ? 100 : 0);
  controlLight(timeSensorCheck(isLightTime3, lightTimeStart1,lightTimeEnd3) ? 100 : 0);

  controlServo(timeSensorCheck(isWindowTime1, windowTimeStart1,windowTimeEnd1));
  controlServo(timeSensorCheck(isWindowTime2, windowTimeStart3,windowTimeEnd2));
  controlServo(timeSensorCheck(isWindowTime3, windowTimeStart3,windowTimeEnd3));

  controlFan(timeSensorCheck(isFanTime1, fanTimeStart1,fanTimeEnd1));
  controlFan(timeSensorCheck(isFanTime2, fanTimeStart2,fanTimeEnd2));
  controlFan(timeSensorCheck(isFanTime3, fanTimeStart3,fanTimeEnd3));

}

bool timeSensorCheck(bool isEnabled, String sTime, String eTime){
  int hour1 = rtc.now().hour();
  int min1 = rtc.now().minute();
  if(isEnabled){
    if(sTime.substring(0,2).toInt() <= hour1 && eTime.substring(0,2).toInt() >= hour1){
      if(sTime.substring(0,2).toInt() == hour1){
        if(sTime.substring(2,4).toInt() <= min1){
          return true;
        } else return false;
      }else if(eTime.substring(0,2).toInt() == hour1){
        if(eTime.substring(2,4).toInt() >= min1) return true;
        else return false;
      }else return true;
    }else return false;
  }
}

void conditionCheck(){
  if(isCdcAuto){
    if(isCdcAutoUp && cdcAuto <= cdc){
      controlLight(isCdcAutoOn ? 100 : 0);
    }else if(!isCdcAutoUp && cdcAuto >= cdc){
      controlLight(isCdcAutoOn ? 100 : 0);
    }else{
      controlLight(isCdcAutoOn ? 0 : 100);
    }
  }

  if(isTempAuto){
    if(isTempAutoUp && tempAuto <= temp){
      controlServo(isTempAutoOn ? 1 : 0);
    }else if(!isTempAutoUp && tempAuto >= temp){
      controlServo(isTempAutoOn ? 1 : 0);
    }else{
      controlServo(isTempAutoOn ? 0 : 1);
    }
  }
  
  if(isHumiAuto){
    if(isHumiAutoUp && humiAuto <= humid){
      controlFan(isHumiAutoOn ? 1 : 0);
    }else if(!isHumiAutoUp && humiAuto >= humid){
      controlFan(isHumiAutoOn ? 1 : 0);
    }else{
      controlFan(isHumiAutoOn ? 0 : 1);
    }
  }
}
