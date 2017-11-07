#include <SoftwareSerial.h>
#include <DS1302.h>
#include <DHT11.h>
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUF_SIZE 16
#define COMMEND 0
#define START "start"
#define GOWORK "read"
#define CONTROL1 "relay1"
#define CONTROL2 "relay2"
#define CONTROL3 "relay3"
#define CONTROL4 "relay4"
#define DHT_H 0
#define DHT_T 1
#define SOUND 2
#define BRIGHT 3
#define PIR 4   

const int relay_pin[4] = {2, 3, 4, 5};
const int RST_pin = 6;
const int IO_pin = 7;
const int CLK_pin = 8;
const int PIR_pin = 9;
const int BTSerial_RX = 10;
const int BTSerial_TX = 11;
const int sound_pin = 12;
const int temp_pin = 13;
const int bright_pin = A0;
const int senserCommendList[5] = {DHT_H, DHT_T, SOUND, BRIGHT, PIR};
int error_DHT11 = 0;  // 온도센서 오류 여부
int bright = 0; // 밝기정도
float temp = 0.0;  // 온도
float humi = 0.0;  // 습도
bool stat_PIR = false;  // 인체감지 여부
bool sound_detect_stat = false;  // 소음감지 여부
char buf[BUF_SIZE];

typedef struct Relay {  // 릴레이 그룹
  int pin;  // 핀주소
  bool stat;  // 상태(True - 전력X , False - 전력O)
}Relay ; 

String dayAsString(const Time::Day day) {  // 요일 형태
  switch (day) {
    case Time::kSunday: return "Sunday";
    case Time::kMonday: return "Monday";
    case Time::kTuesday: return "Tuesday";
    case Time::kWednesday: return "Wednesday";
    case Time::kThursday: return "Thursday";
    case Time::kFriday: return "Friday";
    case Time::kSaturday: return "Saturday";
  }
  return "(unknown day)";
}

SoftwareSerial BTSerial(BTSerial_RX, BTSerial_TX);  // 블루투스 모듈 등록
DS1302 rtc(RST_pin, IO_pin, CLK_pin);  // RTC모듈 활성화
DHT11 dht11(temp_pin);  // 온도센서(DHT11) 활성화

void printTime();  // 시간 출력
void sendData(char *);  // 블루투스 모듈 송신
bool recieveData();  // 블루투스 모듈 수신
char * readSenser(int);  // 센서값 수신
void waitToTransmission();  // 데이터 수신 대기
void translateData();  // 데이터 해석

Relay controller[4];

void setup() {
  for(int i = 0; i < 4; i++) {
    controller[i].pin = relay_pin[i];
    controller[i].stat = false;
    pinMode(controller[i].pin, OUTPUT);
  }
  pinMode(sound_pin, INPUT);
  pinMode(bright_pin, INPUT);
  Serial.begin(9600);
  BTSerial.begin(9600);  // 블루투스 모듈 활성화
}

void loop() {
  waitToTransmission();
}

void printTime() {  
  Time t = rtc.time();
  const String day = dayAsString(t.day);
  char buf[50];
  
  snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
           day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
  Serial.println(buf);
}

bool recieveData() {
  static int i = 0;  // 전용 인덱스
  static bool start = false;
  if(BTSerial.available()) {
    buf[i] = BTSerial.read();
    i++;
    return true;
  }
  else {
    i = 0;
    return false;
  }
}

void sendData(char * data) {
  BTSerial.write(data);
}

char * readSenser(int option) {
  char dataOfBright[4];
  char dataOfHumi[4];
  char dataOfTemp[4];
  char dataOfPir;
  char dataOfSound;
 
  bright = analogRead(bright_pin);
  error_DHT11 = dht11.read(humi, temp);
  if(digitalRead(PIR_pin) == HIGH) {
    stat_PIR = true;
    dataOfPir = '1';
  }
  if(digitalRead(sound_pin) == HIGH) {
    sound_detect_stat = true;
    dataOfSound = '1';
  }
  if(!(error_DHT11)) {  // DEBUG용도
    Serial.print("temperature:");Serial.print(temp);
    Serial.print(" humidity:");Serial.print(humi);
    Serial.println();
  }
  else {
    Serial.println();
    Serial.print("Error No :");Serial.print(error_DHT11);
    Serial.println();    
  }
  itoa(bright, dataOfBright, 10);
  itoa((int)humi, dataOfHumi, 10);
  itoa((int)temp, dataOfTemp, 10);

  switch(option) {
    case DHT_H:
      return dataOfHumi;
      break;
    case DHT_T:
      return dataOfTemp;
      break;
    case SOUND:
      return dataOfSound;
      break;
    case BRIGHT:
      return dataOfBright;
      break;
    case PIR:
      return dataOfPir;
      break;
    default:
      Serial.println("I think we have a problem in readSenser()");
  }
}

void waitToTransmission() {
  static bool start = false;
  static int current = 0;
  static int count = 0;
  
  while(1) {
    if(recieveData()) {
      start = true;
      count++;
    }
    else {
      if(start) {  
        if(current-count != 0) {  // start값 변화 감지시 경주시작          
          break;                 // count는 수신받을시 값증가            
        }                       // current는 상관없이 계속증가 
        else {
          ;
        }
        current++;
      }
    }
  }
  translateData();
  current = 0;
  count = 0;
  start = false;
}

void translateData() {
  static int i = 0;
  char data[BUF_SIZE];
  int realSize = 0; 
  char * senserData[4];
  
  for(i = 0; i < BUF_SIZE; i++) {
    if(buf[i] == '\0') {
      realSize = i-1;  // 크기로 연산(명령문자제외)
      break;
    }
  }
  for(i = 0; i < realSize; i++) {
    data[i] = buf[i+1];
  }

  switch(buf[COMMEND]) {
    case 'c':
     if(!(strcmp(data, START))) {
      Time t = rtc.time();
      const String day = dayAsString(t.day);
      char buf[50];
  
      snprintf(buf, sizeof(buf), "%s %04d-%02d-%02d %02d:%02d:%02d",
           day.c_str(),
           t.yr, t.mon, t.date,
           t.hr, t.min, t.sec);
      //sendData(buf);  // 시작시간을 마스터에게 송신
     }
     else if(!(strcmp(data, GOWORK))) {
      for(i = 0; i < 5; i++) {
        senserData[i] = readSenser(senserCommendList[i]);  // 4개의 센서로부터 데이터 수신
      }                                                   
      for(i = 0; i < 5; i++) {
        sendData(senserData[i]);
      }
     }
     else if(!(strcmp(data, CONTROL1))) {  
      if(controller[0].stat == true) {
        digitalWrite(controller[0].pin, LOW);
      }
      else {
        digitalWrite(controller[0].pin, HIGH);
      }
     }
     else if(!(strcmp(data, CONTROL2))) {  
      if(controller[1].stat == true) {
        digitalWrite(controller[1].pin, LOW);
      }
      else {
        digitalWrite(controller[1].pin, HIGH);
      }
     }
     else if(!(strcmp(data, CONTROL3))) {  
      if(controller[2].stat == true) {
        digitalWrite(controller[2].pin, LOW);
      }
      else {
        digitalWrite(controller[2].pin, HIGH);
      }
     }
     else if(!(strcmp(data, CONTROL4))) {  
      if(controller[3].stat == true) {
        digitalWrite(controller[3].pin, LOW);
      }
      else {
        digitalWrite(controller[3].pin, HIGH);
      }
     }
     break;
    default:
      Serial.println("I think we have a problem!");
      Serial.print("Error code : "); Serial.print(buf[COMMEND]);
  }
}

