/* VERSION 0.1

Author: Gerrit Wellecke

This is a minimal version of the code that runs fully offline.
WiFi functionality is on the horizon, but I recently redid my home server and I have no
MQTT running right now. This will also likely end up in a home assistant and not NodeRED
as I once used it.
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>

#define debug // remove this in "production" system

// Chip Select pin of shift register
static const int CS_shiftreg{ 4 };
// Output Enable of shift register
static const int OEpump{ 5 };
// number of attached pumps
static const int num_channels{ 4 };

// duration in [s] of water flowing per watering
static const int pump_delay{ 3 };
// how many wake intervals are skipped before checking plants again
static const int wake_skip{ 7 }; //TODO set this to water every 5-7 days
// duration of deep sleep
static const unsigned long long sleep_duration{ 6000000 }; // 6 Seconds
//static const unsigned long long sleep_duration{ ESP.deepSleepMax() };

// amount of waterings per wake cycle, i.e. increments of `pump_delay`[s] of water flow
static const int water_levels[4] { 2, 4, 6, 8 }; //TODO set number of "waterings"

// function declarations
void writeIntervalAndSleep(uint wake_interval);
void waterPlants(
    const int pump_delay,
    const int water_levels[],
    const int num_channels
);


void setup() {
    // read current wake interval from RTC memory
    uint wake_interval;
    bool read_rtc = ESP.rtcUserMemoryRead(0, &wake_interval, sizeof(wake_interval));

#ifdef debug
    // start serial debug
    Serial.begin(115200);
    while(!Serial);
    Serial.println("Counter is: " + String(wake_interval / 1000000) + "s");
    Serial.println("Maximum is: " + String(ESP.deepSleepMax()/ 1000000) + "s");
#endif

    // we only want to do something every N times the MCU wakes up
    if (wake_interval % wake_skip != 0) {
#ifdef debug
        Serial.println("Going to sleep - counter is only: " + String(wake_interval));
#endif
        writeIntervalAndSleep(wake_interval);
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


    waterPlants(pump_delay, water_levels, num_channels);


#ifdef debug
    Serial.println("going to sleep for: " + String(sleep_duration / 1000000) + "s");
#endif
    writeIntervalAndSleep(wake_interval);
}

// loop will stay empty - we only run single shots between deep sleeps
void loop() {}

void writeIntervalAndSleep(uint wake_interval) {
    // increment counter
    ++wake_interval;
    // write to RTC memory
    ESP.rtcUserMemoryWrite(0, &wake_interval, sizeof(wake_interval));
    // sleep as long as possible
    ESP.deepSleep(sleep_duration, WAKE_RF_DISABLED);
}

void waterPlants(const int pump_delay, const int water_levels[], const int num_channels)
{
    // turn on pumps as needed
    byte pumps{ 0 };
    int currentWateringIdx{ 0 };

    // water as long as originally requested
    do {
        // determine status for each pump
        for (int i=0; i<num_channels; i++) {
            if (currentWateringIdx <= water_levels[i]) // use time instead of readings
                bitWrite(pumps, i, 1);
            else
                bitWrite(pumps, i, 0);
        }

        /*** write to shift register ***/
        // activate shift register selection pin
        digitalWrite(CS_shiftreg, LOW);
        // transaction
        SPI.beginTransaction(SPISettings(14000000, LSBFIRST, SPI_MODE0));
        SPI.transfer(pumps);
        SPI.endTransaction();
        // deactivate shift register selection pin
        digitalWrite(CS_shiftreg, HIGH);

        // turn pumps on for certain time
        digitalWrite(OEpump, LOW);
#ifdef debug
        Serial.println("Pumps on!");
#endif
        // water for `pump_delay` seconds
        delay(pump_delay * 1000);

        digitalWrite(OEpump, HIGH);
#ifdef debug
        Serial.println("Pumps off!");
#endif
        ++currentWateringIdx;

        // pause 1s before next iteration
        delay(1000);
    } while (pumps != 0);
}
