//Import Libraries
//-----------------
#include <Wire.h>
#include <Keypad.h>
#include <Servo.h>

//MESSAGES MAPPING
//-----------------
//i2c can send numbers from 0 to 255 in its simplest implementation
//the constants below are a collection of messages for the communication between Master <-> Slave
#define SLAVE_STATUS_BOX_OPEN 1
#define SLAVE_STATUS_BOX_CLOSED 2
#define SLAVE_STATUS_BOX_OPENCLOSED_UNKNOWN 3
#define SLAVE_STATUS_BOX_EMPTY 4
#define SLAVE_STATUS_BOX_FULL 5
#define SLAVE_STATUS_BOX_EMPTYFULL_UNKNOWN 6
#define SLAVE_STATUS_LED_BLINKING 7
#define SLAVE_STATUS_LED_OFF 8
#define SLAVE_STATUS_LED_BLINKINGOFF_UNKNOWN 9

#define NO_DATA_REQUEST_RECEIVED_PREVIOUSLY 50
#define DATA_REQUEST_LAST_CODE_RECEIVED 51
#define DATA_REQUEST_STATUS_OPENCLOSE 52
#define DATA_REQUEST_STATUS_EMPTYFULL 53

#define ACTION_REQUEST_OPEN_BOX 100
#define ACTION_REQUEST_CLOSE_BOX 101
#define ACTION_REQUEST_BLINK_LED 102
#define ACTION_REQUEST_TURN_OFF_LED 103
#define ACTION_REQUEST_SET_KEYPAD_PWD 104

//i2c CONFIG
//-----------
#define SLAVE_ADDRESS 0x04

//Servo CONFIG
//-------------
#define servoPin 10 //Pin 10 to control the servo
//Create a Servo instance
Servo servoMotor;

//Ultrasonic Sensors HC-SR04 CONFIG
//----------------------------------
#define echoPinA 11 // Echo PinA
#define trigPinA 12 // Trigger PinA
#define echoPinB 14 // Echo PinB
#define trigPinB 15 // Trigger PinB
#define numberOfSensors 2 // Number of sensors per Arduino
#define maximumRange 40 // Maximum range needed
#define minimumRange 0 // Minimum range needed

//Keypad CONFIG
//--------------
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the symbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad and Servo
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//Runtime global variables
//-------------------------
//i2c communication
int code_received = 0; //saves the code reviced via i2c
int code_toSend = 0; //saves the code to be send when requested via i2c
int code_previousCodeReceived = 0; //saves the previous code reviced via i2c

//status variables
int status_OpenClose = SLAVE_STATUS_BOX_CLOSED; // SLAVE_STATUS_BOX_CLOSED || SLAVE_STATUS_BOX_OPEN
int status_EmptyFull = SLAVE_STATUS_BOX_EMPTY; // SLAVE_STATUS_BOX_EMPTY || SLAVE_STATUS_BOX_FULL

//password variables
bool waitingForPwd = false;
String password = "NoPw";
String inputPwd = ""; //saves the code provided with the keypad

//Ultrasonic Sensors
int lostEchoCounter = 0;
int avtive_sensor = 0;
long duration; // Duration used to calculate distance
long boxContentDistance; // Distance from the sensors to the object in the box
int emptySignalCounter = 0;

//Loop pending actions
bool pending_openDoor = false;
bool pending_closeDoor = false;
bool pending_blinkLed = false;
bool pending_turnOffLed = false;


//Lifecycle Function
//-------------------
//Setup
void setup() {
  Serial.begin(9600); // start serial for output

  //i2c SETUP
  //----------
  // initialize i2c as slave
  Wire.begin(SLAVE_ADDRESS);
  // define callbacks for i2c communication
  Wire.onReceive(receiveData);
  Wire.onRequest(sendData);
  
  //Servo SETUP
  //------------
  servoMotor.attach(servoPin); //Pin 10 to control the servo
  
  //Ultrasonic Sensors SETUP
  //-------------------------
  //HC-SR04 sensors setup
  pinMode(trigPinA, OUTPUT);
  pinMode(echoPinA, INPUT);
  pinMode(trigPinB, OUTPUT);
  pinMode(echoPinB, INPUT);
   
  Serial.println("Ready!");
}

//Loop
void loop() {
  checkEmptyFull();
  //CheckKeypad
  checkKeypad();
  //Resolve Pending Actions (if any)
  resolvePendingActions();
  //Delay 50ms before next reading.
  delay(50);

  /*
   * UNCOMMENT to enable SerialTesting
   * 
  //----------------------------------------
  //TEST: Wire simulation with Serial input
  //----------------------------------------
  // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    code_received = Serial.read();
    processReceivedCode();
  
    // say what you got:
    Serial.print("I received: ");
    Serial.println(code_received);

    int readRequestSimulationCode = 125;
    if(code_received == readRequestSimulationCode){
      Serial.println(code_toSend);
    }
  }
  */
}

//resolve all pending actions based on variables
void resolvePendingActions(){
  //Resolve pending actions
  if(pending_openDoor){
    openDoor();
    pending_openDoor = false;
  }
  if(pending_closeDoor){
    closeDoor();
    pending_closeDoor = false;
  }
  if(pending_blinkLed){
    //TODO: BlinkLed() call
    pending_blinkLed = false;
  }
  if(pending_turnOffLed){
    //TODO: TurnOffLed() call
    pending_turnOffLed = false;
  }
}

//i2c slave send/reveive callbacks
//---------------------------------
// callback for received data
void receiveData(int byteCount){
  while(Wire.available()) {
    code_received = Wire.read();
    processReceivedCode();
  }
}

// callback for sending data
void sendData(){
  Wire.write(code_toSend);
  //clean the "code_toSend" variable after send it
  code_toSend = NO_DATA_REQUEST_RECEIVED_PREVIOUSLY;
}

//process the code received from the Master and executes the corresponding functions
void processReceivedCode(){
  //If Arduino is waiting for password code, then set the received code as new password
  if(waitingForPwd){
    setNewPassword();
  }
  else{
    if(code_received == DATA_REQUEST_STATUS_OPENCLOSE){
      //prepare variable to return the current Open/Close status
      code_toSend = status_OpenClose;
    }
    else if(code_received == DATA_REQUEST_STATUS_EMPTYFULL){
      //prepare variable to return the current Empty/Full status
      code_toSend = status_EmptyFull;
    }
    else if(code_received == DATA_REQUEST_LAST_CODE_RECEIVED){
      //prepare variable to return the previous code received
      code_toSend = code_previousCodeReceived;
    }
    else if(code_received == ACTION_REQUEST_OPEN_BOX){
      //Open door
      openDoor();
    }
    else if(code_received == ACTION_REQUEST_CLOSE_BOX){
      //Close door
      closeDoor();
    }
    else if(code_received == ACTION_REQUEST_BLINK_LED){
      //Turn on LED --> blinking
      //TODO
    }
    else if(code_received == ACTION_REQUEST_TURN_OFF_LED){
      //Turn off LED
      //TODO
    }
    else if(code_received == ACTION_REQUEST_SET_KEYPAD_PWD){
      waitingForPwd = true;
    }
  }

  //Save code for possible checking
  code_previousCodeReceived = code_received;
}

//Password functions
//------------------------------
void setNewPassword(){
  password = code_received; //TODO: password is type 'char'. code_received is type 'int'. check it is correct
  Serial.print("New password set: ");
  Serial.println(password);
  waitingForPwd = false;
}

//Empty/Full sensors functions
//------------------------------
//get distance from sensors to the objects
void checkEmptyFull(){
  distanceSensorCheck(trigPinA, echoPinA);
  distanceSensorCheck(trigPinB, echoPinB);
  emptyCheck();
}

void distanceSensorCheck(int trigPin, int echoPin){
  /*  The following trigPin/echoPin cycle is used to determine the 
   *  distance of the nearest object by bouncing soundwaves off of it. */ 
  digitalWrite(trigPin, LOW); 
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  /* END of trigPin/echoPin cycle */
   
  //Calculate the distance (in cm) based on the speed of sound.
  boxContentDistance = duration/58.2;
  
  if (boxContentDistance >= maximumRange || boxContentDistance <= minimumRange){
    //Send a negative number to computer to indicate "out of range"
    //Fire lostEchoCheck function, to evaluate a possible error in the sensont
    lostEchoCheck(echoPin); 
  }
  else {
    /* Send the distance to the computer using Serial protocol */
    if(boxContentDistance < 30){
      status_EmptyFull = SLAVE_STATUS_BOX_FULL;
      emptySignalCounter = 0; // if one sensor detects an object -> 0
    }
    else {
      emptySignalCounter += 1; //add 1 whenever a sensor detect a "no-object"
    }
  }
}

//Empty box only if all sensors says "empty box"
void emptyCheck(){
  if(emptySignalCounter >= numberOfSensors){
    status_EmptyFull = SLAVE_STATUS_BOX_EMPTY;
    emptySignalCounter = 0;
  }
}

//If there no echo detected during 10 times, reset the echoPin for the sensor with problems
void lostEchoCheck(int echoPin){
  lostEchoCounter += 1;
  if(lostEchoCounter > 10){
    pinMode(echoPin, OUTPUT);
    delay(50);
    digitalWrite(echoPin, LOW);
    delay(50);
    pinMode(echoPin, INPUT);
    //reset counter
    lostEchoCounter = 0;
    //set Empty/Full status to Unknown
    status_EmptyFull = SLAVE_STATUS_BOX_EMPTYFULL_UNKNOWN;
  }
}

//Keyboard functions
//--------------------
//Read key if keaypad was used
void checkKeypad(){
  char keyPressed = customKeypad.getKey();
  
  if (keyPressed){
    if(keyPressed == '*'){
      if(inputPwd == password){
        openDoor();
      }
      else if(status_OpenClose == SLAVE_STATUS_BOX_OPEN){
        closeDoor();
      }
      Serial.print("password: ");
      Serial.println(inputPwd);
      inputPwd = "";
    }
    else {
      if(status_OpenClose == SLAVE_STATUS_BOX_OPEN){
        closeDoor();
      }
      inputPwd += keyPressed;
    }
  }
}

//Door Lock mechanism functions
//------------------------------
//activate servo to open door
void openDoor(){
  for (int i=90; i>0; i--){
    servoMotor.write(i);
    delay(10);
  }
  status_OpenClose = SLAVE_STATUS_BOX_OPEN;
}

//activate servo to close door
void closeDoor(){
  for (int i=0; i<90; i++){
    servoMotor.write(i);
    delay(10);
  }
  status_OpenClose = SLAVE_STATUS_BOX_CLOSED;
}
