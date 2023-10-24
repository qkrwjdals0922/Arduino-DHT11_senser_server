#include <ESP8266WiFi.h>
#include <DHT22.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Servo.h>

// WiFi 연결 정보 설정
const char* ssid = "박정민 핫스팟";  // WiFi SSID
const char* password = "qwer1234";  // WiFi 비밀번호

// DHT22 센서 설정
#define DATA_PIN 2
DHT22 dht22(DATA_PIN);

// 초음파 센서 핀 설정
#define TRIGGER_PIN 4  // 초음파 트리거 핀
#define ECHO_PIN 0     // 초음파 에코 핀 (에코 핀에 연결된 핀 번호)

// 서버 주소 설정
const char* send_serverAddress = "http://capstone.dothome.co.kr/wemos_app.php";
const char* get_serverAddress =  "http://capstone.dothome.co.kr/app_wemos.php";

Servo servo1;  // 서보 모터1 객체 생성
Servo servo2;  // 서보 모터2 객체 생성
int currentAngle1 = 0;  // 현재 각도를 저장할 변수1
int currentAngle2 = 0;  // 현재 각도를 저장할 변수2

// 데이터 전송을 위한 임시 변수들 설정
const String insert_mode = "insert";  // 데이터 발신 모드
const String select_mode = "select";  // 데이터 수신 모드
const String Gas = "235";     // 임시 가스 수치
const String Finedust = "24.4";     // 임시 미세먼지 수치

// 센서 값을 저장하기 위한 전역 변수 선언
float water_value = 0.0f;    // 초음파 센서로 측정한 데이터
String manual = " ";     // 수동, 자동 확인 변수
String barrier_value = " ";  // 차수벽 on, off 상태 확인 변수 

void setup() {
  Serial.begin(115200);
  connectWiFi();
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  servo1.attach(5);  //서보 모터1 5번핀
  servo2.attach(15); //서보 모터2 15번핀
  servo1.write(currentAngle1);  // 모터1의 초기 각도를 0도로 설정
  Serial.print(currentAngle1);
  servo2.write(currentAngle2);  // 모터2의 초기 각도를 0도로 설정
  Serial.print(currentAngle2);
}

// WiFi 연결 함수
void connectWiFi() {
  Serial.print("와이파이 연결중... ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("와이파이 연결 완료");
  Serial.println("IP address: " + WiFi.localIP().toString());
  Serial.println("--------------------------------------------------------------");
}

void loop() {
  float temp_value = dht22.getTemperature(); // 온도 측정
  float humi_value = dht22.getHumidity();    // 습도 측정

  Serial.print("습도: ");
  Serial.print(humi_value);
  Serial.print(" %\t");
  Serial.print("온도: ");
  Serial.print(temp_value);
  Serial.println(" *C");
  
  trigger();    //초음파 측정 함수
  
  // 발신 코드
  DynamicJsonDocument json(256);
  json["mode"] = insert_mode;
  json["manual"] = manual;
  json["barrier_value"] = barrier_value;
  json["temp_value"] = String(temp_value, 1);  
  json["humi_value"] = String(humi_value, 1); 
  json["Gas"] = Gas;
  json["water_value"] = String(water_value,1);

  String jsonString;
  serializeJson(json, jsonString); // json을 문자열로 변환하여 jsonString에 저장
  
  Serial.println("--------------------");
  Serial.println("생성된 JSON 문서: ");
  Serial.println(jsonString);

  sendHttpRequest(insert_mode, temp_value, humi_value);  // 발신용 URL 생성 함수
  receiveData(select_mode); // 수신모드 함수 
  
  if (manual == "auto" && water_value < 7) {
    currentAngle1 = 180;  // 90도로 설정
    servo1.write(currentAngle1);  // 서보 모터1 회전
    currentAngle2 = 180;  // 90도로 설정
    servo2.write(currentAngle2);  // 서보 모터2 회전
    barrier_value = "1";
    Serial.println("스마트 차수벽 자동 ON");
  }

  else if (manual == "auto" && water_value >= 7) {
    Serial.println("스마트 차수벽 자동 (OFF 보류중)");
  }

  else if (manual == "manual" && barrier_value == "1") {
    currentAngle1 = 180;  // 90도로 설정
    servo1.write(currentAngle1);  // 서보 모터1 회전
    currentAngle2 = 180;  // 90도로 설정
    servo2.write(currentAngle2);  // 서보 모터2 회전
    barrier_value = "1";
    Serial.println("스마트 차수벽 수동 ON");
  }

  else if (manual == "manual" && barrier_value == "0") {
    currentAngle1 = 0;  // 0도로 설정
    servo1.write(currentAngle1);  // 서보 모터1 회전
    currentAngle2 = 0;  // 0도로 설정
    servo2.write(currentAngle2);  // 서보 모터2 회전
    barrier_value = "0";
    Serial.println("스마트 차수벽 수동 off");
    }
    
    Serial.println("============================================================");
    delay(1000); // 1초 간격으로 반복 실행
}

// 발신용 HTTP 요청 보내는 함수
void sendHttpRequest(
  const String insert_mode,
  const float temp_value,
  const float humi_value)
{
  WiFiClient client;
  HTTPClient http;

  // 발신용 HTTP 요청 URL 설정
  String url = String(send_serverAddress) + "?";
  url += "mode=" + insert_mode;
  url += "&manual=" + manual;
  url += "&barrier_value=" + barrier_value;
  url += "&temp_value=" + String(temp_value, 1);
  url += "&humi_value=" + String(humi_value, 1);
  url += "&Gas=" + Gas;
  url += "&Finedust=" + Finedust;
  url += "&water_value=" + String(water_value, 1);

  http.begin(client, url);

  int httpResponseCode = http.GET(); // GET 방식 요청 보내기

  if (httpResponseCode > 0) {
    Serial.println("--------------------");
    Serial.println("생성된 발신용 URL: ");
    Serial.println(url);
  } else {
    Serial.println("Error sending HTTP request");
  }

  http.end();
}

void trigger () {
  // 초음파 트리거 핀을 펄스로 설정하여 초음파를 발신
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // 에코 핀에서 초음파의 반사 시간 측정
  unsigned long duration = pulseIn(ECHO_PIN, HIGH);

  // 초음파의 이동 시간을 거리로 변환 (음속: 340m/s)
  water_value = (duration / 2.0) * 0.0343;

  // 시리얼 모니터에 거리 출력
  Serial.print("거리: ");
  Serial.print(water_value);
  Serial.println(" cm");
 
  delay(500);  // 측정 주기 0.5
}

// 데이터 수신 함수
void receiveData(const String select_mode) {
  WiFiClient client;
  HTTPClient http;

  // 수신용 HTTP 요청 URL 설정
  String url = String(get_serverAddress) + "?";
  url += "mode=" + select_mode;

  http.begin(client, url);

  int httpResponseCode = http.GET(); // GET 방식 요청 보내기

  if (httpResponseCode > 0) {
    Serial.println("수신용 URL: ");
    Serial.println(url);
    Serial.println("--------------------");

    String response = http.getString();
    Serial.println("수신된 데이터: ");
    Serial.println(response);
    Serial.println("-------------------------------------------------------------");

    // JSON 데이터 파싱
    DynamicJsonDocument jsonDoc(256);
    DeserializationError error = deserializeJson(jsonDoc, response);

    if (!error) {
      String received_manual = jsonDoc[0]["manual"];
      String received_barrier_control = jsonDoc[0]["barrier_control"];

      if (received_manual == "auto") { 
        manual = "auto";
      } else if (received_manual == "manual") {
        manual = "manual";

      if(received_barrier_control == "1") {
          barrier_value = "1";
      } else if (received_barrier_control == "0") {
          barrier_value = "0";
        }
      }
    } else {
      Serial.print("수신 오류: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.println("Error sending HTTP request");
  }
  http.end();
}