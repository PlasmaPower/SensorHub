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
  If we have turned on the datalogger mode (defined it), then run the rest of the function
  */
  #ifdef DATALOGGER
    /*
    The first filenumber we try to create is DATA0.csv 
    */
    int filenumber = 0;
    /*
    Repeat the next 8 lines of code until we have found a suitable file name ofr our data on the SD card
    */
    while (true) {
      /*
      Set filename equal to DATA, then the file number, then .CSV 
      Example "DATA4.CSV 
      */
      String filenameStr = "DATA" + String(filenumber) + ".CSV";
      /*
      Find the length of the file name
      */
      int len = filenameStr.length() + 1;
      /*
      Create a label (or plaque) that will hold the filename
      */
      char filename[len];
      /*
      Write the file name on the label (plaque)
      */
      filenameStr.toCharArray(filename, len);
      /*
      If the the file does not exist yet, we have found a suitable filename and we run the next two lines of code
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
      If the file already exists, the "filenumber++" tries to find another file.
      Move the filenumber up one
      */
      filenumber++;
    }
    /*
    Put the header in the file
    */
    datafile.println("unixTime, dhtHumidity, dhtTemp, thermTemp, solarPanel, photocell, temp");
    /*
    Begin communicating through the wire to the SD card and Real Time Clock
    */
    Wire.begin();
    /*
    Start the Real Time Clock. If that fails, then run the next 3 lines of code
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
    Start getting data from the DHT (Humidity/Temperature Sensor)
    */
    dht.begin();
  #endif
}
/*
This ends the setup function
*/

/*
This is the stableAnalogRead function (a set of commands)
It makes analogReads (getting values from the sensors) more accurate
*/
int stableAnalogRead(int pin) {
  /*
  If we have switched on (defined) ANALOG_READ_STABILITY, execute the next 2 lines of code
  */
  #ifdef ANALOG_READ_STABILITY
    /*
    Read a value from the sensor, then do nothing with the value
    */
    analogRead(pin);
    /*
    Wait for the value in the box ANALOG_READ_STABILITY (a constant) seconds
    */
    delay(ANALOG_READ_STABILITY);
  #endif
  /*
  Read a value from the sensor, and send it back to wherever this was used
  */
  return analogRead(pin);
}
/*
This ends the stableAnalogRead function
*/

/*
This gets the resistance of a sensor on a certain pin
*/
int readResistance(int pin, long resistor) {
  /*
  This uses the voltage divider forumla to get the resistance of the sensor
  */
  return resistor / (1023.0 / stableAnalogRead(pin) - 1);
}

/*
This gets the voltage of a specified pin
*/
float readVoltage(int pin) {
  /*
  analogRead reads a value from the sensor, but scales it from 0-1023
  It instead should be from 0 volts to the constant AREF_VOLTAGE (maximum number of volts)
  To undo the scaling, we use a ratio
  */
  return stableAnalogRead(pin) * AREF_VOLTAGE / 1023.0;
}

/*
If we have switched on the DATALOGGER constand, then enable the putData function
*/
#ifdef DATALOGGER
  /*
  This function puts data in the data file
  */
  void putData(float data[]) {
    /*
    This calculates the length of an array (a set of values by dividing the length of the array (a set of values) by the length of a single value
    */
    int len = sizeof(data)/sizeof(*data);
    /*
    This finds each value in the array (set of values)
    */
    for (int i = 0; i < len; i++) {
      /*
      This prints each value to the datafile
      */
      datafile.print(String(data[i]));
      /*
      We don't want to put a comma at the end
      If this isn't the last data point
      Put a comma after it
      */
      if (i != len - 1) {
        datafile.print(",");
      }
    }
  }
  /*
  This ends the putData function
  */
#endif

/*
This loop function occupies the rest of the code
All of this code runs again and again. 
*/
void loop() {
  /*
  This code gets the humidity from the DHT
  */
  float dhtHumidity = dht.readHumidity();
  /*
  This code gets the temperature from the DHT
  */
  float dhtTemperature = dht.readTemperature();
  /*
  This code gets the temperature from the termistor
  For more information, see http://en.wikipedia.org/wiki/Steinhart%E2%80%93Hart_equation
  This first line gets the resistance of the thermistor, then divides it by the constant in the THERM_THERM_NOMINAL box
  */
  float thermTemperature = readResistance(THERM_PIN, THERM_RESISTOR) / THERM_THERM_NOMINAL;
  /*
  This code sets the temperature equal to the log of the current temperature
  */
  thermTemperature = log(thermTemperature);
  /*
  This code divides the temperature by the number in the THERM_B_COEFFICIENT box
  */
  thermTemperature /= THERM_B_COEFFICIENT;
  /*
  This code adds the inverse of the constant in the THERM_TEMP_NOMINAL box added to 273.15 (negative absolute zero in celcius)
  */
  thermTemperature += 1.0 / (THERM_TEMP_NOMINAL + 273.15);
  /*
  This code inverts the thermistor temperature
  */
  thermTemperature = 1.0 / thermTemperature;
  /*
  This code converts the temperature from Kelvin to Celsius by subtracting 273.15
  */
  thermTemperature -= 273.15;
  /*
  This code reads the voltage on the solar cell
  The arduino measures voltages from 0 volts to 3.3 volts sometimes, but the solar cell is from 0 volts to 5 volts
  So we need to scale it with a ratio
  */
  float solarPanel = readVoltage(SOLAR_PANEL_PIN) * 5 / AREF_VOLTAGE;
  /*
  This code reads the resistance of the photocell
  */
  float photocell = readResistance(PHOTOCELL_PIN, PHOTOCELL_RESISTOR);
  /*
  The temperature sensor will always output 0.5 volts, and then every 0.01 volt after that is one degree celcius
  */
  float temperature = (readVoltage(TEMP_PIN) - 0.5) * 100;
  /*
  This code asks the vibration switch if there is vibration
  */
  int vibration = digitalRead(VIBRATION_SWITCH_PIN) == HIGH;
  /*
  If the datalogger is installed, get the time and log it
  (The Real Time Clock is part of the datalogger)
  */
  #ifdef DATALOGGER
    /*
    This code gets the current time
    */
    DateTime now = RTC.now();
    /*
    The next two lines of code log the time to the datafile
    */
    datafile.print(now.unixtime());
    datafile.print(",");
    /*
    If debug is enabled, log the current date and time to the computer
    */
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
    /*
    Put the data into an array (a list)
    */
    float data[] = {dhtHumidity, dhtTemperature, thermTemperature, solarPanel, photocell, temperature};
    /*
    Put our data into the datafile
    */
    putData(data);
    /*
    Put the vibration sensor data into the datafile
    */
    datafile.print("," + vibration);
    /*
    This ends the line of data
    */
    datafile.println();
  #endif
  /*
  If DEBUG is enabled, then log the values of the sensors to the computer
  */
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
  /*
  Wait the constant LOOP_DELAY seconds, then loop again
  */
  delay(LOOP_DELAY);
}
