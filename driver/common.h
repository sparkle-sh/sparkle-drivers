#include <vector>
#include <memory>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

void setupEnv() {
  Serial.begin(115200);
  Serial.println();

  WiFi.begin("", "");

  while (WiFi.status() != WL_CONNECTED) {
//      Serial.println(F("Connecting"));
      delay(500);   
  }
  Serial.println(F("Connected"));
//  Serial.print(F("Driver IP address: "));
  Serial.println(WiFi.localIP());
}

enum class DeviceType {
  SWITCHABLE = 0, SENSOR = 1
};

class Device {
public:
  Device(DeviceType type, String nname, String description) 
    : m_type(type), m_name(std::move(nname)), m_description(std::move(description))
  {
  }

  DeviceType getType() const {
    return m_type;
  }

  String getName() const {
    return m_name;
  }

  String getDescription() const {
    return m_description;
  }

  virtual DynamicJsonDocument serialize() = 0;
protected:
  DeviceType m_type;
  String m_name;
  String m_description;
};

class SensorDevice : public Device {
public:  
  DynamicJsonDocument serialize() {
    
  }
};

class SwitchableDevice : public Device {
public:
  SwitchableDevice(String nname, String description, int pin, std::vector<int> states) 
    : Device(DeviceType::SWITCHABLE, std::move(nname), std::move(description)), m_pin(pin), m_states(states)
  {
    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, LOW);
  }

  void switchState(int state) {
     digitalWrite(m_pin, state == 1 ? HIGH : LOW);
  }
  
  std::vector<int> getStates() const {
    return m_states;
  }

  DynamicJsonDocument serialize() {
    DynamicJsonDocument doc(512);

    doc["description"] = m_description.c_str(); // this is thrash in payload
    doc["name"] = m_name.c_str();
    doc["type"] = (int)m_type;

    for (int i = 0; i < m_states.size(); ++i)
      doc["datasheet"]["states"][i] = m_states[i];
    
    return doc;
  }
  
private:
  int m_pin;
  std::vector<int> m_states;
  
};

class ConnectorClient {
public:
  ConnectorClient(IPAddress ip, int port) 
    : m_ip(ip),
      m_port(port)
  {
  }

  bool connect() {
    return m_client.connect(m_ip, m_port); 
  }

  void setupDevices(const std::vector<std::shared_ptr<Device>>& devices) {
    Serial.println("Registering devices");
    
    DynamicJsonDocument doc(256);
    doc["header"] = "register_devices_request";

    int i = 0;
    for (const auto& device : devices) {
        auto s = device->serialize();
        doc["content"]["devices"][i++] = s;
    }
    
    sendPayload(doc);
    readPayload(doc);

    if (doc["header"] != "ack_response") {
        Serial.println(F("Could not register devices"));
        exit(-3);
    }

    Serial.println(F("Devices registered"));
  }


  void initializeSession() {
    DynamicJsonDocument doc(128);
    doc["header"] = "handshake_request";
    doc["content"]["session_type"] = "driver";

    sendPayload(doc);
    readPayload(doc);
  
    if (doc["header"] != "handshake_response") {
        Serial.println(F("Received invalid handshake, quitting"));
        exit(-2);
    }
    Serial.println(F("Session initialized"));
  
  }

  void sendPayload(DynamicJsonDocument payload) {
    char json[256];
    memset(json, 0, 256);
    serializeJson(payload, json, 256);

    uint32_t len = strlen(json);
    char lenBuff[4];
    memset(lenBuff, 0, 4);
    memcpy(lenBuff, &len, 4);
    
    Serial.println(json);
//    Serial.println(len);
    
    m_client.write(lenBuff, 4);
    m_client.write(json, len);
  }


  void readPayload(DynamicJsonDocument& payload) {
    char lenBuff[4];
    char bytes[128];
    memset(bytes, 0, 128);
    int i = 0;
    Serial.println(F("reading payload length"));
    while (i < 4) 
        if (m_client.available())
            lenBuff[i++] = m_client.read();
    
    uint32_t* len = (uint32_t*)lenBuff;
//    Serial.println(*len);
//    Serial.println(F("reading payload"));
  
    i = 0;
    while (i <= *len - 1) 
        if (m_client.available()) 
            bytes[i++] = m_client.read();

    Serial.println(bytes);
    deserializeJson(payload, String(bytes));
}

private:
  WiFiClient m_client;
  IPAddress m_ip;
  int m_port;

};
