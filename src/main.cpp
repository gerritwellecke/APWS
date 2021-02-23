#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <SD.h>
#include <SPI.h>

Adafruit_MCP3008 adc;

static const int SD_CS=4;

void setup() {
    Serial.begin(115200);
    while(!Serial);

    adc.begin();
    Serial.println("MCP3008 setup performed.");

    Serial.println("Setup complete");
}

static const int chan=0;

void loop() {
    Serial.print("Channel " + String(chan) + ": ");
    int val{ adc.readADC(chan) };

    Serial.println(val);
    delay(1000);
}

/*
Idea:
*/