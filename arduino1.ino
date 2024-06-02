#include <RH_ASK.h>
#include <SPI.h>
#include <SoftwareSerial.h>

// Define software serial pins
SoftwareSerial mySerial(8, 9); // RX, TX

RH_ASK rf_driver;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600); // Initialize software serial

  if (!rf_driver.init()) {
    Serial.println("init failed");
  } else {
    Serial.println("RF Receiver Ready");
  }
}

void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);

  if (rf_driver.recv(buf, &buflen)) {
    buf[buflen] = '\0'; // Null-terminate the string
    Serial.println((char*)buf); // Print to Serial Monitor
    mySerial.println((char*)buf); // Send the received message over SoftwareSerial
  }
}
