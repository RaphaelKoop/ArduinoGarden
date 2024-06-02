#include <Wire.h>
#include <U8g2lib.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

// Initialize software serial
SoftwareSerial mySerial(8, 9); // RX, TX

// Initialize U8g2 library in page buffer mode with a larger font
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

float tempC_inside = 0.0;
float humi_inside = 0.0;
float tempC_outside = 0.0;
float humi_outside = 0.0;
int moisture = 0;
int cycles_per_day = 0;

unsigned long lastCycleChangeTime = 0;
unsigned long elapsedTime = 0;
unsigned long lastSwitchTime = 0;
const unsigned long switchInterval = 5000;
bool showInside = true;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600); // Initialize software serial

  // Initialize U8g2 library
  if (!u8g2.begin()) {
    Serial.println(F("Failed to initialize OLED"));
  } else {
    Serial.println(F("OLED initialized"));
  }
  u8g2.setFont(u8g2_font_ncenB08_tr); // Choose a larger font

  Serial.println(F("Setup complete"));
}

void loop() {
  static char received_message[64];
  static int message_index = 0;

  // Print a loop entry message every 5 seconds for debugging
  if (millis() - lastSwitchTime >= 5000) {
    Serial.println(F("Entering loop"));
    lastSwitchTime = millis();
  }

  // Check for data from SoftwareSerial
  while (mySerial.available() > 0) {
    char c = mySerial.read();
    Serial.print(F("Received character: "));
    Serial.println(c); // Print each character as it is received

    // Only handle valid message characters
    if (c != '\r' && c != '\n') {
      received_message[message_index++] = c;
    }

    // Check for end of message
    if (c == '\n' || message_index >= sizeof(received_message) - 1) {
      received_message[message_index] = '\0'; // Null-terminate the string
      Serial.print(F("Full message received: "));
      Serial.println(received_message); // Debugging line
      parseMessage(received_message);
      message_index = 0; // Reset for the next message
    }
  }

  // Update display periodically
  if (millis() - lastSwitchTime >= switchInterval) {
    showInside = !showInside;
    updateDisplay();
  }
}

void parseMessage(char* message) {
  Serial.print(F("Parsing message: "));
  Serial.println(message);

  char *token = strtok(message, ";");
  int index = 0;
  int new_cycles_per_day = cycles_per_day;

  // Parse each token and assign to the respective variables
  while (token != NULL && index < 6) {
    Serial.print(F("Token "));
    Serial.print(index);
    Serial.print(F(": "));
    Serial.println(token);

    switch (index) {
      case 0: tempC_inside = atof(token); break;
      case 1: humi_inside = atof(token); break;
      case 2: tempC_outside = atof(token); break;
      case 3: humi_outside = atof(token); break;
      case 4: moisture = atoi(token); break;
      case 5: new_cycles_per_day = atoi(token); break;
    }
    token = strtok(NULL, ";");
    index++;
  }

  if (index == 6) {
    Serial.print(F("Temperature Inside: "));
    Serial.println(tempC_inside);
    Serial.print(F("Humidity Inside: "));
    Serial.println(humi_inside);
    Serial.print(F("Temperature Outside: "));
    Serial.println(tempC_outside);
    Serial.print(F("Humidity Outside: "));
    Serial.println(humi_outside);
    Serial.print(F("Soil Moisture: "));
    Serial.println(moisture);
    Serial.print(F("Cycles Per Day: "));
    Serial.println(new_cycles_per_day);

    // Update timer if cycles_per_day changes
    if (new_cycles_per_day != cycles_per_day) {
      cycles_per_day = new_cycles_per_day;
      lastCycleChangeTime = millis();
    }

    elapsedTime = millis() - lastCycleChangeTime;

    updateDisplay();
  } else {
    Serial.println(F("Error: Incomplete data received"));
  }
}

void updateDisplay() {
  Serial.println(F("Updating display...")); // Debugging line

  u8g2.clearBuffer(); // Clear the internal buffer

  if (showInside) {
    u8g2.setCursor(0, 12); // Adjust cursor position for larger font
    u8g2.print(F("In Temp: "));
    u8g2.print(tempC_inside);
    u8g2.print(F(" C"));

    u8g2.setCursor(0, 24); // Adjust cursor position for larger font
    u8g2.print(F("In Hum: "));
    u8g2.print(humi_inside);
    u8g2.print(F(" %"));
  } else {
    u8g2.setCursor(0, 12); // Adjust cursor position for larger font
    u8g2.print(F("Out Temp: "));
    u8g2.print(tempC_outside);
    u8g2.print(F(" C"));

    u8g2.setCursor(0, 24); // Adjust cursor position for larger font
    u8g2.print(F("Out Hum: "));
    u8g2.print(humi_outside);
    u8g2.print(F(" %"));
  }

  u8g2.setCursor(0, 36); // Adjust cursor position for larger font
  u8g2.print(F("Moisture: "));
  u8g2.print(moisture);

  // Convert elapsed time to hours, minutes, and seconds
  unsigned long seconds = elapsedTime / 1000;
  unsigned int minutes = seconds / 60;
  unsigned int hours = minutes / 60;
  minutes = minutes % 60;
  seconds = seconds % 60;

  u8g2.setCursor(0, 48); // Adjust cursor position for elapsed time
  u8g2.print(F("Time: "));
  u8g2.print(hours);
  u8g2.print(F(":"));
  if (minutes < 10) u8g2.print(F("0")); // Add leading zero if needed
  u8g2.print(minutes);
  u8g2.print(F(":"));
  if (seconds < 10) u8g2.print(F("0")); // Add leading zero if needed
  u8g2.print(seconds);

  if (cycles_per_day == 1) {
    u8g2.drawDisc(120, 36, 6, U8G2_DRAW_ALL); // Increased radius
  } else {
    u8g2.drawCircle(120, 36, 6, U8G2_DRAW_ALL); // Increased radius
  }

  u8g2.sendBuffer(); // Send buffer to display

  Serial.println(F("Display updated")); // Debugging line
}