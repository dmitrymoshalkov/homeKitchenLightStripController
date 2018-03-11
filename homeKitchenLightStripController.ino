/*****************************************************************************************************************/
// Подключение:
// G - стол
// R - первый канал подсветки раб зоны
// B - второй канал подсветки рабочей зоны
/*****************************************************************************************************************/
#define MY_RADIO_NRF24
#define MY_RF24_CHANNEL	81
#define MY_NODE_ID 101
//#define MY_DEBUG // Enables debug messages in the serial log
//#define NDEBUG
#define MY_BAUD_RATE 115200
//#define MY_OTA_FIRMWARE_FEATURE

#define MY_RF24_CE_PIN 4

#define MY_TRANSPORT_WAIT_READY_MS 30000

#include <MySensors.h>  
#include <SPI.h>
#include <Encoder.h>
#include <Bounce2.h>
#include "EEPROMAnything.h"
#include <avr/wdt.h>


#define SKETCH_NAME "Kitchen Light Strip"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"



// Arduino pin attached to driver pins
#define WHITE_PIN 3 
#define WHITE2_PIN 5
#define WHITE3_PIN 6
#define NUM_CHANNELS 3 // how many channels, RGBW=4 RGB=3...

#define SENSOR_ID 30
#define TABLELIGHT_ID 31
#define REBOOT_CHILD_ID                       100


// Smooth stepping between the values
#define STEP 1
#define INTERVAL 3 // 10
const int pwmIntervals = 255; //255;
float R; // equation for dimming curve


#define KNOB_ENC_PIN_1 14    // Rotary encoder input pin 1 (A0)
#define KNOB_ENC_PIN_2 7    // Rotary encoder input pin 2
#define KNOB_BUTTON_PIN 8   // Rotary encoder button pin 

#define NOCONTROLLER_MODE_PIN 17

//#define FADE_DELAY 10       // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)
//#define SEND_THROTTLE_DELAY 400 // Number of milliseconds before sending after user stops turning knob

#define KNOBUPDATE_TIME 500

#define RADIO_RESET_DELAY_TIME 25 //Задержка между сообщениями
#define MESSAGE_ACK_RETRY_COUNT 5  //количество попыток отсылки сообщения с запросом подтверждения
#define DATASEND_DELAY  10
#define GWSTATUSCHECK_TIME 150000


boolean gotAck=false; //подтверждение от гейта о получении сообщения 

int iCount = MESSAGE_ACK_RETRY_COUNT;


Encoder knob(KNOB_ENC_PIN_2, KNOB_ENC_PIN_1);  


Bounce debouncer = Bounce(); 
 
byte oldButtonVal;

boolean whiteColor=false;

long lastencoderValue=0;
long lastcolorencoderValue=0;

boolean bGatewayPresent = true;
byte bNoControllerMode = HIGH;
unsigned long previousStatMillis = 0;


// Stores the current color settings
byte channels[3] = {WHITE_PIN,WHITE2_PIN,WHITE3_PIN};
byte values[3] = {100,100,50};
byte target_values[3] = {100,100,50}; 


// stores dimming level
byte dimming = 100;
byte target_dimming = 100;

// tracks if the strip should be on of off
boolean isOn = true;
boolean isOnTable = true;

// time tracking for updates
unsigned long lastupdate = millis();
 

// update knobs changed
unsigned long lastKnobsChanged;
boolean bNeedToSendUpdate = false;
Bounce debouncernocontroller = Bounce(); 



MyMessage msgLedStripStatus(SENSOR_ID, V_STATUS);
MyMessage msgLedStripLightness(SENSOR_ID, V_PERCENTAGE);
MyMessage msgTableLightStatus(TABLELIGHT_ID, V_STATUS);
MyMessage msgTableLightLightness(TABLELIGHT_ID, V_PERCENTAGE);


void before() 
{

  pinMode(KNOB_BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(KNOB_BUTTON_PIN, HIGH);
  debouncer.attach(KNOB_BUTTON_PIN);
  debouncer.interval(5);
  oldButtonVal = debouncer.read();

    pinMode(NOCONTROLLER_MODE_PIN,INPUT_PULLUP);
    digitalWrite(NOCONTROLLER_MODE_PIN,HIGH);
    debouncernocontroller.attach(NOCONTROLLER_MODE_PIN);
    debouncernocontroller.interval(5);
    bNoControllerMode = debouncernocontroller.read();


        pinMode(WHITE_PIN, OUTPUT);
        pinMode(WHITE2_PIN, OUTPUT);
        pinMode(WHITE3_PIN, OUTPUT);

// set up dimming
  R = (pwmIntervals * log10(2))/(log10(255));



EEPROM_readAnything(0, values);
EEPROM_readAnything(4, target_values);

}


void presentation() 
{






					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER) && iCount > 0 )
	                      {
	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }

						wait(RADIO_RESET_DELAY_TIME);



					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !present(SENSOR_ID, S_DIMMER, "LightStrip") && iCount > 0 )
	                      {
	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }

						wait(RADIO_RESET_DELAY_TIME);


					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !present(TABLELIGHT_ID, S_DIMMER, "TableLight") && iCount > 0 )
	                      {
	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }

						wait(RADIO_RESET_DELAY_TIME);



					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !present(REBOOT_CHILD_ID, S_BINARY, "Reboot sensor") && iCount > 0 )
	                      {
	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }

						wait(RADIO_RESET_DELAY_TIME);


 
        wait(RADIO_RESET_DELAY_TIME);
        request(SENSOR_ID, V_PERCENTAGE);
  		wait(1000); 


    	//wait(RADIO_RESET_DELAY_TIME);
    	request(SENSOR_ID, V_STATUS);
  		wait(1000); 


        wait(RADIO_RESET_DELAY_TIME);
        request(TABLELIGHT_ID, V_PERCENTAGE);
  		wait(1000); 


    	//wait(RADIO_RESET_DELAY_TIME);
    	request(TABLELIGHT_ID, V_STATUS);




  	//Enable watchdog timer
  	wdt_enable(WDTO_8S); 

}



void setup() {

  #ifdef NDEBUG 	
  Serial.begin(115200);
  #endif

}



void loop()
{

  debouncernocontroller.update();
  byte switchState = debouncernocontroller.read();



  // and set the new light colors
  if (millis() > lastupdate + INTERVAL) {
    updateLights();
    lastupdate = millis();
  } 

checkButtonClick();

checkRotaryEncoder();

updateBrightness();

    //reset watchdog timer
    wdt_reset();  


}




void receive(const MyMessage &message) 
{
 // Handle incoming message 

  //if (message.isAck())
  //{
  //  gotAck = true;
  //  return;
  //}

    if ( message.sensor == REBOOT_CHILD_ID && message.getBool() == true && strlen(message.getString())>0 ) {
    	   	  #ifdef NDEBUG      
      			Serial.println("Received reboot message");
   	  			#endif    
             //wdt_enable(WDTO_30MS);
              while(1) { 
                
              };

     return;         

     }

   if ( message.sensor == SENSOR_ID && strlen(message.getString())>0 ) 
   {

      	if (message.type == V_DIMMER) 
      	{
      	byte currVal=message.getByte();
       		for (int i = 0; i < NUM_CHANNELS-1; i++) 
			{
      			target_values[i] = map(currVal,0,100,0,255);
      		}
      	knob.write(currVal);
  		}

  	 
  	 	if (message.type == V_STATUS) 
  	 	{
		    isOn = message.getInt();

    		#ifdef NDEBUG
    		if (isOn) {
      		Serial.println("on");
    		} else {
      		Serial.println("off");
    		}
    		#endif
  		}
   }


   if ( message.sensor == TABLELIGHT_ID && strlen(message.getString())>0 ) 
   {

      	if (message.type == V_DIMMER) 
      	{
      		byte currVal=message.getByte();

      		if (currVal < 13) currVal = 13;
      		if (currVal > 100) currVal = 100;

      		target_values[2] = map(currVal,0,100,0,255);

      				    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !send(msgTableLightLightness.set(currVal,0)) && iCount > 0 )
	                      {

	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }

  		}

  	 
  	 	if (message.type == V_STATUS) 
  	 	{
		    isOnTable = message.getInt();




    		#ifdef NDEBUG
    		if (isOnTable) {
      		Serial.println("on");
    		} else {
      		Serial.println("off");
    		}
    		#endif
  		}
   }


 }




// this gets called every INTERVAL milliseconds and updates the current pwm levels for all colors
void updateLights() {  


  // for each color
  for (int v = 0; v < NUM_CHANNELS; v++) {

    if (values[v] < target_values[v]) {
      values[v] += STEP;
      if (values[v] > target_values[v]) {
        values[v] = target_values[v];
      }
    }

    if (values[v] > target_values[v]) {
      values[v] -= STEP;
      if (values[v] < target_values[v]) {
        values[v] = target_values[v];
      }
    }
  }

  // dimming
  if (dimming < target_dimming) {
    dimming += STEP;
    if (dimming > target_dimming) {
      dimming = target_dimming;
    }
  }
  if (dimming > target_dimming) {
    dimming -= STEP;
    if (dimming < target_dimming) {
      dimming = target_dimming;
    }
  }




		    if (isOnTable) {
		      analogWrite(channels[2], pow (2, (values[2] / R)) - 1);
		    } else {
		      analogWrite(channels[2], 0);
		    }

  // set actual pin values
  for (int i = 0; i < NUM_CHANNELS-1; i++) {
    if (isOn) {
      analogWrite(channels[i], pow (2, (values[i] / R)) - 1);
    } else {
      analogWrite(channels[i], 0);
      	    	EEPROM_writeAnything(0, values);  
      	    	EEPROM_writeAnything(4, target_values);        	    	
    }
  }




}




void checkButtonClick() 
{
  debouncer.update();
  byte buttonVal = debouncer.read();
  byte newLevel = 0;
  if (buttonVal != oldButtonVal && buttonVal == LOW) {

  	if (isOn)
  	{
  		isOn = false;
  		isOnTable = false;
  	}
  	else
  	{
  		isOn = true;
  		isOnTable = true;
  	}

					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !send(msgLedStripStatus.set(isOn?"1":"0")) && iCount > 0 )
	                      {

	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }
  

					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !send(msgTableLightStatus.set(isOnTable?"1":"0")) && iCount > 0 )
	                      {

	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }

      }

  
  oldButtonVal = buttonVal;
}



void checkRotaryEncoder() 
{
  long encoderValue = knob.read();

  if (encoderValue != lastencoderValue)
  {
        #ifdef NDEBUG 
  			Serial.print("Encoder value: ");
  			Serial.println(encoderValue);
        #endif

  			lastencoderValue=encoderValue;


 for (int i = 0; i < NUM_CHANNELS-1; i++) 
{
    target_values[i] = map(lastencoderValue,0,100,0,255);
}


	lastKnobsChanged = millis();
	bNeedToSendUpdate = true;

	//isOn = true;


  }

  if (encoderValue > 100) {
    encoderValue = 100;
    knob.write(100);
  } else if (encoderValue < 13) {
    encoderValue = 13;
    knob.write(13);
  }

 
}



void updateBrightness()
{

 unsigned long currentTempMillis = millis();
    if(((currentTempMillis - lastKnobsChanged ) > KNOBUPDATE_TIME) && bNeedToSendUpdate)

	{

			bNeedToSendUpdate = false;


					    iCount = MESSAGE_ACK_RETRY_COUNT;

	                    while( !send(msgLedStripLightness.set(lastencoderValue,0)) && iCount > 0 )
	                      {

	                         wait(RADIO_RESET_DELAY_TIME);
	                        iCount--;
	                       }
 

	}

}





