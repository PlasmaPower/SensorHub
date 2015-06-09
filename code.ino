#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Wire.h>
#include <DHT.h>

#define DEBUG
#define DATALOGGER

#define VIBRATION_SWITCH_PIN 2
#define DHT_PIN 3
#define DHT_TYPE DHT22
#define CHIP_SELECT_PIN 10

#define THERM_PIN 0
#define SOLAR_PANEL_PIN 1
#define PHOTOCELL_PIN 2
#define TEMP_PIN 3

#define THERM_RESISTOR 10000
#define SOLAR_PANEL_RESISTOR 10000
#define PHOTOCELL_RESISTOR 10000

#define THERM_B_COEFFICIENT 3950 // From 3000 - 4000 usually, testing possibly required
#define THERM_THERM_NOMINAL 10000
#define THERM_TEMP_NOMINAL 25 // 10000 ohms @ 25 degrees celsius

#define AREF_VOLTAGE 3.3

#define ANALOG_READ_STABILITY 10
#define LOOP_DELAY 30000

DHT dht(DHT_PIN, DHT_TYPE);
#ifdef DATALOGGER
  File datafile;
  RTC_DS1307 RTC;
#endif

void setup() {
  #ifdef DEBUG // If DEBUG is defined (if the first define statement is not commented out), then enable logging to the computer
    Serial.begin(9600);
  #endif
  pinMode(CHIP_SELECT_PIN, OUTPUT);
  pinMode(VIBRATION_SWITCH_PIN, INPUT);
  #ifdef DATALOGGER
    if (!SD.begin(CHIP_SELECT_PIN)) {
       #ifdef DEBUG
         Serial.println("Failed to initialize SD card");
       #endif
    }
  #endif
  
  #ifdef DATALOGGER
    int filenumber = 0;
    while (true) { // Find a filename to put the data in
      String filenameStr = "DATA" + String(filenumber) + ".CSV";
      int len = filenameStr.length();
      char filename[len];
      filenameStr.toCharArray(filename, len);
      if (!SD.exists(filename)) {
        // This filename will work
        datafile = SD.open(filename, FILE_WRITE);
        break;
      }
      filenumber++;
    }
    datafile.println("unixTime, dhtHumidity, dhtTemp, thermTemp, solarPanel, photocell, temp");
    Wire.begin();
    if (!RTC.begin()) {
      #ifdef DEBUG
        Serial.println("Failed to initialize Real Time Clock");
      #endif
    }
    dht.begin();
  #endif
  analogReference(EXTERNAL);
}

int stableAnalogRead(int pin) {
 #ifdef ANALOG_READ_STABILITY // From what I've read online, this helps make the next analogRead more accurate
   analogRead(pin);
   delay(ANALOG_READ_STABILITY);
 #endif
 return analogRead(pin);
}

int readResistance(int pin, long resistor) {
 return resistor / (1023.0 / stableAnalogRead(pin) - 1); // http://en.wikipedia.org/wiki/Voltage_divider
}

float readVoltage(int pin) {
  return stableAnalogRead(pin) * AREF_VOLTAGE / 1023.0; // The arduino scales analog inputs from 0-AREF volts to 0 - 1023 for better precision
}

#ifdef DATALOGGER
  void putData(float data[]) {
    int len = sizeof(data)/sizeof(*data); // This calculates the length of the array by dividing the size of the array by the size of a float
    for (int i = 0; i < len; i++) {
      datafile.print(String(data[i]));
      if (i != len - 1) {
        datafile.print(",");
      }
    }
  }
#endif

void loop() {
  float dhtHumidity = dht.readHumidity();
  float dhtTemperature = dht.readTemperature();
  float thermTemperature = readResistance(THERM_PIN, THERM_RESISTOR) / THERM_THERM_NOMINAL; // http://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation
  thermTemperature = log(thermTemperature);
  thermTemperature /= THERM_B_COEFFICIENT;
  thermTemperature += 1.0 / (THERM_TEMP_NOMINAL + 273.15);
  thermTemperature = 1.0 / thermTemperature;
  thermTemperature -= 273.15; // From Kelvin to Celsius
  float solarPanel = readVoltage(SOLAR_PANEL_PIN) * 5 / AREF_VOLTAGE; // Input has a voltage divider lowering it from 0-5V to 0-3.3V
  float photocell = readResistance(PHOTOCELL_PIN, PHOTOCELL_RESISTOR);
  float temperature = (readVoltage(TEMP_PIN) - 0.5) * 100; // The voltage has an offset of 0.5V (there is always 0.5V flowing through) and the rest is 1/100th the temperature
  int vibration = (digitalRead(VIBRATION_SWITCH_PIN) == HIGH) ? 1 : 0; // 1 if there is vibration, 0 if there isn't
  #ifdef DATALOGGER
    DateTime now = RTC.now();
    datafile.print(now.unixtime());
    datafile.print(",");
    #ifdef DEBUG
      Serial.print(now.year(), DEC);
      Serial.print("/");
      Serial.print(now.month(), DEC);
      Serial.print("/");
      Serial.print(now.day(), DEC);
      Serial.print(" ");
      Serial.print(now.hour(), DEC);
      Serial.print(":");
      Serial.print(now.minute(), DEC);
      Serial.print(":");
      Serial.print(now.second(), DEC);
      Serial.print(" - ");
    #endif
    float data[] = {dhtHumidity, dhtTemperature, thermTemperature, solarPanel, photocell, temperature};
    putData(data);
    datafile.print("," + vibration);
    datafile.println();
  #endif
  #ifdef DEBUG
    Serial.print("dhtHumidity: ");
    Serial.print(String(dhtHumidity));
    Serial.print(" dhtTemperature: ");
    Serial.print(String(dhtTemperature));
    Serial.print(" thermTemperature: ");
    Serial.print(String(thermTemperature));
    Serial.print(" solarPanel: ");
    Serial.print(String(solarPanel));
    Serial.print(" photocell: ");
    Serial.print(String(photocell));
    Serial.print(" temperature: ");
    Serial.print(String(temperature));
    Serial.print(" vibration: ");
    if (vibration) {
      Serial.print("yes");
    } else {
      Serial.print("no");
    }
    Serial.println();
  #endif
  delay(LOOP_DELAY);
}
