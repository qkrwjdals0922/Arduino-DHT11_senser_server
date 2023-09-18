#include <ESP8266WiFi.h>
#include <DHT22.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// WiFi 연결 정보 설정
const char* ssid = "박정민 핫스팟";  // WiFi SSID
const char* password = "qwer1234";  // WiFi 비밀번호

// DHT11 센서 설정
#define data 2        // DHT 데이터 핀 2
#define DHTTYPE DHT22    // DHT22 센서 사용
DHT22 dht22(data);

// 서버 주소 설정
const char* send_serverAddress = "http://capstone.dothome.co.kr/wemos_app.php";
const char* get_serverAddress =  "http://capstone.dothome.co.kr/app_wemos.php";

String manual = " ";    // 수동 or 자동 확인 변수
String barrier_value = " "; // 차수벽 on,off 상태 확인 변수 

void setup() {
  Serial.begin(115200);
  connectWiFi();
  //dht.begin();
}

// WiFi 연결 함수
void connectWiFi() {
  Serial.print("와이파이 연결중... ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  // 와이파이 연결                                                                                

  Serial.println("");
  Serial.println("와이파이 연결 완료");
  Serial.println("IP address: " + WiFi.localIP().toString());
  Serial.println("--------------------------------------------------------------");
}

void loop() {
  // 데이터 전송을 위한 변수들 설정
  String insert_mode = "insert";  // 데이터 발신 모드
  String select_mode = "select";  // 데이터 수신 모드
  String car_value = "15";     // 임시 차량 포화도     
  float temp_value = dht22.getTemperature(); // 온도 측정
  float humi_value = dht22.getHumidity();    // 습도 측정

  Serial.println("측정된 센서 값: ");
  Serial.print("습도: ");
  Serial.print(humi_value);
  Serial.print(" %\t");
  Serial.print("온도: ");
  Serial.print(temp_value);
  Serial.println(" *C");
  // 센서값 출력

  // 발신 코드
  {
  DynamicJsonDocument json(256);
  json["mode"] = insert_mode;
  json["manual"] = manual;
  json["car_value"] = car_value.toFloat();
  json["barrier_value"] = barrier_value;
  json["temp_value"] = String(temp_value, 1);  // 센서값 소수점 1의 자리에서 자르기
  json["humi_value"] = String(humi_value, 1);  // 센서값 소수점 1의 자리에서 자르기
  // 센서값을 json 변수에 저장

  String jsonString;
  serializeJson(json, jsonString); // json을 문자열로 변환하여 jsonString에 저장
  
  Serial.println("--------------------");
  Serial.println("생성된 JSON 문서: ");
  Serial.println(jsonString);  // JSON 문자열을 시리얼 모니터에 출력

  sendHttpRequest(insert_mode, manual, car_value, barrier_value, temp_value, humi_value);
  // HTTP 요청 보내기
  }

  // 수신 코드
  {
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
      Serial.println(response); // 수신된 데이터 출력
      Serial.println("-------------------------------------------------------------");

      // JSON 데이터 파싱
      DynamicJsonDocument jsonDoc(256);
      DeserializationError error = deserializeJson(jsonDoc, response);

      if (!error) {
        String received_manual = jsonDoc[0]["manual"];
        String received_barrier_control = jsonDoc[0]["barrier_control"];
        // JSON 데이터에서 각 변수 추출

        if (received_manual == "auto") { 
          manual = "auto";
          Serial.println("스마트 차수벽 자동 모드");
          // 자동 모드로 변환
  
        } else if (received_manual == "manual") {
          manual = "manual";
          Serial.println("스마트 차수벽 수동모드");

          if(received_barrier_control == "1") {
            barrier_value = "1";
            Serial.println("스마트 차수벽 on");
          } else if (received_barrier_control == "0") {
            barrier_value = "0";
            Serial.println("스마트 차수벽 off");
          }
          // 수동 모드로 변환 후 차수벽 상태 확인 
        }
      } else {
        Serial.print("수신 오류: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.println("Error sending HTTP request");
    }
    Serial.println("============================================================");
    http.end();
  }

  delay(5000); // 5초 간격으로 반복 실행
}

// HTTP 요청 보내는 함수
void sendHttpRequest(
  String insert_mode,
  String manual,
  String car_value,
  String barrier_control,
  float temp_value,
  float humi_value)
{
  WiFiClient client;
  HTTPClient http;

  // 발신용 HTTP 요청 URL 설정
  String url = String(send_serverAddress) + "?";
  url += "mode=" + insert_mode;
  url += "&manual=" + manual;
  url += "&car_value=" + car_value;
  url += "&barrier_value=" + barrier_control;
  url += "&temp_value=" + String(temp_value, 1);
  url += "&humi_value=" + String(humi_value, 1);

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