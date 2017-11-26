# IoTChallenge_ArduinoBoxSlaves
This repository saves the code for the Arduino Slaves in the boxes of the "NoFrontiers" project for the 2017 IoT Challenge


## 1. box_i2c_slave
Features:
1. Control Servo Motor on demand
2. Read ultrasonic sensors HC-SR04 signals to determine if there are objects at a certain distance
3. Keypad enabled. The code inserted in the keypad is compared with a saved password
4. i2c communication enabled. Preapred to receive certain codes to perform some actions
.

## 2. Multiple Slaves in 1 GW
When connecting multiple slaves to one RaspPi GW you need to define different HEX address for each slave. To do so, just change the corresponding constant in the Arduino Code
```
//i2c CONFIG
//-----------
#define SLAVE_ADDRESS 0x04
```

> *for example from 0x04 to **0x05** or **0x06** or ...* 

When connecting multiple slaves to one RaspPi GW it is important to be sure that the Arduinos have a good power supply. If you use different power supplies for the Arduinos and/or RaspPi GW, make sure all of them share the same GND. Otherwise, I2C communication won't work.

**MESSAGES MAPPING**
> i2c can send numbers from 0 to 255 in its simplest implementation. 

> The constants below are a collection of messages for the communication between Master <-> Slave. 

> Slaves (Arduinos) are always pasive. They don't insert data into the BUS unless Master calls for it. 

> If Master sends an ACTION code, the slave will execute certain action in the actuators. 

> Whenever there is an status update, the new status will be added to a buffer, to be sent whenever the Master wants to read

> If Master wants to read some data, first should read how many items there are in the buffer, writing the codeDATA_REQUEST_STATUS_BUFFER_INDEX first. This will prepare the buffer_lenght to be sent in the Arduino. Then Master can read() the bus in the Slave's address to get the buffer_lenght first. Next read() calls will deliver the data in the buffer, entry by entry 


*LIST OF MESSAGES*

```
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
```


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
  
  
# IoTChallenge schematics

Setting this with the [IoTChallenge_GW](https://github.com/rafachurre/IoTChallenge_RaspPiGateway)

.

![](https://raw.githubusercontent.com/rafachurre/IoTChallenge_ArduinoBoxSlaves/master/Arduino_Keypad4x4_Servo_Ultrasounds_schematics_bb.jpg)
