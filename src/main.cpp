#include "Arduino.h"
#include "Homie.h"
#include "Bounce2.h"

#define FIRMWARE_VERSION "0.9.6"
/* Revision history:
   0.9.6 Switched to Homie 2.0.0
   0.9.5 Removed ArduinoOTA and relying on Homie OTA to save space... (Homie 2.0.0-beta.3)
   0.9.4 Changed led behaviour
*/

#ifdef SONOFF
  #define FIRMWARE_NAME "Sonoff"
  #define BUTTON1_PIN 0
  //GPIO0=D3
  #define RELAY1_PIN  12
  //GPIO12=D6
  #define HOMIE_PIN  13, LOW
#endif

#ifdef WIFISWITCH2
  #define FIRMWARE_NAME "WifiSwitch2"
  #define BUTTON1_PIN 0
  //GPIO0=D3
  #define RELAY1_PIN  12
  //GPIO12=D6
  #define BUTTON2_PIN 14
  //GPIO14=D5 
  #define RELAY2_PIN  13
  //GPIO13=D7  
#endif

 
HomieNode switchNode1("switch1", "switch");
Bounce button1 = Bounce();
bool on1 = false;

void set_switch1() {
  digitalWrite(RELAY1_PIN, on1 ? HIGH : LOW);
  #ifdef LED1_PIN
    digitalWrite(LED1_PIN, on1 ? HIGH : LOW);
  #endif
  switchNode1.setProperty("on").send(on1 ? "true":"false");
  Homie.getLogger() << "Switch1 is " << (on1 ? "on" : "off") << endl;
}

bool switch1OnHandler(const HomieRange& range, const String& value) {
  if (value != "true" && value != "false" && value != "1" && value != "0") return false;
  on1 = (value == "true" || value =="1");
  set_switch1();
  return true;
}


#ifdef BUTTON2_PIN
  HomieNode switchNode2("switch2", "switch");
  Bounce button2 = Bounce();
  bool on2 = false;

  void set_switch2() {
    digitalWrite(RELAY2_PIN, on2 ? HIGH : LOW);
    switchNode2.setProperty("on").send(on2 ? "true":"false");
    Homie.getLogger() << "Switch2 is " << (on2 ? "on" : "off") << endl;
  }
  bool switch2OnHandler(const HomieRange& range, const String& value) {
    if (value != "true" && value != "false" && value != "1" && value != "0") return false;
    on2 = (value == "true" || value =="1");
    set_switch2();
    return true;
  }
#endif




void setup() {
        Serial.begin(115200);
        Homie_setFirmware(FIRMWARE_NAME, FIRMWARE_VERSION);
        #ifdef HOMIE_PIN
          Homie.setLedPin(HOMIE_PIN);
        #endif
        Homie.setResetTrigger(BUTTON1_PIN, LOW, 30000);
        Homie.setup();

        pinMode(RELAY1_PIN,OUTPUT);
        digitalWrite(RELAY1_PIN,LOW);
        pinMode(BUTTON1_PIN,INPUT);
        button1.attach(BUTTON1_PIN);
        button1.interval(10);
        switchNode1.advertise("on").settable(switch1OnHandler);
        #ifdef LED1_PIN
          pinMode(LED1_PIN,OUTPUT);
          digitalWrite(LED1_PIN,HIGH);
        #endif

        #ifdef BUTTON2_PIN
          pinMode(RELAY2_PIN,OUTPUT);
          digitalWrite(RELAY2_PIN,LOW);
          pinMode(BUTTON2_PIN,INPUT);
          button2.attach(BUTTON2_PIN);
          button2.interval(10);
          switchNode2.advertise("on").settable(switch2OnHandler);
        #endif
        
}

void loop() {

    Homie.loop();

    button1.update();
    if(button1.fell()) {
            on1=!on1;
            set_switch1();
    }

    #ifdef RELAY2_PIN
        button2.update();
        if(button2.fell()) {
                on2=!on2;
                set_switch2();
        }
    #endif
}
