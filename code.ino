/*
The #include means that the code is using other code (libraries) written by other people to do common tasks, in this case talk to the sensors
*/
#include <SPI.h>
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <Wire.h>
#include <DHT.h>

/*
#define defines a constant
Later, we check if these first two constants are defined
If they are, we enable certain features
Think of the first two as off and on statements

DEBUG: Enables logging debug information to the computer
DATALOGGER: Enables putting data in the SD card
*/
#define DEBUG
#define DATALOGGER

/*
These constants are tied to values
They are kept the same throughout the code
Think of these as boxes with values in them
*/
/*
This first #define defines the pin that the vibration switch is plugged into
*/
#define VIBRATION_SWITCH_PIN 6
/*
This #define defines the pin that the DHT is plugged into
*/
#define DHT_PIN 5
/*
This #define defines the type of the DHT
*/
#define DHT_TYPE DHT22

/*
This #define defines the pin that the analog temperature sensor is plugged into
*/
#define TEMP_PIN 0
/*
This #define defines the pin that the photocell is plugged into
*/
#define PHOTOCELL_PIN 1
/*
This #define defines the pin that the thermistor is plugged into
*/
#define THERM_PIN 2
/*
This #define defines the pin that the solar panel is plugged into
*/
#define SOLAR_PANEL_PIN 3

/*
This #define defines the resistance of resistor b
*/
#define THERM_RESISTOR 10000
/*
This #define defines the resistance of resistor c
*/
#define PHOTOCELL_RESISTOR 10000

/*
This is the coefficent in the Steinhart-hart equation
(an equation used to find the temperature from a resistance)
that we need to calibrate
Usually from 3000-4000
*/
#define THERM_B_COEFFICIENT 3950
/*
These state that at 25 degrees celcius, the resistance is 10K ohms
*/
#define THERM_THERM_NOMINAL 10000
#define THERM_TEMP_NOMINAL 25

/*
This defines the voltage running through the circut
*/
#define AREF_VOLTAGE 5

/*
This defines the amount of time it takes to preform a stable conversation (between the arduino and sensors)
Devleoping a stable conversation takes more time
*/
#define ANALOG_READ_STABILITY 10
/*
This is how often the arduino talks with the sensors in milliseconds
It then logs the data to the SD card
*/
#define LOOP_DELAY 30000

/*
This tells the code that a DHT is plugged into port DHT_PIN and the DHT is of type DHT_TYPE
*/
DHT dht(DHT_PIN, DHT_TYPE);
/*
This #ifdef checks if the DATALOGGER constant is switched on (defined) and runs the next 2 lines of code (not comments) if it is
*/
#ifdef DATALOGGER
  /*
  This tells the arduino to create a file editor
  */
  File datafile;
  /*
  This tells the arduino that it has a Real Time Clock
  */
  RTC_DS1307 RTC;
#endif

/*
The void setup() is a function (a list of commands to execute)
This particular function is executed (the commands are run in order) when the arduino starts up
*/
void setup() {
  /*
  If DEBUG is defined (the constant is switched on, if the first define statement is not commented out), then enable logging to the computer
  */
  #ifdef DEBUG
    /*
    Enables logging to the computer
    */
    Serial.begin(9600);
  #endif
  /*
  This tells our code that pins 10, 11, 12, 13, and the vibration switch pin are being by the arduino
  Output means the arduino is talking to the sensors at these pins
  Input means the arduino is listening to sensors at these pins
  */
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(VIBRATION_SWITCH_PIN, INPUT);
  /*
  If we are using a datalogger, run the next 5 lines of code
  */
  #ifdef DATALOGGER
    /*
    If the SD failes to begin communicating on pins 10, 11, 12, 13, then run the next 3 lines of code
    */
    if (!SD.begin(10, 11, 12, 13)) {
      /*
      The next line of code (not the comment) runs if debug is switched on and this code is running
      */
       #ifdef DEBUG
         /*
         If this code is executed, then the arduino must be talking wrong to the SD card
         This could be that the SD card is not using that pin or not connected to it
         */
         Serial.println("Failed to initialize SD card");
       #endif
    }
  #endif
  
  /*
  If we have turned on the datalogger (defined it), then run the rest of the function
  */
  #ifdef DATALOGGER
    /*
    The first filenumber we try is 0
    */
    int filenumber = 0;
    /*
    Repeat the next 8 lines of code until we have found a suitable file name
    */
    while (true) {
      /*
      Set filenameStr equal to DATA, then the file number, then .CSV
      */
      String filenameStr = "DATA" + String(filenumber) + ".CSV";
      /*
      Retrieve the length of the file name
      */
      int len = filenameStr.length() + 1;
      /*
      Create a spot to put the file name
      */
      char filename[len];
      /*
      Put the filename in the spot to put the filename
      */
      filenameStr.toCharArray(filename, len);
      /*
      If the the file does not exist yet, we have found a suitable filename
      Run the next two lines of code
      */
      if (!SD.exists(filename)) {
        /*
        In our file editor, open the unused file
        */
        datafile = SD.open(filename, FILE_WRITE);
        /*
        Stop trying to find a filename, we have found one
        */
        break;
      }
      /*
      Move the filenumber up one
      */
      filenumber++;
    }
    /*
    Put the header in the file
    */
    datafile.println("unixTime, dhtHumidity, dhtTemp, thermTemp, solarPanel, photocell, temp");
    /*
    Begin communicating through the wire
    */
    Wire.begin();
    /*
    Initialize the Real Time Clock. If that fails, then run the next 3 lines of code
    */
    if (!RTC.begin()) {
      /*
      If debug is enabled, print an error message to the computer
      */
      #ifdef DEBUG
        Serial.println("Failed to initialize Real Time Clock");
      #endif
    }
    /*
    Start getting data from the DHT
    */
    dht.begin();
  #endif
}
/*
This ends the setup function
*/

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
