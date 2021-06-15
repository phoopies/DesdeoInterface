#include <CRC8.h>
#include <Potentiometer.h>
#include <Button.h>
#include <RotaryEncoder.h>
#define PJON_INCLUDE_MAC
#include <PJONSoftwareBitBang.h>
#include <ArduinoJson.h>

const uint8_t crcKey = 7;
CRC8 crc = CRC8(crcKey); // Crc4 vs paritycheck

PJONSoftwareBitBang bus;
StaticJsonDocument<512> doc; //512 is the RAM allocated to this document.
JsonArray rotary = doc["master"].createNestedArray("Rotary");

const int communicationPin = 12;
const int outputPin = 11;
const int inputPin = 12;

// Make a own class for component making and checking
// Modulemaker or something

// Have this serial sending as it's own class also?

// Master could have one internal component module
Button buttons[2] = {Button(2,0), Button(3,1)};
RotaryEncoder rotEncoder = RotaryEncoder(8,9,0);

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &info) {
    String id;
    for (int i = 0; i < 6; ++i) {
      id += info.tx.mac[i];
    }
    bus.reply("A", 1);
    String type;
    type +=char(payload[0]);
    uint8_t componentId = (uint8_t)(payload[1]);

    if (type == "B") {
      uint16_t value = (uint16_t)(payload[2]);
      doc[id][type][String(componentId)] = value;
    }
    else if (type == "P") {
      uint16_t value = ((uint16_t)(payload[2] << 8) | (uint16_t)(payload[3] & 0xFF));
      doc[id][type][String(componentId)] = value;
    }
    else if (type == "R") {
      JsonArray slaveRotary = doc[type][id] ;
      if (slaveRotary.isNull()) {slaveRotary = doc[id][type].createNestedArray(String(componentId));}
      slaveRotary[0] = payload[2];
      slaveRotary[1] = payload[3];
    }
    else {return;}
    writeJsonToSerial();
};

uint8_t getComponentCount(String id) {
    uint8_t componentCount = 0;
    JsonObject node = doc[id].as<JsonObject>();
    for (JsonPair kv : node) {
      String compType = kv.key().c_str();
      componentCount += doc[String(id)][compType].size();
    }
    return componentCount;
}

void writeJsonToSerial(){
   String json;
   serializeJson(doc, json);
   int len = json.length() + 1;
   uint8_t data[len];
   json.getBytes(data, len);
   uint8_t crc8 = crc.getCRC8(data, len - 1); // getBytes -> last is nul
   Serial.print(json); // print data
   Serial.println(crc8); // Add crc checksum 
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {;}
  bus.set_id(0);
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
};

void loop() {
  uint8_t values[2];
  rotEncoder.getValues(values);
  rotary[0] = values[0];
  rotary[1] = values[1];

  doc["master"]["Accept"] = buttons[0].getValue();
  doc["master"]["Decline"] = buttons[1].getValue();
  
  if (rotEncoder.hasChanged() || buttons[0].hasChanged() || buttons[1].hasChanged()) { writeJsonToSerial(); }

  bus.receive(1000);
};