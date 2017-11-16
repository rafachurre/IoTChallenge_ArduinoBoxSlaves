# IoTChallenge_ArduinoBoxSlaves
This repository saves the code for the Arduino Slaves in the boxes of the "NoFrontiers" project for the 2017 IoT Challenge


## 1. box_i2c_slave
Features:
1. Control Servo Motor on demand
2. Read ultrasonic sensors HC-SR04 signals to determine if there are objects at a certain distance
3. Keypad enabled. The code inserted in the keypad is compared with a saved password
4. i2c communication enabled. Preapred to receive certain codes to perform some actions

**MESSAGES MAPPING**
> i2c can send numbers from 0 to 255 in its simplest implementation. 

> The constants below are a collection of messages for the communication between Master <-> Slave. 

> Slaves (Arduinos) are always pasive. They don't insert data into the BUS unless Master calls for it. 

> If Master sends an ACTION code, the slave will execute certain action in the actuators. 

> If Master wants to read some data, it needs to send a DATA_REQUEST code first. This will prepare the data to be sent in the Arduino, and then Master can read() the bus in the Slave's address to get the data. 

> The data send from the Slave will be one of the SLAVE_STATUS messages.

<br> *SLAVE_STATUS_BOX_OPEN* **1** //The box is open
<br> *SLAVE_STATUS_BOX_CLOSED* **2** //The box is closed
<br> *SLAVE_STATUS_BOX_OPENCLOSED_UNKNOWN* **3** //It is not clear if the box is open or closed (error handling)
<br> *SLAVE_STATUS_BOX_EMPTY* **4** //The box is empty
<br> *SLAVE_STATUS_BOX_FULL* **5** //The box is full
<br> *SLAVE_STATUS_BOX_EMPTYFULL_UNKNOWN* **6** //It is not clear if the box is empty or full (error handling)
<br> *SLAVE_STATUS_LED_BLINKING* **7** //The LED is blinking
<br> *SLAVE_STATUS_LED_OFF* **8** //The LED is off
<br> *SLAVE_STATUS_LED_BLINKINGOFF_UNKNOWN* **9** //It is not clear if the LED is blinking or off (error handling)

<br> *NO_DATA_REQUEST_RECEIVED_PREVIOUSLY* **50** //Return message when Master reads without writing a DATA_REQUEST message before
<br> *DATA_REQUEST_LAST_CODE_RECEIVED* **51** //Master wants to read() the previous code received. Prepare "to_send" variable for a read event
<br> *DATA_REQUEST_STATUS_OPENCLOSE* **52** //Master wants to read() the open/close status. Prepare "to_send" variable for a read event
<br> *DATA_REQUEST_STATUS_EMPTYFULL* **53** //Master wants to read() the empty/full status. Prepare "to_send" variable for a read event

<br> *ACTION_REQUEST_OPEN_BOX* **100** //Master requests to open the box
<br> *ACTION_REQUEST_CLOSE_BOX* **101** //Master requests to close the box
<br> *ACTION_REQUEST_BLINK_LED* **102** //Master requests to blink the LED
<br> *ACTION_REQUEST_TURN_OFF_LED* **103** //Master requests to turn off the LED
<br> *ACTION_REQUEST_SET_KEYPAD_PWD* **104** //Master requests to set a new password. A [0-255] password will be sent in the next write() byte


**TEST MODE**

The code is prepare to simulate the i2c connection with Serial inputs. To use this test mode:<br>
1. Uncomment the code in the loop()
```
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
```
2. Send ASCII bytes using the Serial monitor. The ideal situation is to use ALT+<codeNumber> to send this number byte. (i.e ALT+100 will send the ACTION_REQUEST_OPEN_BOX to the Arduino.)
3. To simulate a Master read() send 125 (i.e ALT+125) and the Arduino will write in the Serial monitor the return code.
