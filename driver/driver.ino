
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

WiFiClient c;

IPAddress server(192, 168, 1, 13);
const int port = 7777;

#define SWITCHABLE_DEVICE 0
#define SENSOR_DEVICE     1

typedef struct t_device {
    int type;
    char name[16];
    char description[32];
} Device;


 
void send_payload(DynamicJsonDocument payload);
void read_payload(DynamicJsonDocument* payload);
void setup_env();
void setup_devices();

#define DEVICES_LEN 1

static Device DEVICES[DEVICES_LEN];


void setup() {
    setup_env();
    initialize_session();
    setup_devices();
    pinMode(1, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
//    while (1) {
//        DynamicJsonDocument doc(256);
//        read_payload(&doc);

      
//    }
    

    delay(2500);  
    digitalWrite(1, HIGH);
    delay(2500);
    digitalWrite(1, LOW);
}

void setup_devices() {
    Serial.println("Registering devices");
  
    Device* dev = &DEVICES[0];

    memset(dev->name, 0, 16);
    memset(dev->description, 0, 32);
    dev->type = SWITCHABLE_DEVICE;
    sprintf(dev->name, "light");
    sprintf(dev->description, "testing light");

    DynamicJsonDocument doc(256);
    doc["header"] = "register_devices_request";

    int i;
    for (i = 0; i < DEVICES_LEN; ++i) {
        dev = &DEVICES[0];
        doc["content"]["devices"][i]["type"] = dev->type;
        doc["content"]["devices"][i]["name"] = dev->name;
        doc["content"]["devices"][i]["description"] = dev->description;
    }

    send_payload(doc);
    read_payload(&doc);

    if (doc["header"] != "ack_response") {
        Serial.println("Could not register devices");
        exit(-3);
    }

    Serial.println("Devices registered");
}

void initialize_session() {
    DynamicJsonDocument doc(256);
    doc["header"] = "handshake_request";
    doc["content"]["session_type"] = "driver";

    send_payload(doc);
    read_payload(&doc);

    if (doc["header"] != "handshake_response") {
        Serial.println("Received invalid handshake, quitting");
        exit(-2);
    }
    Serial.println("Session initialized");
  
}

void send_payload(DynamicJsonDocument payload) {
    char json[256];
    memset(json, 0, 256);
    serializeJson(payload, json, 256);

    uint32_t len = strlen(json);
    char lenBuff[4];
    memset(lenBuff, 0, 4);
    memcpy(lenBuff, &len, 4);
    
    Serial.println(json);
    Serial.println(len);
    
    c.write(lenBuff, 4);
    c.write(json, len);
  
}

void read_payload(DynamicJsonDocument* payload) {
    char lenBuff[4];
    char bytes[256];
    memset(bytes, 0, 256);
    int i;
    while (i < 4) {
        if (c.available()) {
            lenBuff[i++] = c.read();
        }
    }

    uint32_t* len = (uint32_t*)lenBuff;

    i = 0;
    while (i <= *len - 1) {
        if (c.available()) {
            bytes[i++] = c.read();
        }
    }
    deserializeJson(*payload, bytes);
}

void setup_env() {
    Serial.begin(115200);
    Serial.println();

    WiFi.begin("", "");

    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    if (!c.connect(server, port)) {
        
    } 
    Serial.println("Connected");
}
