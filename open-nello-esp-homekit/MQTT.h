#include <AsyncMqttClient.h>
#include <WiFi.h>
#include "Config.h"
#include "Encryption.h"
#include "esp_log.h"

WiFiClient espClient;

class NelloMQTTClient {
  public:
    NelloMQTTClient() {
      LOG0("Initializing Nello MQTT Client\n");
      crypto = new EncryptionContext();
      client = new AsyncMqttClient();
      static NelloMQTTClient* ref = this;
      client->onMessage([](char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) { ref->handleIncomingMessage(topic, payload); });
      client->onConnect([](bool sessionPresent) { ref->onConnect(); });
      LOG0("Nello MQTT Client Ready, waiting for WifiCallback to connect\n");
    }
    void setRingCallback(void(*cb)(int)) {
      ringCallback = cb;
    }
    void setWifiReady() {
      wifiReady = true;
      initTimeSync();
    }
    void openDoor() {
      sendMessage(mqtt_topic_door, "000000"); // TODO: handle command id's correctly
    }
    void loop(){
      bool clientIsConnected = client->connected();
      if (initialized && !clientIsConnected) {
        LOG0("Reconnecting\n");
        initialized = false;
        connectToBroker();
      }
      if (wifiReady && !initialized) {
        LOG0("Wifi Ready - Connecting\n");
        connectToBroker();
      }
      
    }
  private:
    AsyncMqttClient* client;
    EncryptionContext* crypto;
    bool initialized = false;
    bool wifiReady = false;
    uint32_t waitTime=10000;
    void(*ringCallback)(int);

    void handleIncomingMessage(char* topic, char* payload) {
      LOG0("Message arrived [");
      LOG0(topic);
      LOG0("]:\n");
      payload[strlen(payload)-1] = '\0'; // remove last character (newline)
      String decrypted = crypto->decrypt(String(payload));
      if (strcmp(topic, mqtt_topic_ring) == 0){
        LOG0("Ring received\n");
        if (ringCallback != NULL) {
          ringCallback(3); // TODO: Handle Code
        }       
      }
      if (strcmp(topic, mqtt_topic_map) == 0){
        handleMap();     
      }
      if (strcmp(topic, mqtt_topic_nonline) == 0){
        handleNOnline();     
      }
    }

    void onConnect(){
          LOG0("Successfully connected to broker ");
          LOG0(mqtt_hostname);
          LOG0("\n");
          initialized = true;
          LOG0("Subscribing to Client Topics:\n");
          subscribeTo(mqtt_topic_map);
          subscribeTo(mqtt_topic_nack);
          subscribeTo(mqtt_topic_ntobe);
          subscribeTo(mqtt_topic_nonline);
          subscribeTo(mqtt_topic_ring);
    }

    void handleMap(){
      sendMessage(mqtt_topic_test, "0;10;0;0;0;0;0;0;");
    }

     void handleNOnline(){
      sendMessage(mqtt_topic_beack, "0;0;");
    }

    void subscribeTo(char* topic) {
        if (client->subscribe(topic, mqtt_qos)) {
          LOG0("Successfully subscribed to topic [");
          LOG0(topic);
          LOG0("]\n");
        } else {
          LOG0("Subscription to topic [");
          LOG0(topic);
          LOG0("] failed\n");
        }
    }

    void connectToBroker(){
      client->setClientId("open-nello-homekit");
      client->setServer(mqtt_hostname, 1883);
      LOG0("Connecting to MQTT Broker\n");
      client->connect();
    }

    void sendMessage(char* topic, char* command){
      String clearTextPayload = String(command);
      clearTextPayload.concat("@");
      clearTextPayload.concat(getTime());
      clearTextPayload.concat("@");
      String payload = crypto->encrypt(clearTextPayload);
      payload = String(payload + "\n");
      LOG0(payload.c_str());
      LOG0("sending...\n\n");
      client->publish(topic, 1, false, payload.c_str(), 0, false);
    }

    unsigned long long getTime() {
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo, 100)) {
        LOG0("Failed to get current time");
        return(0);
      }
      time_t now;
      time(&now);
      return 1000LL * now; // convert to millis
    }

    void initTimeSync() {
      LOG0("Sync time via NTP with %s (%s).  Waiting %d seconds for result... ", ntp_server, timezone, waitTime/1000);
      configTzTime(timezone, ntp_server);
      struct tm timeinfo;
      if(getLocalTime(&timeinfo,waitTime)){
        char bootTime[33]="Unknown"; 
        strftime(bootTime,sizeof(bootTime),"%c",&timeinfo);
        LOG0("Time received: %s\n\n",bootTime);
      } else {
        LOG0("Failed syncing time\n\n");
      }
    }
};
