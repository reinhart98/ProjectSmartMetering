#include "ACS712.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <PubSubClient.h>

int daya;
// Add your MQTT Broker IP address, example:
const char* mqtt_server = "192.168.43.87";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

//char auth[] = "zUTOKy7Aq19qUXRkT78CyBEY-JEzc3po";
char ssid[] = "Shinogamimobile";
char pass[] = "erinachana";

float totP = 0;
float totPSmart = 0;
float totWFan = 0;


/*
  This example shows how to measure the power consumption
  of devices in 230V electrical system
  or any other system with alternative current
*/

// We have 30 amps version sensor connected to A0 pin of arduino
// Replace with your version if necessary
ACS712 sensor(ACS712_05B, 32);


const int rch1 = 26;
const int rch2 = 25;
bool count;
bool count2;
void setup() {
  Serial.begin(9600);
  pinMode(rch1, OUTPUT);
  pinMode(rch2, OUTPUT);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //  pinMode(ledpin, OUTPUT);
  //  pinMode(ldr, INPUT);
  delay(10);
  Serial.print("connecting to....");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  int wifi_ctr = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // calibrate() method calibrates zero point of sensor,
  // It is not necessary, but may positively affect the accuracy
  // Ensure that no current flows through the sensor at this moment
  // If you are not sure that the current through the sensor will not leak during calibration - comment out this method
  Serial.println("Calibrating... Ensure that no current flows through the sensor at this moment");
  sensor.calibrate();
  Serial.println("Done!");
  digitalWrite(rch1, HIGH);
  digitalWrite(rch2, HIGH);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  String messageTemp2;
  String messageTemp3;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp2 += (char)message[i];
  }

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp3 += (char)message[i];
  }
  //    Control Fan
  if (String(topic) == "esp32/fan") {
    Serial.print("Changing output to ");
    if (messageTemp == "1") {
      Serial.println("on");
      //      digitalWrite(ledpin, HIGH);
      digitalWrite(rch2, LOW);
      count2 = true;
    }
    else if (messageTemp == "0") {
      Serial.println("off");
      //      digitalWrite(ledpin, LOW);
      digitalWrite(rch2, HIGH);
      count2 = false;
    }
  }
  //  LAMP
  if (String(topic) == "esp32/lamp") {
    Serial.print("Changing output to ");
    if (messageTemp == "1") {
      Serial.println("on");
      //      digitalWrite(ledpin, HIGH);
      digitalWrite(rch1, LOW);
      count = true;
    }
    else if (messageTemp == "0") {
      Serial.println("off");
      //      digitalWrite(ledpin, LOW);
      digitalWrite(rch1, HIGH);
      count = false;
    }
  }

  //Auto Control
  if (String(topic) == "esp32/totDayaLamp") {
    int dayaprev = messageTemp3.toInt();
    daya = daya + dayaprev;
    Serial.println("Changing output to " + daya);

  }
  Serial.println();

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/lamp");
      client.subscribe("esp32/fan");
      client.subscribe("esp32/totDayalamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
    }
  }
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  // We use 230V because it is the common standard in European countries
  // Change to your local, if necessary
  float U = 220;
  float I;
  float FU = 9;
  float FI;
  if (count == true) {
    I = sensor.getCurrentAC();
    I *= 1000;
  }

  else {
    I = 0;
  }

  if (count2 == true) {
    FI = sensor.getCurrentDC();
    FI *= 1000;
//    FI += 600;
  }

  else {
    FI = 0;
  }

  // To measure current we need to know the frequency of current
  // By default 50Hz is used, but you can specify desired frequency
  // as first argument to getCurrentAC() method, if necessary


  // To calculate the power we need voltage multiplied by current
  float P = U * I / 1000;

  totPSmart = totPSmart + P;
  totP = totP + P;
  if (totPSmart >= 10000) {
    digitalWrite(rch1, HIGH);
    count = false;
    totPSmart = 0;
  }

  float PF = FU * FI / 1000;
  
  totWFan = totWFan + PF;
  if (totWFan >= 10000) {
    digitalWrite(rch2, HIGH);
    count2 = false;
    totWFan = 0;
  }

  Serial.println(String("I = ") + I + " mA");
  Serial.println(String("P = ") + P + " Watts");
  Serial.println(String("totP = ") + totP + " Watts");
  Serial.println(String("daya = ") + daya + " Watts");
  Serial.println();
  Serial.println(String("FI = ") + FI + " mA");
  Serial.println(String("PF = ") + PF + " Watts");
  Serial.println(String("PF = ") + totWFan + " Watts");
  Serial.println();


  long now = millis();
  if (now - lastMsg > 100) {
    lastMsg = now;

    char tempStringLamp[8];
    char tempStringFan[8];
    char tempStringtotP[8];
    char tempStringtotWFan[8];
    
    dtostrf(P, 1, 2, tempStringLamp);
    dtostrf(PF, 1, 2, tempStringFan);
    dtostrf(totP, 1, 2, tempStringtotP);
    dtostrf(totWFan, 1, 2, tempStringtotWFan);


    client.publish("esp32/dayalamp", tempStringLamp);
    client.publish("esp32/dayafan", tempStringFan);
    client.publish("esp32/totDayaLamp", tempStringtotP);
    client.publish("esp32/totDayaFan", tempStringtotWFan);

    delay(5000);
  }

}
