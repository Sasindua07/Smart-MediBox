#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>

#define DHT_PIN 15
#define BUZZER 12
#define LDR1_PIN 34
#define LDR2_PIN 35
#define SERVO_PIN 18

Servo servo;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

char tempAr[6];
char ldrAr[6];

DHTesp dhtSensor;\

bool isScheduledON = false;
bool repeatDaily =false;

int pos = 0;
int minAngle = 30;
double cF = 0.75;
double D;
double intensity;

long scheduledOnTime;

void setup() {
  Serial.begin(115200);
  servo.attach(SERVO_PIN, 500, 2400);

  setupWifi();

  setupMqtt();

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  timeClient.begin();
  timeClient.setTimeOffset(5.5*3600);

  pinMode(BUZZER, OUTPUT);
  pinMode(LDR1_PIN,INPUT);
  pinMode(LDR2_PIN,INPUT);
  digitalWrite(BUZZER, LOW);


}

void loop() {
  if (!mqttClient.connected()) {
    connectToBroker();
  }
  mqttClient.loop();

  updateTemperature();
  updateLightIntensity();
  Serial.println(tempAr);
  Serial.println(ldrAr);
  mqttClient.publish("ENTC-ADMIN-TEMP", tempAr);
  mqttClient.publish("ENTC-ADMIN-LDR", ldrAr);

  updateShadedSlidingWindow();

  checkSchedule();
  delay(1000);
}

void buzzerOn(bool on) {
  if (on) {
    tone(BUZZER, 256);
  } else {
    noTone(BUZZER);
  }
}

void setupMqtt(){
  mqttClient.setServer("test.mosquitto.org", 1883);
  mqttClient.setCallback(receiveCallback);
}

void receiveCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  Serial.println();

  if (strcmp(topic, "ENTC-ADMIN-MAIN-ON-OFF") == 0) {
    if (payloadCharAr[0] == 't'){
      buzzerOn(true);
    }else{
      buzzerOn(false);
      noTone(BUZZER);
    }
  }else if(strcmp(topic,"ENTC-ADMIN-SCH-ON")== 0){
    if(payloadCharAr[0] == 'N'){
      isScheduledON = false;
    }else{
      isScheduledON = true;
      scheduledOnTime = atol(payloadCharAr);
    }
  }if(strcmp(topic,"ENTC-ADMIN-SERVO")== 0){
    minAngle=atol(payloadCharAr);
  }if(strcmp(topic,"ENTC-ADMIN-CF")== 0){
    cF=atof(payloadCharAr);
  }if(strcmp(topic,"ENTC-ADMIN-I")== 0){
    intensity=atof(payloadCharAr);
  }if(strcmp(topic,"ENTC-ADMIN-REPEAT-DAILY")== 0){
    if (payloadCharAr[0] == 't'){
      repeatDaily = true;
    }else{
      repeatDaily = false;
    }
  }
}

void connectToBroker() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32-34655655476")) {
      Serial.println("connected");
      mqttClient.subscribe("ENTC-ADMIN-MAIN-ON-OFF");
      mqttClient.subscribe("ENTC-ADMIN-SCH-ON");
      mqttClient.subscribe("ENTC-ADMIN-SERVO");
      mqttClient.subscribe("ENTC-ADMIN-CF");
      mqttClient.subscribe("ENTC-ADMIN-I");
      mqttClient.subscribe("ENTC-ADMIN-REPEAT-DAILY");
    } else {
      Serial.print("failed ");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void setupWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(". ");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void updateTemperature() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  String(data.temperature, 2).toCharArray(tempAr, 6);
}

void updateLightIntensity(){
  long sensorValue1=4095 - analogRead(LDR1_PIN);
  long sensorValue2=4095 - analogRead(LDR2_PIN);

  long ldrValue = max(sensorValue1,sensorValue2);
  String ldrValueString = String(map(ldrValue, 4095, 0, 1024, 0));
  if (sensorValue1>sensorValue2){
    ldrValueString = 'R' + ldrValueString;
    D=0.5;
  }else{
    ldrValueString = 'L' + ldrValueString;
    D=1.5;
  }
  ldrValueString.toCharArray(ldrAr, 6);
}

void updateShadedSlidingWindow(){
  int newPos = round(minAngle*D+(180-minAngle)*intensity*cF);

  pos = (min(newPos,180));

  servo.write(pos);
  delay(15);
}

unsigned long getTime() {
  timeClient.update();
  return timeClient.getEpochTime();
}

void checkSchedule() {
  if (isScheduledON) {
    unsigned long currentTime = getTime();
    if (currentTime > scheduledOnTime) {
      buzzerOn(true);
      if(repeatDaily == true){
        isScheduledON = true;
        mqttClient.publish("ENTC-ADMIN-MAIN-ON-OFF-ESP", "1");
        scheduledOnTime += 60;
      }else{
        isScheduledON = false;
        mqttClient.publish("ENTC-ADMIN-MAIN-ON-OFF-ESP", "1");
        mqttClient.publish("ENTC-ADMIN-SCH-ESP-ON", "0");
        Serial.println("Scheduled ON");
      } 
    }
  }  
}