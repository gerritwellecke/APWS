#include "credentials.h"
#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SD.h>
#include <SPI.h>

// reconfigure the built-in ADC
ADC_MODE(ADC_VCC);

// function declarations
void callback(String topic, byte* message, uint length);
void reconnect();

// MQTT
const char* mqtt_server("172.18.100.106");
WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, callback, espClient);

//Adafruit_MCP3008 adc;

// global variables
static const int SD_CS=4;

void setup() {
    // keep WiFi off while we're not transmitting
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);

    Serial.begin(115200);
    while(!Serial);

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);

    int VCC{ ESP.getVcc() };
    Serial.println("Vcc = " + String(VCC));

    // turn WiFi on and transmit data 
    WiFi.forceSleepWake();
    delay(1);
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

    Serial.print("ESP ");
    Serial.println(WiFi.macAddress());
    Serial.print("Connecting to ");
    Serial.println(SSID);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi connected - ESP IP address: ");
    Serial.println(WiFi.localIP());

    // MQTT connection
    if (!client.connected())
        reconnect();

    // publish value to MQTT server
    if (client.publish("room/plants", String(VCC).c_str()))
        Serial.println("published");
    for (int i=0; i<10; i++) {
        client.loop();
        delay(100);
    }

    client.disconnect();
    if (!client.connected())
        Serial.println("disconnected");

    delay(1000);
    // disconnect from wifi and go to deep sleep
    WiFi.disconnect(true);
    delay(1);
    Serial.println("going to sleep");
    ESP.deepSleep(6e6, WAKE_RF_DISABLED);
    //adc.begin();
    //Serial.println("MCP3008 setup performed.");

    //Serial.println("Setup complete");
}

void loop() {
    /*
    Serial.print("Channel " + String(chan) + ": ");
    int val{ adc.readADC(chan) };

    Serial.println(val);
    delay(1000);
    */
}

void callback(String topic, byte* message, uint length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (uint i=0; i<length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    if (topic=="room/lamp") {
        Serial.print("Changing room lamp to ");
        if (messageTemp == "on") {
            digitalWrite(2, LOW);
            Serial.println("ON");
        }
        else if (messageTemp == "off") {
            digitalWrite(2, HIGH);
            Serial.println("OFF");
        }
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.println("Attempting MQTT connection...");

        if (client.connect("ESP8266APWS")) {
            Serial.println("connected");
            client.subscribe("room/lamp");
        }
        else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" trying again in 5 seconds");
            delay(5000);
        }
    }
}

/*
Idea: 
- read moisture every hour 
- once a week water the plant, unless critical moisture value is exceeded
- if critical dryness is exceeded, water immediately
- keep track of used volume of water (stop watering when no water left)
*/