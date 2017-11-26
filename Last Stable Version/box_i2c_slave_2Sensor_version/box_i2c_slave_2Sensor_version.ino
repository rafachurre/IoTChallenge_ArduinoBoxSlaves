//Import Libraries
//-----------------
#include <Wire.h>
#include <Keypad.h>
#include <Servo.h>

//MESSAGES MAPPING
//-----------------
//i2c can send numbers from 0 to 255 in its simplest implementation
//the constants below are a collection of messages for the communication between Master <-> Slave
#define SLAVE_NO_BUFFER_DATA 0
#define SLAVE_STATUS_BOX_OPEN 1
#define SLAVE_STATUS_BOX_CLOSED 2
#define SLAVE_STATUS_BOX_OPENCLOSED_UNKNOWN 3
#define SLAVE_STATUS_BOX_EMPTY 4
#define SLAVE_STATUS_BOX_FULL 5
#define SLAVE_STATUS_BOX_EMPTYFULL_UNKNOWN 6
#define SLAVE_STATUS_LED_BLINKING 7
#define SLAVE_STATUS_LED_OFF 8
#define SLAVE_STATUS_LED_BLINKINGOFF_UNKNOWN 9

#define DATA_REQUEST_STATUS_BUFFER_INDEX 50
#define DATA_REQUEST_GET_ALL_STATUSES 51
#define DATA_REQUEST_STATUS_OPENCLOSE 52
#define DATA_REQUEST_STATUS_EMPTYFULL 53
#define DATA_REQUEST_STATUS_LEDSTATUS 54

#define ACTION_REQUEST_CLEAR_STATUS_BUFFER 74
#define ACTION_REQUEST_OPEN_BOX 75
#define ACTION_REQUEST_CLOSE_BOX 76
#define ACTION_REQUEST_BLINK_LED 77
#define ACTION_REQUEST_TURN_OFF_LED 78
#define ACTION_REQUEST_SET_KEYPAD_PWD 79

//MAIN LOOP CONFIG
//----------------
#define main_loop_delay 50 // miliseconds delay in the main loop

//i2c CONFIG
//-----------
#define SLAVE_ADDRESS 0x04

//LED CONFIG
//-----------
#define LEDPin 17
#define LED_BLINK_PERIOD 20 // number of loops before changing the LED state. Each loop needs 50ms.
#define NUMBER_OF_BLINKS 5 // Number of times the LED turns on during the blinking phase

//Servo CONFIG
//-------------
#define servoPin 16 //Pin 16 to control the servo
//Create a Servo instance
Servo servoMotor;

//Ultrasonic Sensors HC-SR04 CONFIG
//----------------------------------
#define echoPinA 3 // Echo PinA
#define trigPinA 4 // Trigger PinA
#define echoPinB 14 // Echo PinB
#define trigPinB 15 // Trigger PinB
#define numberOfSensors 2 // Number of sensors per Arduino
#define maximumRange 40 // Maximum range needed
#define minimumRange 0 // Minimum range needed
#define emptyThreshold 5 // Opposite wall distance. Cannot be an object further than this distance.

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
byte rowPins[ROWS] = {5, 6, 7, 8}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 10, 11, 12}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad and Servo
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//Runtime global variables
//-------------------------
//status variables
#define STATUS_CHANGED_BUFFER_MAX_LENGTH 50 // Max lenght of the buffer
int status_changed_Buffer[STATUS_CHANGED_BUFFER_MAX_LENGTH];
int status_changed_buffer_index = 0;

int status_OpenClose = SLAVE_STATUS_BOX_CLOSED; // SLAVE_STATUS_BOX_CLOSED || SLAVE_STATUS_BOX_OPEN || SLAVE_STATUS_BOX_OPENCLOSED_UNKNOWN
int status_EmptyFull = SLAVE_STATUS_BOX_EMPTY; // SLAVE_STATUS_BOX_EMPTY || SLAVE_STATUS_BOX_FULL || SLAVE_STATUS_BOX_EMPTYFULL_UNKNOWN
int status_LED = SLAVE_STATUS_LED_OFF; // SLAVE_STATUS_LED_BLINKING || SLAVE_STATUS_LED_OFF || SLAVE_STATUS_LED_BLINKINGOFF_UNKNOWN

//i2c communication
int code_received = 0; //saves the code reviced via i2c
int code_toSend = SLAVE_NO_BUFFER_DATA; //saves the code to be send when requested via i2c
int code_previousCodeReceived = 0; //saves the previous code reviced via i2c

//LED variables
int blinkingCounter = 0;
int blinkPeriodCounter = 0;
bool ledIsBlinking = false;
int ledCurrentStatus = 0; // 0 = LOW / 1 = HIGH

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

int emptyFull_safetyCounter = 0;
int emptyFull_safetyPeriod = 20; // Cycles waiting before safety check

//Loop pending actions
bool pending_send_status_buffer = false;
bool pending_openDoor = false;
bool pending_closeDoor = false;
bool pending_blinkLed = false;
bool pending_turnOffLed = false;
bool pending_updateFullBox = false;
bool pending_updateEmptyBox = false;
bool pending_emptyFullSafetyCheck = false;

//Debugging global variables
//-------------------------
int serialPrintFrequency = 20; //Number of loops before printing in the Serial console. Each loop = 50ms
int serialPrintCounter = 0;


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
  
  //LED SETUP
  //------------
  pinMode(LEDPin, OUTPUT);
  
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
  //Blink LED routine
  blinkLed();
  //Check if the box is empty or full
  checkEmptyFull();
  //CheckKeypad
  checkKeypad();
  //Resolve Pending Actions (if any)
  resolvePendingActions();
  //Delay 50ms before next reading.
  delay(main_loop_delay);

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
    
    // say what you got:
    Serial.print("I received: ");
    Serial.println(code_received);
    
    processReceivedCode();

    int readRequestSimulationCode = 125;
    if(code_received == readRequestSimulationCode){
      if (status_changed_buffer_index > 0){
        code_toSend = popFromStatusBuffer();
      }
      else{
        code_toSend = SLAVE_NO_BUFFER_DATA;
      }
      Serial.println(code_toSend);
    }
  }
  
  if(serialPrintCounter >= serialPrintFrequency){
    serialPrintCounter = 0;
    //Log status in serial console
    Serial.print("Distance: ");
    Serial.println(boxContentDistance);
    Serial.print("Status OpenClose: ");
    if(status_OpenClose == SLAVE_STATUS_BOX_CLOSED){
      Serial.println("CLOSED");
    } else if(status_OpenClose == SLAVE_STATUS_BOX_OPEN){
      Serial.println("OPEN");
    } else {
      Serial.print("UNKNOWN: ");
      Serial.println(status_OpenClose);
    } 
    Serial.print("Status EmptyFull: ");
    if(status_EmptyFull == SLAVE_STATUS_BOX_EMPTY){
      Serial.println("EMPTY");
    } else if(status_EmptyFull == SLAVE_STATUS_BOX_FULL){
      Serial.println("FULL");
    } else {
      Serial.print("UNKNOWN: ");
      Serial.println(status_EmptyFull);
    } 
    Serial.print("Status LED: ");
    if(ledIsBlinking){
      Serial.println("BLINKING");
    } else {
      Serial.println("OFF");
    }
  } else {
    serialPrintCounter += 1;
  }
  */
}

//Runtime Functions
//------------------

//resolve all pending actions based on variables
void resolvePendingActions(){
  //Resolve pending actions
  if(pending_openDoor){
    openDoor();
    pending_openDoor = false;
    status_OpenClose = SLAVE_STATUS_BOX_OPEN;
    pushStatusToBuffer(status_OpenClose);
  }
  if(pending_closeDoor){
    closeDoor();
    pending_closeDoor = false;
    status_OpenClose = SLAVE_STATUS_BOX_CLOSED;
    pushStatusToBuffer(status_OpenClose);
  }
  if(pending_blinkLed){
    blinkingCounter = 0;
    ledIsBlinking = true;
    pending_blinkLed = false;
    status_LED = SLAVE_STATUS_LED_BLINKING;
    pushStatusToBuffer(status_LED);
  }
  if(pending_turnOffLed){
    turnOffLed();
    pending_turnOffLed = false;
    status_LED = SLAVE_STATUS_LED_OFF;
    pushStatusToBuffer(status_LED);
  }
  if(pending_updateFullBox){
    pending_updateFullBox = false;
    status_EmptyFull = SLAVE_STATUS_BOX_FULL;
    pushStatusToBuffer(status_EmptyFull);
  }
  if(pending_updateEmptyBox){
    pending_updateEmptyBox = false;
    status_EmptyFull = SLAVE_STATUS_BOX_EMPTY;
    pushStatusToBuffer(status_EmptyFull);
  }
  if(pending_emptyFullSafetyCheck){
    if(emptyFull_safetyCounter >= emptyFull_safetyPeriod){
      pending_emptyFullSafetyCheck = false;
      emptyFull_safetyCounter = 0;
      emptyFullSafetyCheck();
    } else {
      emptyFull_safetyCounter += 1;
    }
  }
}

void clearStatusBuffer(){
  status_changed_buffer_index = 0;
}

//pop the next value in the status changed
int popFromStatusBuffer(){
  status_changed_buffer_index -= 1;
  int value = status_changed_Buffer[status_changed_buffer_index];
  Serial.print("Value poped in buffer[");
  Serial.print(status_changed_buffer_index);
  Serial.print("]: ");
  Serial.println(value);
  return value;
}

void pushStatusToBuffer(int value){
  status_changed_Buffer[status_changed_buffer_index] = value;
  Serial.print("Value pushed in buffer[");
  Serial.print(status_changed_buffer_index);
  Serial.print("]: ");
  Serial.println(value);
  status_changed_buffer_index += 1;
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
  if(pending_send_status_buffer){
    pending_send_status_buffer = false;
    code_toSend = status_changed_buffer_index;
  }
  else if (status_changed_buffer_index > 0){
    code_toSend = popFromStatusBuffer();
  }
  else{
    code_toSend = SLAVE_NO_BUFFER_DATA;
  }
  Wire.write(code_toSend);
}

//process the code received from the Master and executes the corresponding functions
void processReceivedCode(){
  //If Arduino is waiting for password code, then set the received code as new password
  if(waitingForPwd){
    setNewPassword();
    waitingForPwd = false;
  }

  else{
    if(code_received == ACTION_REQUEST_CLEAR_STATUS_BUFFER){
      clearStatusBuffer(); // delete all entries in status buffer
    }
    else if(code_received == DATA_REQUEST_STATUS_BUFFER_INDEX){
      //pushStatusToBuffer(status_changed_buffer_index); // push the index to the buffer, to be read in the next i2c_Read()
      pending_send_status_buffer = true;
    }
    else if(code_received == DATA_REQUEST_GET_ALL_STATUSES){
      //Prepare all statuses to be read
      pushStatusToBuffer(status_OpenClose);
      pushStatusToBuffer(status_EmptyFull);
      pushStatusToBuffer(status_LED);
    }
    else if(code_received == DATA_REQUEST_STATUS_OPENCLOSE){
      //prepare variable to return the current Open/Close status
      pushStatusToBuffer(status_OpenClose);
    }
    else if(code_received == DATA_REQUEST_STATUS_EMPTYFULL){
      //prepare variable to return the current Empty/Full status
      pushStatusToBuffer(status_EmptyFull);
    }
    else if(code_received == DATA_REQUEST_STATUS_LEDSTATUS){
      //prepare variable to return the current LED status
      pushStatusToBuffer(status_LED);
    }
    else if(code_received == ACTION_REQUEST_OPEN_BOX){
      //Open door
      pending_openDoor = true;
      pending_turnOffLed = true;
    }
    else if(code_received == ACTION_REQUEST_CLOSE_BOX){
      //Close door
      pending_closeDoor = true;
    }
    else if(code_received == ACTION_REQUEST_BLINK_LED){
      //Turn on LED --> blinking
      pending_blinkLed = true;
    }
    else if(code_received == ACTION_REQUEST_TURN_OFF_LED){
      //Turn off LED
      pending_turnOffLed = true;
    }
    else if(code_received == ACTION_REQUEST_SET_KEYPAD_PWD){
      waitingForPwd = true;
    }
  }

  //Save code for possible checking
  code_previousCodeReceived = code_received;
}

//LED functions
//------------------------------
void blinkLed(){
  if(ledIsBlinking){
    if(blinkingCounter < NUMBER_OF_BLINKS){
      if(blinkPeriodCounter < LED_BLINK_PERIOD){
        blinkPeriodCounter += 1;
      }
      else{
        blinkPeriodCounter = 0;
        if(ledCurrentStatus == 0){
          ledCurrentStatus = 1;
          digitalWrite(LEDPin, HIGH);
        }
        else {
          ledCurrentStatus = 0;
          blinkingCounter += 1;
          digitalWrite(LEDPin, LOW);
        }
      }
    }
    else {
      blinkingCounter = 0;
      ledIsBlinking = false;
      pending_turnOffLed = true;
    }
  }
}

void turnOffLed(){
  ledIsBlinking = false;
  blinkPeriodCounter = 0;
  ledCurrentStatus = 0;
  digitalWrite(LEDPin, LOW);
}

//Password functions
//------------------------------
void setNewPassword(){
  password = code_received; //TODO: password is type 'char'. code_received is type 'int'. check it is correct
  Serial.print("New password set: ");
  Serial.println(password);
}

//Keyboard functions
//--------------------
//Read key if keaypad was used
void checkKeypad(){
  char keyPressed = customKeypad.getKey();
  
  if (keyPressed){
    if(keyPressed == '*'){
      if(inputPwd == password){
        pending_openDoor = true;
      }
      else if(status_OpenClose == SLAVE_STATUS_BOX_OPEN){
        pending_closeDoor= true;
      }
      Serial.print("password: ");
      Serial.println(inputPwd);
      inputPwd = "";
    }
    else {
      if(status_OpenClose == SLAVE_STATUS_BOX_OPEN){
        pending_closeDoor = true;
      }
      inputPwd += keyPressed;
    }
  }
}

//Empty/Full sensors functions
//------------------------------
//get distance from sensors to the objects
void checkEmptyFull(){
  distanceSensorCheck(trigPinA, echoPinA);
  distanceSensorCheck(trigPinB, echoPinB);
  emptyCheck();
}

void emptyFullSafetyCheck(){
  pushStatusToBuffer(status_EmptyFull);
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
    if(boxContentDistance < emptyThreshold){
      if (status_EmptyFull != SLAVE_STATUS_BOX_FULL){
        //pending_updateFullBox = true;
        status_EmptyFull = SLAVE_STATUS_BOX_FULL;
        
        //Set emptyFullSafetyCheck variables
        pending_emptyFullSafetyCheck = true;
        emptyFull_safetyCounter = 0;
      }
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
    if (status_EmptyFull != SLAVE_STATUS_BOX_EMPTY){
      //pending_updateEmptyBox = true;
      status_EmptyFull = SLAVE_STATUS_BOX_EMPTY;
      
      //Set emptyFullSafetyCheck variables
      pending_emptyFullSafetyCheck = true;
      emptyFull_safetyCounter = 0;
    }
    emptySignalCounter = 0;
  }
}

//If there no echo detected during 10 times, reset the echoPin for the sensor with problems
void lostEchoCheck(int echoPin){
  lostEchoCounter += 1;
  if(lostEchoCounter > 10){
    pinMode(echoPin, OUTPUT);
    delay(10);
    digitalWrite(echoPin, LOW);
    delay(10);
    pinMode(echoPin, INPUT);
    //reset counter
    lostEchoCounter = 0;
    //set Empty/Full status to Unknown
    status_EmptyFull = SLAVE_STATUS_BOX_EMPTYFULL_UNKNOWN;
  }
}

//Door Lock mechanism functions
//------------------------------
//activate servo to open door
void openDoor(){
  for (int i=90; i>0; i--){
    servoMotor.write(i);
    delay(5);
  }
}

//activate servo to close door
void closeDoor(){
  for (int i=0; i<90; i++){
    servoMotor.write(i);
    delay(5);
  }
}
