#include "credentials.h"
#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <ESP8266WiFi.h>
//#include <PubSubClient.h>
#include <SPI.h>

#define debug

Adafruit_MCP3008 adc;

// Chip Select pin of shift register
static const int CS_shiftreg{ 4 };
// Output Enable of shift register
static const int OEpump{ 5 };
// number of attached pumps
static const int num_channels{ 4 };


// duration of water flowing
static const int pump_delay{ 3 };
// how many wake intervals are skipped before checking plants again
static const int wake_skip{ 7 }; //TODO set this!
// duration of deep sleep
static const unsigned long long sleep_duration{ 6000000 }; // 6 Seconds
//static const unsigned long long sleep_duration{ ESP.deepSleepMax() };

// critical water level per plant in percent
static int water_levels[4] { 20, 40, 60, 80 }; //TODO calibrate this somehow

void setup() {
    // read current wake interval
    uint wake_interval;
    bool read_rtc = ESP.rtcUserMemoryRead(0, &wake_interval, sizeof(wake_interval));

#ifdef debug
    // start serial debug
    Serial.begin(115200);
    while(!Serial);
    Serial.println("Counter is: " + String(wake_interval));
#endif

    // make sure that counter doesn't go through the roof
    if (wake_interval > wake_skip)
        wake_interval = 0;

    // we only want to do something every N times the MCU wakes up
    if (wake_interval % wake_skip != 0) {
#ifdef debug
        Serial.println("Going to sleep - counter is only: " + String(wake_interval));
#endif
        // increment counter
        ++wake_interval;
        // write to RTC memory
        ESP.rtcUserMemoryWrite(0, &wake_interval, sizeof(wake_interval));
        ESP.deepSleep(sleep_duration, WAKE_RF_DISABLED);
    }
    else
        wake_interval = 0;

    // keep WiFi off while we're not transmitting
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);

    // disable output for the pump driver
    pinMode(OEpump, OUTPUT);
    digitalWrite(OEpump, HIGH);

    // CS for shift register
    pinMode(CS_shiftreg, OUTPUT);
    digitalWrite(CS_shiftreg, HIGH);


    // read ADC channels 0-3
    adc.begin(15); //TODO this should not be hard coded

    int readings[num_channels]{};
    for (int i=0; i<4; i++) {
        readings[i] = adc.readADC(i) * 100 / 1023;
#ifdef debug
        Serial.println("Channel " + String(i) + ": " + String(readings[i]));
#endif
    }

    // turn on pumps as needed
    byte pumps{ 0 };
    // determine status for each pump
    for (int i=0; i<num_channels; i++) {
        if (readings[i] <= water_levels[i])
            bitWrite(pumps, i, 1);
        else
            bitWrite(pumps, i, 0);
    }

    /*** write to shift register ***/
    // activate shift register
    digitalWrite(CS_shiftreg, LOW);
    // transaction
    SPI.beginTransaction(SPISettings(14000000, LSBFIRST, SPI_MODE0));
    SPI.transfer(pumps);
    SPI.endTransaction();
    // deactivate shift register
    digitalWrite(CS_shiftreg, HIGH);

    // turn pumps on for certain time
    digitalWrite(OEpump, LOW);
#ifdef debug
    Serial.println("Pumps on!");
#endif
    delay(pump_delay * 1000);
    digitalWrite(OEpump, HIGH);
#ifdef debug
    Serial.println("Pumps off!");
#endif

#ifdef MQTT
    // turn WiFi on and transmit data
    WiFi.forceSleepWake();
    delay(1);
    WiFi.persistent(false); // prevent credentials from being written to EEPROM
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);

#ifdef debug
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
#endif

    // disconnect from wifi and go to deep sleep
    WiFi.disconnect(true);
#endif

    delay(1);

#ifdef debug
    Serial.println("going to sleep for: " + String(sleep_duration));
#endif
    // TODO this is duplicate code
    // increment counter
    ++wake_interval;
    // write to RTC memory
    ESP.rtcUserMemoryWrite(0, &wake_interval, sizeof(wake_interval));
    // sleep as long as possible
    ESP.deepSleep(sleep_duration, WAKE_RF_DISABLED);
}

// loop will stay empty - we only run single shots between deep sleeps
void loop() {}
