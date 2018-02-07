/*-----( Import needed libraries )-----*/
#include "Wire.h"
#include <Servo.h>
#include <elapsedMillis.h>
#include <MPR121.h>
#include <OneWire.h>
#include <DallasTemperature.h>  //Get DallasTemperature Library here:  http://milesburton.com/Main_Page?title=Dallas_Temperature_Control_Library

/*-----( Declare Constants and Pin Numbers )-----*/
#define ONE_WIRE_BUS_PIN 3
// Device resolution
#define TEMP_9BIT  9 //  9 bit
#define TEMP_10BIT 10 // 10 bit
#define TEMP_11BIT 11 // 11 bit
#define TEMP_12BIT 12 // 12 bit

const int pin_SERVO_HOOK = 13;              // Hook servo-motor is physically plugged on pin 13
const int pin_SERVO_COCCIX_SPHINCTER = 10;  // Left Sphincter servo-motor is physically plugged on pin 10
const int pin_SERVO_PERINEUM_SPHINCTER = 7; // Right Sphincter servo-motor is physically plugged on pin 7

const int pin_RELAY_HOOK = A0;              // Relay attached to Hook servo-motor power supply plugged on pin A0
const int pin_RELAY_SPHINCTERS = A1;        // Relay attached to the 2 Sphincter servo-motors power supply plugged on pin A1
const int pin_IRQ = 4;

boolean touchStates;                        // to keep track of the previous touch state
const int openhook = 165;
const int closehook = 40;
const int initialCoccixAngle = 120;
const int initialPerineumAngle = 50;
int servo_angle = openhook;                  // initial position of thee servo motor

// TIMING
elapsedMillis timeElapsed;                   // declare global if you don't want it reset every time loop runs
elapsedMillis timeElapsedTempSensors;        // declare global if you don't want it reset every time loop runs
unsigned int interval = 3000;                // delay in milliseconds between the pressure measurement
unsigned int intervalTempSensors = 30000;    // delay in milliseconds between the pressure measurement

/*-----( Declare objects )-----*/
Servo Servo_Hook;   // create servo object to control a servo
Servo Servo_CoccixSphincter;
Servo Servo_PerineumSphincter;

// SERIAL INPUT
String readString;

// Pass our oneWire reference to Dallas Temperature.
OneWire oneWire(ONE_WIRE_BUS_PIN);  // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);

/*-----( Declare Variables )-----*/
// Assign the addresses of your 1-Wire temp sensors.
DeviceAddress Sensor1 = { 0x28, 0x3A, 0x43, 0x82, 0x09, 0x00, 0x00, 0xCB };
DeviceAddress Sensor2 = { 0x28, 0x52, 0x05, 0x82, 0x09, 0x00, 0x00, 0xDC };
DeviceAddress Sensor3 = { 0x28, 0x90, 0xEE, 0x81, 0x09, 0x00, 0x00, 0x0F };
DeviceAddress Sensor4 = { 0x28, 0x1D, 0xEF, 0x80, 0x09, 0x00, 0x00, 0xED };
DeviceAddress Sensor5 = { 0x28, 0xFA, 0xED, 0x80, 0x09, 0x00, 0x00, 0xC2 };
DeviceAddress Sensor6 = { 0x28, 0x05, 0xB4, 0x81, 0x09, 0x00, 0x00, 0x7D };
DeviceAddress* ArrayOfSensorAddress[6] = {&Sensor1,&Sensor2,&Sensor3,&Sensor4,&Sensor5,&Sensor6};



void setup() 
{
  pinMode(pin_IRQ, INPUT);
  digitalWrite(pin_IRQ, HIGH);   // Enable pullup resistor

//*************************************************
// Setup Relays pinout
//*************************************************  
  pinMode(pin_RELAY_HOOK,OUTPUT);
  pinMode(pin_RELAY_SPHINCTERS,OUTPUT);
  set_relay(pin_RELAY_HOOK, false);       // Powers OFF Hook servo
  set_relay(pin_RELAY_SPHINCTERS, false); // Powers OFF Sphincter servos

//*************************************************
// Setup Servo-motors
//*************************************************  
  Servo_Hook.attach(pin_SERVO_HOOK);                            // Attaches the Hook servo-motor pin to the servo object  
  Servo_CoccixSphincter.attach(pin_SERVO_COCCIX_SPHINCTER);     // Attaches the Coccix Sphincter servo-motor on pin 9 to the servo object
  Servo_PerineumSphincter.attach(pin_SERVO_PERINEUM_SPHINCTER); // Attaches the Perineum Sphincter servo-motor on pin 10 to the servo object

  Servo_Hook.write(openhook);                                   // Put the Hook servo in the initial position
  Serial.println(F("Hook servo-motor setup: complete"));
  Servo_CoccixSphincter.write(initialCoccixAngle);              // Put the Left Sphincter servo in the initial position
  Serial.println(F("Coccix Sphincter servo-motor setup: complete"));   
  Servo_PerineumSphincter.write(initialPerineumAngle);          // Put the Right Sphincter servo in the initial position
  Serial.println(F("Perineum Sphincter servo-motor setup: complete"));

//*************************************************
// Setup Serial communication
//*************************************************  
  Serial.begin(115200);

//*************************************************
// Setup Capacitive sensor
//*************************************************  
  Wire.begin();
  mpr121_setup();
  Serial.println(F("MPR121 setup complete")); 

//*************************************************
// Setup Dallas Temperature sensors
//*************************************************
  Serial.print(F("Initializing Temperature Control Library Version "));
  Serial.println(F(DALLASTEMPLIBVERSION));
  // Initialize the Temperature measurement library
  sensors.begin();
  
  // Find the sensors on the bus
  Serial.print(F("Number of Temperature Devices found on bus = "));  
  Serial.println(sensors.getDeviceCount());  

   // set resolution of ALL devices
  sensors.setResolution(TEMP_9BIT);     // set the resolution to 9 bit (Can be 9 to 12 bits .. lower is faster)
  Serial.print(F("Resolution set on all Temperature Devices = "));
  Serial.print(sensors.getResolution()); Serial.println(F("bits"));
      
    // Use the following code instead if you want to set resolution manually to each device
    // set the resolution to 9 bit (Can be 9 to 12 bits .. lower is faster)
    //for(int i=0; i<=5; i++)
    //{
    //  sensors.setResolution(*ArrayOfSensorAddress[i], TEMP_9_BIT);  // dereference pointer to DeviceAddress
    //}
  
}



void loop()
{
//*************************************************
// Reads serial port when characters are presents
//*************************************************
  while (Serial.available())
  {
    char inptutchar = Serial.read();  //gets one byte from serial buffer
    readString += inptutchar;         //makes the string readString
    delay(2);                         //slow looping to allow buffer to fill with next character
  }

//*************************************************
// Reads commands and send to the servo-motors
//*************************************************    
  if (readString.length() >0) 
  {
     if ((readString[0] == '#') && (readString[1] == 'R') && (readString[4] == '\n'))  // debug test Hook opening 
     {
       if(readString[2] == 'H')
       {
          if(readString[3] == '1')
          {
            set_relay(pin_RELAY_HOOK, true);
            Serial.println(F("Relay Hook ON")); //debug
          }
          else if (readString[3] == '0')
          {
            set_relay(pin_RELAY_HOOK, false);
            Serial.println(F("Relay Hook OFF")); //debug
          }
          else
          {
            Serial.print(F("Relay Hook command not in the good format")); //debug
          }
       }
       else if(readString[2] == 'S')
       {
          if(readString[3] == '1')
          {
            set_relay(pin_RELAY_SPHINCTERS, true);
            Serial.println(F("Relay Sphincters ON")); //debug
          }
          else if (readString[3] == '0')
          {
            set_relay(pin_RELAY_SPHINCTERS, false);
            Serial.println(F("Relay Sphincters OFF")); //debug
          }
          else
          {
            Serial.println(F("Relay Sphincters command not in the good format")); //debug
          }
       }
       else
       {
         Serial.print(F("Relay command not in the good format")); //debug
       }       
     }           
     else if ((readString[0] == '#') && (readString[1] == 'H') && (readString[5] == '\n'))
     {
       int H_angle = Find_servo_angle("H",readString);

       if(H_angle >= 0)   // if angle =-1 there is an error
       {
         Servo_Hook.write(H_angle);
         Serial.print(F("Hook servo new value: ")); //debug
         Serial.println(H_angle);                   //debug
       }
     }
     else if ((readString[0] == '#') && (readString[1] == 'o'))  // debug test Hook opening 
     {
       openHookTest(&Servo_Hook, closehook, openhook, 5, 100);   // First argument is a Pointer to Servo
       Serial.print(F("Hook openning")); //debug
     }
     else if ((readString[0] == '#') && (readString[1] == 'c'))  // debug test hook closing
     {
       closeHookTest(&Servo_Hook, openhook, closehook, 5, 100);  // First argument is a Pointer to Servo
       Serial.print(F("Hook closing")); //debug
     }
     else if ((readString[0] == '#') && (readString[1] == 'C') && (readString[5] == '\n'))
     {
       int C_angle = Find_servo_angle("C",readString);
       
       if(C_angle >= 0)   // if angle =-1 there is an error
       {
          Servo_CoccixSphincter.write(C_angle);
          Serial.print(F("Coccix Sphincter servo new value: ")); //debug
          Serial.println(C_angle);                               //debug
       }
     }
     else if ((readString[0] == '#') && (readString[1] == 'P') && (readString[5] == '\n'))
     {
       int P_angle = Find_servo_angle("P",readString);
       
       if(P_angle >= 0)   // if angle =-1 there is an error
       {
          Servo_PerineumSphincter.write(P_angle);
          Serial.print(F("Perineum Sphincter servo new value: ")); //debug
          Serial.println(P_angle);                                 //debug
       }
     }
     else if ((readString[0] == '#') && (readString[1] == 'T') && (readString[3] == '\n'))
     {
       //String strNbSensors = readString[2];
       int nbTempSensors = readString[2] - '0'; // Convert ASCII char to number  (ASCII char '0' is int 48)
       TemperatureMultiRead(nbTempSensors);
     }
     else if(readString[1] != 'T')
     {
          Serial.println(F("Error: servo motor command not in the good format"));
     }
     else
     {
       Serial.println(F("Error: Temperature sensor command not in the good format"));
     }
     
                         
     readString=""; //empty for next input
  }  


//*************************************************
// Reads capacitive sensor
//*************************************************     
  readTouchInputs();

  if (touchStates == 1)
  {
      if (timeElapsed > interval)
      {
        //servo_angle = openhook;
        Servo_Hook.write(openhook);
        Serial.println(F("Opening"));
        timeElapsed = 0;              // reset the counter to 0 so the counting starts over...      
      }
  }
  else
  {
      timeElapsed = 0;              // reset the counter to 0 so the counting starts over...      
  }   


//*************************************************
// Reads Temperature sensors (AUTOMATICS)
//************************************************* 
//  if (timeElapsedTempSensors > intervalTempSensors)
//  {
//    TemperatureMultiRead();
//    timeElapsedTempSensors = 0;     // reset the counter to 0 so the counting starts over...      
//  }




}




int Find_servo_angle(String regular_expression, String input_string)
{
   int angle = -1;
   
   int index = Match_Regular_Expression(regular_expression, input_string);
   
   if(index != -1)
   {
      String substr = input_string.substring(index+1,index+4);
      angle = substr.toInt(); 
      if((angle < 0) || (angle > 175))
      {
         angle = -1;
         Serial.println(F("angle not in range [0 - 175] degrees"));
      }
   }
   return angle;
}


//********************************************************************************************
// Find a fixed expression in the data frame and return the position
// i.e. finds the char 'H' in the frame '#H050\n'
// This function is used in: Find_servo_angle(String regular_expression, String input_string)
//******************************************************************************************** 
int Match_Regular_Expression(String regular_expression, String input_string) 
{
  int foundpos = -1;
  for (int i = 0; i <= input_string.length() - regular_expression.length(); i++) 
  {
    if (input_string.substring(i,regular_expression.length()+i) == regular_expression)
    {
      foundpos = i;
    }
  }
  return foundpos;
}




void openHookTest(Servo* ptrtoServo, int startAngle, int endAngle, int stepAngle, unsigned int waitingTime)
{
  elapsedMillis timeElapsedServo;
  
  for (int pos = startAngle; pos <= endAngle; pos += stepAngle) 
  { // goes from 'startAngle' degrees to 'endAngle' degrees in steps of 'stepAngle' degree
    while (timeElapsedServo < waitingTime)
    {
      // waits 'waitingTime' ms for the servo to reach the position
    }
      (*ptrtoServo).write(pos);    // tell servo to go to position in variable 'pos'
      timeElapsedServo = 0;        // reset the counter to 0 so the counting starts over...     
  }
}


void closeHookTest(Servo* ptrtoServo, int startAngle, int endAngle, int stepAngle, unsigned int waitingTime)
{
  elapsedMillis timeElapsedServo;
  
  for (int pos = startAngle; pos >= endAngle; pos -= stepAngle) 
  { // goes from 'startAngle' degrees to 'endAngle' degrees in steps of 'stepAngle' degree
    while (timeElapsedServo < waitingTime)
    {
      // waits 'waitingTime' ms for the servo to reach the position
    }
      (*ptrtoServo).write(pos);    // tell servo to go to position in variable 'pos'
      timeElapsedServo = 0;        // reset the counter to 0 so the counting starts over...     
  }
}


void TemperatureMultiRead(int nbTempSensors)
{
  int tempC;  // float tempC by default but if you don't need precision use int tempC instead to save microcontroller memory.
  // Command all devices on bus to read temperature  
  sensors.requestTemperatures();  
  
  Serial.print(F("#T"));  // Header of the frame
  for(int i=0; i<nbTempSensors; i++)
  {
    tempC = sensors.getTempC(*ArrayOfSensorAddress[i]);  // returns temperature in degrees C or DEVICE_DISCONNECTED_C if the device's scratch pad cannot be read successfully.
    Serial.print(tempC);
    if (i != nbTempSensors-1)
    {
      Serial.print(F(","));
    }
    else
    {
      Serial.println();  // End of the frame
    }
  }
}




void readTouchInputs() 
{
  if (!checkInterrupt()) 
  {

    //read the touch state from the MPR121
    Wire.requestFrom(0x5A, 2);

    byte LSB = Wire.read();
    byte MSB = Wire.read();

    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states


     // Check if what electrode 0 was pressed
      if (touched & (1 << 0)) 
      {

        if (touchStates == 0) 
        {
          //pin was just touched
          Serial.println(F("touched"));
        } 
        else if (touchStates == 1) 
        {
          //pin i is still being touched
        }

        touchStates = 1;
      } 
      else 
      {
        if (touchStates == 1) 
        {
          Serial.println(F("no longer being touched"));
          //pin i is no longer being touched
        }
        touchStates = 0;
      }
  }
}




void mpr121_setup(void) 
{
  set_register(0x5A, ELE_CFG, 0x00);

  // Section A - Controls filtering when data is > baseline.
  set_register(0x5A, MHD_R, 0x01);
  set_register(0x5A, NHD_R, 0x01);
  set_register(0x5A, NCL_R, 0x00);
  set_register(0x5A, FDL_R, 0x00);

  // Section B - Controls filtering when data is < baseline.
  set_register(0x5A, MHD_F, 0x01);
  set_register(0x5A, NHD_F, 0x01);
  set_register(0x5A, NCL_F, 0xFF);
  set_register(0x5A, FDL_F, 0x02);

  // Section C - Sets touch and release thresholds for each electrode
  // set_register(0x5A, ELE0_T, TOU_THRESH);
  // set_register(0x5A, ELE0_R, REL_THRESH);
  set_register(0x5A, ELE0_T, 0x25);
  set_register(0x5A, ELE0_R, 0x30);

  // Section D
  // Set the Filter Configuration
  // Set ESI2
  //set_register(0x5A, FIL_CFG, 0x04);  // initialement
  set_register(0x5A, FIL_CFG, 0x00);   // changer par luc au lieu de initialement

  // Section E
  // Electrode Configuration
  // Set ELE_CFG to 0x00 to return to standby mode
  //set_register(0x5A, ELE_CFG, 0x0C);  // initialement Enables all 12 Electrodes
  set_register(0x5A, ELE_CFG, 0x01);  // Enables 1 Electrodes


  // Section F
  // Enable Auto Config and auto Reconfig
  /*set_register(0x5A, ATO_CFG0, 0x0B);
    set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
    set_register(0x5A, ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V

  //set_register(0x5A, ELE_CFG, 0x0C);  // initialement
  set_register(0x5A, ELE_CFG, 0x01);  //1 Electrodes
}


boolean checkInterrupt(void)
{
  return digitalRead(pin_IRQ);
}


void set_register(int address, unsigned char r, unsigned char v) 
{
  Wire.beginTransmission(address);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}

void set_relay(int pin_relay, boolean state)
{
  if(state)
  {
    digitalWrite(pin_relay, LOW);  // Powers ON Hook servo
  }
  else
  {
    digitalWrite(pin_relay, HIGH); // Powers OFF Hook servo
  }
}

