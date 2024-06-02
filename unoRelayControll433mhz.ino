#include "DHT.h"
#include <RH_ASK.h>
#include <SPI.h>

#define DHT22_inside_PIN 2
#define DHT22_outside_PIN 3
#define AOUT_PIN A0
#define RELAY_PIN 7
#define DATA_PIN 12 

// Control variables
const unsigned long interval = 5000; // 5 seconds interval for sensor readings
const unsigned long dayInterval = 86400000; // 24 hours interval
const unsigned long twoDayInterval = 172800000; // 48 hours interval
const unsigned long maxRelayOpenTime = 30000; // Maximum time the relay can stay open (30 seconds)
const unsigned long minRelayOpenTime = 20000; // Minimum time the relay should stay open (20 seconds)
const unsigned long relayWaitTime = 60000; // Time to wait after the relay turns off (1 minute)
const int openMoistureThreshold = 420; // Moisture level to open the relay
const int closeMoistureThreshold = 350; // Moisture level to close the relay

// DHT sensors
DHT dht22_inside(DHT22_inside_PIN, DHT22);
DHT dht22_outside(DHT22_outside_PIN, DHT22);

// RF driver
RH_ASK rf_driver;

int cycles_per_day = 0;
unsigned long lastTime = 0;
unsigned long relay_timer_start = 0;
unsigned long lastRelayActivation = 0; // Track the last time the relay was activated
unsigned long wait_timer_start = 0; // Track the start of the waiting period
int wait_cycles = 0; // Track the number of wait cycles

enum State { CHECK_MOISTURE, RUN_RELAY, WAIT, FINAL_WAIT };
State currentState = CHECK_MOISTURE;

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  Serial.begin(9600);
  dht22_inside.begin();
  dht22_outside.begin(); 
  rf_driver.init();
  lastRelayActivation = millis(); // Initialize the last relay activation time
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;

    int moisture = analogRead(AOUT_PIN);
    float humi_inside  = dht22_inside.readHumidity();
    float tempC_inside = dht22_inside.readTemperature();
    float humi_outside  = dht22_outside.readHumidity();
    float tempC_outside = dht22_outside.readTemperature();

    // Check if 48 hours have passed since the last relay activation
    if (currentTime - lastRelayActivation >= twoDayInterval) {
      cycles_per_day = 0;
      currentState = RUN_RELAY;
    }

    switch (currentState) {
      case CHECK_MOISTURE:
        if (moisture > openMoistureThreshold && cycles_per_day == 0) {
          digitalWrite(RELAY_PIN, HIGH);
          relay_timer_start = currentTime;
          currentState = RUN_RELAY;
        }
        break;

      case RUN_RELAY:
        if ((currentTime - relay_timer_start >= maxRelayOpenTime) || 
            (moisture <= closeMoistureThreshold && currentTime - relay_timer_start >= minRelayOpenTime)) {
          digitalWrite(RELAY_PIN, LOW);
          wait_timer_start = currentTime;
          currentState = WAIT;
        }
        break;

      case WAIT:
        if (currentTime - wait_timer_start >= relayWaitTime) {
          if (moisture > openMoistureThreshold && wait_cycles < 2) {
            digitalWrite(RELAY_PIN, HIGH);
            relay_timer_start = currentTime;
            currentState = RUN_RELAY;
            wait_cycles++;
          } else {
            currentState = FINAL_WAIT;
            wait_timer_start = currentTime;
          }
        }
        break;

      case FINAL_WAIT:
        if (currentTime - wait_timer_start >= relayWaitTime) {
          cycles_per_day = 1;
          lastRelayActivation = currentTime; // Update the last relay activation time
          currentState = CHECK_MOISTURE;
          wait_cycles = 0; // Reset the wait cycles
        }
        break;
    }

    // Reset cycles_per_day after a full day
    if (currentTime - lastRelayActivation >= dayInterval) {
      cycles_per_day = 0;
    }

    String radio_msg = String(tempC_inside) + ";" + String(humi_inside) + ";" + String(tempC_outside) + ";" + String(humi_outside) + ";" + String(moisture) + ";" + String(cycles_per_day);
    rf_driver.send((uint8_t*)radio_msg.c_str(), radio_msg.length() + 1);
    rf_driver.waitPacketSent();

    Serial.print("Temperature Inside: " + String(tempC_inside) + " ");
    Serial.print("Humidity Inside: " + String(humi_inside) + " ");
    Serial.print("Temperature Outside: " + String(tempC_outside) + " ");
    Serial.print("Humidity Outside: " + String(humi_outside) + " ");
    Serial.print("Timer: " + String(currentTime) + " ");
    Serial.println("Soil Moisture: " + String(moisture) + " Cycles: " + String(cycles_per_day));
  }
}
