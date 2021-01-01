#include "Arduino.h"
#include "Homie.h"
#include "Bounce2.h"

#define FIRMWARE_VERSION "1.0.0"
/* Revision history:
   1.0.0 Switched to Homie 3.0.1
         Removed reset when not connected, 
         Added new functionality for long press of the button:
           * >1 sek sends a message on switchN/broadcast_on (true or false depending on switch status) 
           * >5 sek, both switches flashes on/off
           * 5-10 sek (1-4 flashes), restart the device
           * 10 sek (5 flashes), clears the decvice an restart it 
           * >10 sek, nothing happens

   0.9.6 Switched to Homie 2.0.0

   0.9.5 Removed ArduinoOTA and relying on Homie OTA to save space... (Homie 2.0.0-beta.3)

   0.9.4 Changed led behaviour
*/

#ifdef SONOFF
  #define FIRMWARE_NAME "Sonoff"
  #define NOF_SWITHCES 1
  const int BUTTON_PIN[NOF_SWITHCES]= { 0 }; //GPIO0=D3
  const int RELAY_PIN[NOF_SWITHCES]= { 12 }; //GPIO12=D6
  HomieNode switchNode[NOF_SWITHCES]={ HomieNode("switch0", "switch") };
  #define HOMIE_PIN  13, LOW
#endif

#ifdef WIFISWITCH2
  #define FIRMWARE_NAME "WifiSwitch2"
  #define NOF_SWITHCES 2
  const int BUTTON_PIN[NOF_SWITHCES]= { 0, 14 }; //GPIO0=D3, GPIO14=D5 
  const int  RELAY_PIN[NOF_SWITHCES]= { 12, 13 }; //GPIO12=D6, GPIO13=D7
  HomieNode switchNode[NOF_SWITHCES]={ HomieNode("switch0", "switch"), HomieNode("switch1", "switch") };
#endif

Bounce button[NOF_SWITHCES];
unsigned long buttonMillis[NOF_SWITHCES];
int buttonState[NOF_SWITHCES];
bool on[NOF_SWITHCES] ;

void setSwitch(int n, bool state) {
    on[n]=state;
    digitalWrite(RELAY_PIN[n], on[n] ? HIGH : LOW);
    #ifdef LED1_PIN
        digitalWrite(LED1_PIN, on1 ? HIGH : LOW);
    #endif
    switchNode[n].setProperty("on").send(on[n] ? "true":"false");
    Homie.getLogger() << "Switch " << n << " is " << (on[n] ? "on" : "off") << endl;
}

bool switch0_OnHandler(const HomieRange& range, const String& value) {
  if (value != "true" && value != "false" && value != "1" && value != "0") return false;
  setSwitch(0, value == "true" || value =="1");
  return true;
}

#if NOF_SWITCHES==2
    bool switch1_OnHandler(const HomieRange& range, const String& value) {
        if (value != "true" && value != "false" && value != "1" && value != "0") return false;
        setSwitch(1,value == "true" || value =="1");
        return true;
    }
#endif

void setLights(bool state){
    for (int n = 0; n < NOF_SWITHCES; n++) {
        digitalWrite(RELAY_PIN[n], state ? HIGH : LOW);
    }
}

void resetLights() {
    for (int n = 0; n < NOF_SWITHCES; n++) {
        digitalWrite(RELAY_PIN[n], on[n] ? HIGH : LOW);
    }
}

void flashLights(int times) {
    for (int i = 0; i < times; i++) {
        setLights(false);
        delay(500);
        setLights(true);
        delay(500);
    }
    resetLights();    
}

void setup() {
        Serial.begin(115200);
        Homie_setFirmware(FIRMWARE_NAME, FIRMWARE_VERSION);
        #ifdef HOMIE_PIN
          Homie.setLedPin(HOMIE_PIN);
        #endif

        for (int i = 0; i < NOF_SWITHCES; i++) {
            buttonState[i]=0;
            on[i]=false;
            pinMode(RELAY_PIN[i],OUTPUT);
            digitalWrite(RELAY_PIN[i],LOW);
            pinMode(BUTTON_PIN[i],INPUT);
            button[i].attach(BUTTON_PIN[i]);
            button[i].interval(10);
        }
        switchNode[0].advertise("on").settable(switch0_OnHandler);
        #if NOF_SWITCHES >= 2
            switchNode[1].advertise("on").settable(switch1_OnHandler);
        #endif        
        Homie.disableResetTrigger();
        Homie.setup();

        #ifdef LED1_PIN
          pinMode(LED1_PIN,OUTPUT);
          digitalWrite(LED1_PIN,HIGH);
        #endif
}

void loop() {

    Homie.loop();

    for (int n = 0; n < NOF_SWITHCES; n++)  {
        button[n].update();
        if(button[n].fell()) {
                setSwitch(n,!on[n]);
                buttonMillis[n] = millis();
                buttonState[n] = 0;
        }
        if( button[n].read()==LOW) {
            if(buttonState[n]==0 && millis()-buttonMillis[n]>=1000) {
                //Pressed for more than 1 second, send command (state=1)
                Homie.getLogger() << "Button "<< n << " send long press command, state=" << buttonState[n] << endl;
                buttonState[n]=1;
            }
            else {
                //Reset and clear sequence
                for (int i = 1; i <= 12; i++) {
                    if(buttonState[n]==i && millis()-buttonMillis[n]>=(8000+(unsigned long)i*1000)) {
                        setLights((i % 2)==0);
                        Homie.getLogger() << "Button "<< n << " clear sequence, state=" << buttonState[n] << endl;
                        buttonState[n]++;
                    }
                }
            }
        }
        if(button[n].rose()) {
            Homie.getLogger() << "Button "<< n <<" released at state=" << buttonState[n] << endl;
            if(buttonState[n]==1) {
                switchNode[n].setProperty("broadcast_on").send(on[n] ? "true":"false");
                Homie.getLogger() << "Button "<< n << " broadcast " << (on[n] ? "on":"off");
            }
            if(buttonState[n]>=2 && buttonState[n]<10) {
                Homie.getLogger() << "Restart device" << endl;
                flashLights(2);
                ESP.reset();
            }
            if(buttonState[n]==11) {
                Homie.getLogger() << "Clear and restart device" << endl;
                flashLights(5);
                Homie.reset();
            }
            resetLights();    
        }
    }
}
