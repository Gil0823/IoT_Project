#include <SoftwareSerial.h>
#include <DS1302.h>
#include <DHT11.h>
#include <stdio.h>
#include <stdarg.h>
#define BUF_SIZE 256     

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
void sendData();  // 블루투스 모듈 송신
bool recieveData();  // 블루투스 모듈 수신
void readSenser(int x, ...);  // 센서값 수신
void waitToTransmission();  // 데이터 수신 대기
void translateData(char);

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
  bright = analogRead(bright_pin);
  error_DHT11 = dht11.read(humi, temp);
  if(digitalRead(PIR_pin) == HIGH) {
    stat_PIR = true;
  }
  if(digitalRead(sound_pin) == HIGH) {
    sound_detect_stat = true;
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
          current = 0;           // count는 수신받을시 값증가
          count = 0;            // current는 상관없이 계속증가
          start = false; 
          break;                 
        }                       
        else {
          ;
        }
        current++;
      }
    }
  }
  translateData();
}

void translateData() {
  ;
}

/*void readSenser() {
  
}*/

