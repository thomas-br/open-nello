#include "HomeSpan.h"
#include "MQTT.h"

class ManualOpeningSwitch : public Service::Switch {
  public: 
    ManualOpeningSwitch() : Service::Switch() {
      state=new Characteristic::On();
      name=new Characteristic::Name("Manuelle Öffnung");
    }

    void setOpenDoorDelegate(void(*cb)()) {
      openDoorDelegate = cb;
    }

    void loop(){
      if(state->getVal() && state->timeVal()>2000) {
        state->setVal(false);
      }      
    }
    
    boolean update() {
      if (state->getNewVal() && openDoorDelegate != NULL) {
        LOG0("Sending manual door opening command now\n");
        openDoorDelegate();
      }
      return true;
    }

  private:
    SpanCharacteristic *state;
    SpanCharacteristic *name;
    void(*openDoorDelegate)();
};

class OpenNello : public Service::Doorbell
{
  public:
    OpenNello() : Service::Doorbell() {
      LOG0("Initializing Open Nello\n");
      mqttClient = new NelloMQTTClient();
      switchEvent=new Characteristic::ProgrammableSwitchEvent();
      doorOpener=new ManualOpeningSwitch();
      addLink(doorOpener);
      static OpenNello* ref = this;
      doorOpener->setOpenDoorDelegate(
        [](){
          ref->openDoor();
        }
      );
      mqttClient->setRingCallback(
        [](int code) {
          ref->handleRing(code);
        }
      );
      LOG0("Open Nello ready\n");
    }
    void wifiCallback(){
      mqttClient->setWifiReady();
    }
    void loop(){
      mqttClient->loop();
    }
    void openDoor() {
      mqttClient->openDoor();
    }
  private:
    SpanCharacteristic *switchEvent;
    ManualOpeningSwitch *doorOpener;
    NelloMQTTClient *mqttClient;
    
    void handleRing(int code) {
      LOG0("Ring Singal detected, persisting to HomeKit\n");
      switchEvent->setVal(0);
    }
};

void setup() {  
  Serial.begin(115200);
  homeSpan.begin(Category::Doors, "Open Nello");
  new SpanAccessory(); 
    new Service::AccessoryInformation();  
      new Characteristic::Identify(); 
      new Characteristic::Name("Open Nello");
  static OpenNello* nello = new OpenNello();  

  homeSpan.setWifiCallback([]() { nello->wifiCallback(); });
}

void loop(){
  homeSpan.poll();
}
