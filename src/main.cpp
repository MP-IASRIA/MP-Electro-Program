/**
 * @file main.cpp
 * @author N. AZZOUZ, A. MELLAH, R. ALOUI
 * @brief 
 * @version 0.1
 * @date 2023-01-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <Arduino.h>

#include <WiFi.h>
#include <WiFiUdp.h>  // Utilisation du protocole UDP (pour les applications temps réel)
#include <PubSubClient.h>
#include <NTPClient.h>


#define WIFISSID "Redmi Note 11" // Enter WifiSSID here
#define PASSWORD "isetbeja" // Enter password here
#define TOKEN "BBFF-Hz6tdZJl9805kiV1KRrxKrXaEjIV24" // Ubidots' TOKEN
#define MQTT_CLIENT_NAME "BBFF-Hz6tdZJl9805kiV1KRrxKrXaEjIV24" // MQTT client Name
#define VARIABLE_LABEL "ecg_val" // ubidots variable label
#define DEVICE_LABEL "AD8232_IASRIA" // ubidots device label
#define T_SAMPLING 150 // Period of sampling (ms)


#define SENSORPIN A0 // Set the A0 as SENSORPIN

char mqttBroker[]  = "industrial.api.ubidots.com";
char payload[10000];
char topic[150];
// Space to store values to send
char str_sensor[10];
char str_millis[20];
double epochseconds = 0;
double epochmilliseconds = 0;
double current_millis = 0;
double current_millis_at_sensordata = 0;
double timestampp = 0;
int j = 0;
/****************************************
   Auxiliar Functions
 ****************************************/
WiFiClient ubidots;
PubSubClient client(ubidots);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  Serial.write(payload, length);
  Serial.println(topic);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      delay(2000);
    }
  }
}

/****************************************
   setup function
 ****************************************/
void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFISSID, PASSWORD);
  // Assign the pin as INPUT
  pinMode(SENSORPIN, INPUT);

  Serial.println();
  Serial.print("Waiting for WiFi...");

  while (WiFi.status() != WL_CONNECTED) { // Try to connect to network
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  client.setServer(mqttBroker, 1883); // Try to connect to MQTT server
  client.setCallback(callback);
  timeClient.update();
  epochseconds = timeClient.getEpochTime();
  epochmilliseconds = epochseconds * 1000;
  Serial.print("epochmilliseconds=");
  Serial.println(epochmilliseconds);
  current_millis = millis();
  Serial.print("current_millis=");
  Serial.println(current_millis);
}

/****************************************
   setup function
 ****************************************/
void loop() {
  if (!client.connected()) {
    reconnect();
    j = 0;
  }
  
  j = j + 1;  // Messages sent
  Serial.print("j=");
  Serial.println(j);
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\": [", VARIABLE_LABEL); // Adds the variable label
  for (int i = 1; i <= 3; i++)
  {
    float sensor = analogRead(SENSORPIN); // Read from sensor
    dtostrf(sensor, 4, 2, str_sensor);
    sprintf(payload, "%s{\"value\":", payload); // Adds the value
    sprintf(payload, "%s %s,", payload, str_sensor); // Adds the value
    current_millis_at_sensordata = millis();
    timestampp = epochmilliseconds + (current_millis_at_sensordata - current_millis);
    dtostrf(timestampp, 10, 0, str_millis);
    sprintf(payload, "%s \"timestamp\": %s},", payload, str_millis); // Adds the value
    delay(T_SAMPLING);
  }

  float sensor = analogRead(SENSORPIN); // Read from sensor
  dtostrf(sensor, 4, 2, str_sensor);
  current_millis_at_sensordata = millis();
  timestampp = epochmilliseconds + (current_millis_at_sensordata - current_millis);
  dtostrf(timestampp, 10, 0, str_millis);
  sprintf(payload, "%s{\"value\":%s, \"timestamp\": %s}]}", payload, str_sensor, str_millis);
  Serial.println("Publishing data to Ubidots Cloud");
  client.publish(topic, payload); // Publish data to ubidots.com
  Serial.println(payload);
  delay(T_SAMPLING);
}