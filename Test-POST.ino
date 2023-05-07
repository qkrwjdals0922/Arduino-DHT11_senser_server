#include <ESP8266WiFi.h>   
#include <DHT.h>            
#include <WiFiClient.h>   
#include <ESP8266HTTPClient.h> 
#include <ArduinoJson.h>    

const char* ssid = "KT_GiGA_2G_Wave2_5B1D";  // WiFi SSID
const char* password = "4gxf7ke740";        // WiFi 비밀번호

#define DHTPIN 2         // DHT11 데이터 핀
#define DHTTYPE DHT11    // DHT11 센서 사용

DHT dht(DHTPIN, DHTTYPE);

const char* serverAddress = "http://34.64.163.160/insert_db.php";
// 웹 서버 주소 설정

void setup() {
  Serial.begin(115200);
  // WiFi 연결
  connectWiFi();
  dht.begin();
}

void loop() {
  float temp_value = dht.readTemperature();
  float humi_value = dht.readHumidity();
  // 온습도 측정

  StaticJsonDocument<200> json;
  json["temperature"] = temp_value;
  json["humidity"] = humi_value;
   // JSON 객체 생성
   
  serializeJsonPretty(json, Serial);
   //JSON값 시리얼모니터에 출력 
  Serial.println(" ");
  Serial.print("습도: ");
  Serial.print(humi_value);
  Serial.print(" %\t");
  Serial.print("온도: ");
  Serial.print(temp_value);
  Serial.println(" *C");
  //원본 출력 

  String jsonString;
  serializeJson(json, jsonString);
   // JSON 문자열로 변환

  sendHttpRequest(jsonString);
  // HTTP 요청 보내기

  delay(5000);
}

void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  // 와이파이 연결 

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void sendHttpRequest(String payload) {
  WiFiClient client;

  HTTPClient http;
  http.begin(client, serverAddress);
  // HTTP 요청 설정

  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);
  // POST 본문 추가

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.println("Error sending HTTP request");
  }
  http.end();
}