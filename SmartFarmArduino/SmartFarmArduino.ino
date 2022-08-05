// 스마트팜에서 입출력 센서를 사용하기 위한 라이브러리를 가져옵니다.
#include <Servo.h>
#include "DHT.h"
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
#include "RTClib.h" // 시계 모듈 라이브러리, 장착 시 주석해제
//#include <SoftwareSerial.h> // 해당 라이브러리는 메가에서는 통신핀이 있으므로 주석처리 해주세요.



// 각 센서의 핀을 정의합니다.
#define DHTPIN 12 // 온습도 핀
#define DHTTYPE DHT11 // 보드에 장착된 온습도 센서의 모델명
#define SERVOPIN 9 // 창문(서보모터) 핀
#define LIGHTPIN 4 // 천장 조명 핀
#define FAN_PIN 32 // 팬 핀
#define WATER_PUMP_PIN 31 // 펌프 핀
#define RGB_R 4 // 큰 LED의 빨간색 핀
#define RGB_G 35 // 큰 LED의 초록색 핀
#define RGB_B 36 // 큰 LED의 파란색 핀
#define SEND_DELAY 1000 // 센서값 전송 간격(단위 : ms)
#define WINDOW_OPEN_ANGLE 80 // 창문이 열렸을때의 서보모터 각도
#define WINDOW_CLOSE_ANGLE 0 // 창문이 열렸을때의 서보모터 각도


// 필요한 변수 및 상수를 선언합니다.
DHT dht(DHTPIN,DHTTYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27,16,2);
RTC_DS3231 rtc;

// 부울 변수
bool isConnected = true; // 스마트팜이 블루투스와 연결되었는지 판단합니다.
bool isFan = false; // 팬이 돌아가고 있는지 여부를 판단합니다.
bool isWindow = false; // 창문이 열려있는지 확인합니다.
bool isRTC = false;

// 캐릭터 변수
char sData[128] = { 0x00, };

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
bool isLightTime = false;
String lightTimeStart = "";
String lightTimeEnd = "";
//
bool isWindowTime = false;
String windowTimeStart = "";
String windowTimeEnd = "";
//
bool isFanTime = false;
String fanTimeStart = "";
String fanTimeEnd = "";

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  
  servo.attach(SERVOPIN);
  controlServo(0);
  
  pinMode(LIGHTPIN,OUTPUT);
  pinMode(FAN_PIN,OUTPUT);
  pinMode(WATER_PUMP_PIN,OUTPUT);
  pinMode(RGB_G,OUTPUT);
  pinMode(RGB_B,OUTPUT);
  pinMode(RGB_R,OUTPUT);
  serialSystemPrint("핀 모드 설정완료",false);
  
  startRTC();
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  serialSystemPrint("스마트팜 준비완료",false);
}

void loop() {
  if(Serial1.available()){
    remoteCommand(Serial1.readStringUntil('\n'));
  }
  if(isConnected){
    sendAllSensorValue();
    conditionCheck();
    timeCheck();
  }
  


}


/*
 * **********************************
 * 유형 : 함수
 * 설명 : 블루투스 신호를 받았을 때 명령어를 분류합니다.
 * 매개변수 :
 *    command : 명령어를 분류하기 위한 코드를 받습니다.
 * **********************************
 */
void remoteCommand(String command){
  if(command == "C"){
    isConnected = true;
    serialSystemPrint("기기와 연결됨",false);
  }else if(command == "D"){
    isConnected = false;
    serialSystemPrint("기기와 연결이 해제됨",false);
  }else if(command.indexOf("L,") != -1){
    controlLight(command.substring(2,command.length()).toInt());
  }else if(command.indexOf("F,") != -1){
    controlFan(command.substring(2,3).toInt());
  }else if(command.indexOf("W,") != -1){
    controlServo(command.substring(2,3).toInt());
  }else if(command.indexOf("CV,") != -1){
    if(command.indexOf("f") != -1){
      isCdcAuto = false;
    }else{
      isCdcAuto = true;
      cdcAuto = command.substring(command.indexOf("-")+1,command.indexOf("+")).toInt();
      isCdcAutoUp = command.indexOf("이상") != -1;
      isCdcAutoOn = command.indexOf("켜짐") != -1;
    }
  }
  
  else if(command.indexOf("TV,") != -1){
    if(command.indexOf("f") != -1){
      isTempAuto = false;
    }else{
      isTempAuto = true;
      tempAuto = command.substring(command.indexOf("-")+1,command.indexOf("+")).toInt();
      isTempAutoUp = command.indexOf("이상") != -1;
      isTempAutoOn = command.indexOf("열림") != -1;
    }
  }
  
  else if(command.indexOf("HV,") != -1){
    if(command.indexOf("f") != -1){
      isHumiAuto = false;
    }else{
      isHumiAuto = true;
      humiAuto = command.substring(command.indexOf("-")+1,command.indexOf("+")).toInt();
      isHumiAutoUp = command.indexOf("이상") != -1;
      isHumiAutoOn = command.indexOf("켜짐") != -1;
    }
  }else if(command.indexOf("T-") != -1){
    if(command.substring(2,3).toInt() == 0){
      if(command.indexOf(",f") != -1){
        isLightTime = false;
      }else{
        isLightTime = true;
        lightTimeStart = command.substring(11,13)+command.substring(17,19);
        lightTimeEnd = command.substring(30,32)+command.substring(36,38);
      }  
    }else if(command.substring(2,3).toInt() == 1){
      if(command.indexOf(",f") != -1){
        isWindowTime = false;
      }else{
        isWindowTime = true;
        windowTimeStart = command.substring(11,13)+command.substring(17,19);
        windowTimeEnd = command.substring(30,32)+command.substring(36,38);
      }  
    }else if(command.substring(2,3).toInt() == 2){
      if(command.indexOf(",f") != -1){
        isFanTime = false;
      }else{
        isFanTime = true;
        fanTimeStart = command.substring(11,13)+command.substring(17,19);
        fanTimeEnd = command.substring(30,32)+command.substring(36,38);
      }  
    }
    
  }else serialSystemPrint("알 수 없는 내용의 메시지가 수신되었습니다 : "+command,true);
}

/*
 * **********************************
 * 유형 : 함수
 * 설명 : 각 센서 값을 가져오는 함수를 통해서 디바이스에 센서값을 전송합니다
 * **********************************
 */
void sendAllSensorValue(){
  delay(SEND_DELAY);
  DateTime now = rtc.now(); // RTC 모듈에서 시간을 가져옵니다
  temp = dht.readTemperature(false);
  humid = dht.readHumidity(false);
  cdc = map(analogRead(A0),0,1023,0,100);
  memset(sData,0x00,128);
  sprintf(sData,"Temp:%d,Humid:%d,Cdc:%d,Fan:%d,Light:%d,Window:%d,Time:%d-%d-%d-%d-%d",temp,humid,cdc,isFan,lightBright,isWindow,now.year(),now.month(),now.day(),now.hour(),now.minute());
//  Serial.println(sData);
  Serial1.write(sData);
}


/*
 * **********************************
 * 유형 : 함수
 * 설명 : 준비 관련 메시지 출력을 위한 함수입니다.
 * 매개변수 : 
 *      message 매개변수를 받아 출력에 사용합니다
 * **********************************
 */
void serialSystemPrint(String message,bool isError){
  String printMessage = " "+message;
  Serial.print(isError ? "[ERROR]" : "[INFO]");
  Serial.println(printMessage);
}


/*
 * **********************************
 * 유형 : 함수
 * 설명 : RTC 모듈이 사용이 가능한지 확인하고 설정하는 함수입니다.
 * 매개변수 : 
 *      year //
 * **********************************
 */
void startRTC(){
  serialSystemPrint("RTC 모듈을 준비합니다",false);
  if(!rtc.begin()){
    serialSystemPrint("RTC 모듈을 찾지 못했습니다",true);
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    isRTC = false;
    return;
  }
  serialSystemPrint("RTC 모듈의 시간을 컴퓨터에 저장된 시간으로 설정합니다",false);
  rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  isRTC = true;
  return;
}

/*
 * **********************************
 * 유형 : 함수
 * 설명 : RTC 모듈이 사용이 가능한지 확인하고 설정하는 함수입니다.
 * 매개변수 : 
 *      year //
 * **********************************
 */
void controlLight(int value){
  lightBright = value;
  analogWrite(LIGHTPIN,map(value,0,100,0,255));
  
}

void controlFan(int value){
  isFan = value == 1 ? true : false;
  digitalWrite(FAN_PIN,value == 1 ? HIGH : LOW);
}

void controlServo(int value){
  isWindow = value == 1 ? true : false;
  servo.write(value == 1 ? 90 : 0);
}

void timeCheck(){
  int hour1 = rtc.now().hour();
  int min1 = rtc.now().minute();
  
  if(isLightTime){
    if(lightTimeStart.substring(0,2).toInt() <= hour1 && lightTimeEnd.substring(0,2).toInt() >= hour1){
      if(lightTimeStart.substring(0,2).toInt() == hour1){
        if(lightTimeStart.substring(2,4).toInt() <= min1){
          controlLight(100);
        } else controlLight(0);
      }else if(lightTimeEnd.substring(0,2).toInt() == hour1){
        if(lightTimeEnd.substring(2,4).toInt() >= min1) controlServo(1); 
        else controlLight(0);
      }else controlLight(100);
    }else controlLight(0);
  }
  
  if(isWindowTime){
    if(windowTimeStart.substring(0,2).toInt() <= hour1 && windowTimeEnd.substring(0,2).toInt() >= hour1){
      if(windowTimeStart.substring(0,2).toInt() == hour1){
        if(windowTimeStart.substring(2,4).toInt() <= min1){
          controlServo(1);
        } else controlServo(0);
      }else if(windowTimeEnd.substring(0,2).toInt() == hour1){
        if(windowTimeEnd.substring(2,4).toInt() >= min1) controlServo(1); 
        else controlServo(0);
      }else controlServo(1); 
    }else controlServo(0);
  }

  if(isFanTime){
    if(fanTimeStart.substring(0,2).toInt() <= hour1 && fanTimeEnd.substring(0,2).toInt() >= hour1){
      if(fanTimeStart.substring(0,2).toInt() == hour1){
        if(fanTimeStart.substring(2,4).toInt() <= min1){
          controlFan(1);
        } else controlFan(0);
      }else if(fanTimeEnd.substring(0,2).toInt() == hour1){
        if(fanTimeEnd.substring(2,4).toInt() >= min1) controlServo(1); 
        else controlFan(0);
      }else controlFan(1); 
    }else controlFan(0);
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
