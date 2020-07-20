#include "common.h"

std::unique_ptr<ConnectorClient> CC;
std::vector<std::shared_ptr<Device>> DEVICES;

DynamicJsonDocument handleRequest(String h, JsonObject content);

void setup() {
  IPAddress CONNECTOR_IP(192, 168, 1, 13);  
  const int CONNECTOR_PORT = 7777;

  CC = std::unique_ptr<ConnectorClient>(new ConnectorClient(CONNECTOR_IP, CONNECTOR_PORT));
  
  std::vector<int> states = {0, 1};
  DEVICES.emplace_back(std::make_shared<SwitchableDevice>("Arya", "simple lights", 14, states));

  setupEnv();
  CC->connect();
  CC->initializeSession();
  CC->setupDevices(DEVICES);
}

// the loop function runs over and over again forever
void loop() {
    while (true) {
      DynamicJsonDocument req(256);
      CC->readPayload(req);
      auto res = handleRequest(req);
      CC->sendPayload(res);
      delay(100);
    }
}

DynamicJsonDocument handleRequest(DynamicJsonDocument req) {
  DynamicJsonDocument res(256);

  if (req["header"] == "switch_device_state_request") {
    Serial.println("Switching device state");
    auto state = req["content"]["state"].as<int>();
    auto name = req["content"]["device_name"].as<String>();
    // TODO: validate name
    std::static_pointer_cast<SwitchableDevice>(DEVICES[0])->switchState(state);
  }

  res["header"] = "ack_response";
  res.createNestedObject("content");

  return res;
}

    
