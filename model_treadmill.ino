//*****************************OLed setup *********************************************************************************
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//******************************Pluse Sensor Setup**************************************************************************

#include <PulseSensorPlayground.h>     // Includes the PulseSensorPlayground Library.   

//  Variables
const int PulseWire = 0;       // PulseSensor PURPLE WIRE connected to ANALOG PIN 0
const int LED = LED_BUILTIN;          // The on-board Arduino LED, close to PIN 13.
int Threshold = 580;           // Determine which Signal to "count as a beat" and which to ignore.
                               // Use the "Gettting Started Project" to fine-tune Threshold Value beyond default setting.
                               // Otherwise leave the default "550" value. 
                               
PulseSensorPlayground pulseSensor;  // Creates an instance of the PulseSensorPlayground object called "pulseSensor"

// ******************************Keypad Setup***********************************************************************************

#include "Adafruit_Keypad.h"

const byte ROWS = 4; // rows
const byte COLS = 3; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'.','0',' '}
};
byte rowPins[ROWS] = {2,3,4,5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = { 8,9,10}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Adafruit_Keypad myKeypad = Adafruit_Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS);
// *******************************Motor Setup***************************************************************************************

#include <Adafruit_MotorShield.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *myMotor = AFMS.getMotor(3);
// You can also make another motor on port M2
//Adafruit_DCMotor *myOtherMotor = AFMS.getMotor(2);

// ****************************************************global************************************************************************
#include <RunningAverage.h>
void refresh(int bpm, int avgbpm, double speed); // method to update all data on the display
void setMotorSpeed(std::string sp); // changes the speed of the motor based on keypad entry
int bpm = -1; // holds the current bpm
// variables to acesses speed later
std::string entry; // string to hold keypad inputs
double speed; // holds speed sent from keypad as double
RunningAverage myRA(100); // running average size 100 to average the bpm




// ***************************************************Setup*************************************************************************
void setup() {


  Serial.begin(9600);

  
// *****************************************************************************************************************************
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.println("Heart rate: ");
  display.println("Average heart rate: ");
  display.println("Speed: 0 In/s");
  display.display();
// *****************************************************************************************************************************
  
   // Configure the PulseSensor object, by assigning our variables to it. 
  pulseSensor.analogInput(PulseWire);   
  pulseSensor.blinkOnPulse(LED);       //auto-magically blink Arduino's LED with heartbeat.
  pulseSensor.setThreshold(Threshold);   

  // Double-check the "pulseSensor" object was created and "began" seeing a signal. 
   if (pulseSensor.begin()) {
    Serial.println("We created a pulseSensor Object !");  //This prints one time at Arduino power-up,  or on Arduino reset.  
  }

// *****************************************************************************************************************************
  myKeypad.begin();

// *****************************************************************************************************************************
   if (!AFMS.begin()) {         // create with the default frequency 1.6KHz
  // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1);
  }
  Serial.println("Motor Shield found.");

  myMotor->setSpeed(0);
  myMotor->run(FORWARD);
  
}





void loop() {

  myKeypad.tick();

  while(myKeypad.available()){ 
    keypadEvent e = myKeypad.read();
    Serial.print((char)e.bit.KEY);
    if(e.bit.EVENT == KEY_JUST_PRESSED) Serial.println(" pressed"); // checking for key press
    else if(e.bit.EVENT == KEY_JUST_RELEASED){
      Serial.println(" released");
      if(e.bit.KEY==' '){ // the '#' key is set to a whitespace and acts like an enter button 
                          // here when the enter key is pressed the speed is updated
        setMotorSpeed(entry);
      }
      else
        entry.push_back(e.bit.KEY); // any other key pressed that is not the '#' key is sent to a string called entry
    } 
  } 
  delay(10);




 if (pulseSensor.sawStartOfBeat()) {            // Constantly test to see if "a beat happened".
  int bpm = pulseSensor.getBeatsPerMinute();  // Calls function on our pulseSensor object that returns BPM as an "int".
                                               // "myBPM" hold this BPM value now.
  if(bpm>20&&bpm<200){
    myRA.addValue(bpm);
    refresh(bpm,myRA.getAverage(),speed);
  }
//   Serial.println("♥  A HeartBeat Happened ! "); // If test is "true", print a message "a heartbeat happened".
//    Serial.print("BPM: ");                        // Print phrase "BPM: " 
//   Serial.println(myBPM);                        // Print the value inside of myBPM. 
}

  delay(20);                    // considered best practice in a simple sketch.


}


void setMotorSpeed(std::string sp){
  if(!sp.empty()){ // taking the string entry and converting it to a double
    speed = stod(entry);
    Serial.print(speed); // confirm speed by printing to serial
    Serial.println(" entered");
    entry.clear(); // empties keypad for next input
  }


  // using the double gotten from converting the string to a double
  // checks to see if the input falls within the bounds of the speed
  if( speed>=3&&speed<=8.25){ 
    myMotor->setSpeed(27.504*speed+27.593); // speed of the treadmill is measured inches per second
    refresh(bpm,myRA.getAverage(),speed);
  }
  else if( speed >= 8.25){ // if the speed is outside the bounds by being to large speed is set to the maximum
    myMotor->setSpeed(255);
    speed = 8.25;
    refresh(bpm,myRA.getAverage(),speed);
  }
  else{ // if the speed is outside the bound by being to small speed is set to zero
     myMotor->setSpeed(0);
     speed = 0;
    refresh(bpm,myRA.getAverage(),speed);
  }
}


void refresh(int bpm,int avg, double speed){ // takes the data and displays on the oled
  display.clearDisplay(); // clears the screen to be re-written

  // set-up text size, color and starting position (cursor)
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner

  display.print("Heart rate: ");
  display.println(bpm);

  display.print("Avg. heart rate: ");
  display.println(avg);

  display.print("Speed: ");
  display.print(speed);
  display.println(" In/s");

  display.display(); // sends display to oled
}




