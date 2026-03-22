#include <Arduino.h>
#include <ArduinoJson.h>

void setup() {
  // Initialize Serial communication at the agreed upon baud rate
  Serial.begin(115200);
  
  // Wait for the serial port to connect (necessary for native USB boards, good practice overall)
  while (!Serial) {
    ; 
  }
}

void loop() {
  // State 0: Passive Listening Loop
  if (Serial.available() > 0) {
    // Read the incoming string until a newline character is detected
    String incomingMessage = Serial.readStringUntil('\n');
    incomingMessage.trim(); // Remove trailing whitespace or carriage returns

    // Ensure the message is not empty before processing
    if (incomingMessage.length() > 0) {
      
      // Initialize a JSON Document to hold the incoming data
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, incomingMessage);

      // Error Handling: If Apex sends malformed JSON
      if (error) {
        Serial.println("{\"error\": \"Invalid JSON received by Micromax\"}");
        return; // Exit the current loop iteration
      }

      // Extract the command
      String command = doc["command"];

      // State 1: Discovery & Handshake Execution
      if (command == "ping") {
        // Construct the reply payload
        JsonDocument replyDoc;
        replyDoc["status"] = "ready";
        replyDoc["id"] = "micromax_uno_01";
        
        // Add dynamic capabilities as a JSON array
        JsonArray capabilities = replyDoc["capabilities"].to<JsonArray>();
        capabilities.add("serial");
        capabilities.add("gpio");
        capabilities.add("adc"); // Analog-to-Digital Converter

        // Serialize the JSON back into a string and send it over Serial
        serializeJson(replyDoc, Serial);
        Serial.println(); // Send a newline character so Python knows the message is complete
      } 
      else {
        // Handle unrecognized commands
        Serial.println("{\"error\": \"Unknown command\"}");
      }
    }
  }
}