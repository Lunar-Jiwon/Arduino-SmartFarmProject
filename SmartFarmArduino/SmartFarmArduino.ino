// 스마트팜에서 입출력 센서를 사용하기 위한 라이브러리를 가져옵니다.
#include <Servo.h>
#include "DHT.h"
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
//#include "DS3231.h"
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
#define SEND_DELAY 800 // 센서값 전송 간격
#define WINDOW_OPEN_ANGLE 80 // 창문이 열렸을때의 서보모터 각도
#define WINDOW_CLOSE_ANGLE 0 // 창문이 열렸을때의 서보모터 각도


// 필요한 변수 및 상수를 선언합니다.
DHT dht(DHTPIN,DHTTYPE);
Servo servo;
LiquidCrystal_I2C lcd(0x27,16,2);

bool isConnected = true;
bool isFan = false; // 팬이 돌아가고 있는지 여부를 판단합니다.
bool isWindow = false; // 창문이 열려있는지 확인합니다.
bool isLight = false;
char sData[128] = { 0x00, };
String date = "2022-07-09,15:44";

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  serialSystemPrint("핀 모드 설정중");
  
  pinMode(LIGHTPIN,OUTPUT);
  pinMode(FAN_PIN,OUTPUT);
  pinMode(WATER_PUMP_PIN,OUTPUT);
  pinMode(RGB_G,OUTPUT);
  pinMode(RGB_B,OUTPUT);
  pinMode(RGB_R,OUTPUT);
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  serialSystemPrint("스마트팜 준비완료");
}

void loop() {
 
  if(Serial1.available()){
    delay(10);
    remoteCommand((char)Serial1.read());
  }
  
  if(isConnected){

    sendAllSensorValue();
    delay(SEND_DELAY);
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
void remoteCommand(char command){
  if(command == 'C'){
    isConnected = true;
    serialSystemPrint("기기와 연결됨");
  }else if(command == 'D'){
    isConnected = false;
    serialSystemPrint("기기와 연결이 해제됨");
  }
}

/*
 * **********************************
 * 유형 : 함수
 * 설명 : 각 센서 값을 가져오는 함수를 통해서 디바이스에 센서값을 전송합니다
 * **********************************
 */
void sendAllSensorValue(){

  
  int temp = dht.readTemperature(false);
  int humid = dht.readHumidity(false);
  int cdc = analogRead(A0);
  memset(sData,0x00,128);
  sprintf(sData,"Temp:%d,Humid:%d,Cdc:%d,Fan:%d,Light:%d,Window:%d",temp,humid,cdc,isFan,isLight,isWindow);
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
void serialSystemPrint(String message){
  String printMessage = "\t"+message+"\t\t\t";
  Serial.println("************************************");
  Serial.println(printMessage);
  Serial.println("************************************\n");
}
