// Define the built-in LED pin (for Teensy 4.1, it's usually pin 13)
const int ledPin = 13;  

void setup() {
  // Start serial communication
  Serial.begin(115200);
  
    // Initialize the built-in LED pin as an output
  pinMode(ledPin, OUTPUT);
  
  // Print a welcome message
  Serial.println("Teensy 4.1 Command Receiver Initialized.");
  Serial.println("Use commands to control the built-in LED and print messages.");
  Serial.println("Commands: ON, OFF, PRINT");
  digitalWrite(ledPin, HIGH);
}

void loop() {
  // Check if data is available to read from serial monitor
  if (Serial.available() > 0) {
    // Read the incoming data (command) as a string
    String command = Serial.readStringUntil('\n');
    
    // Clean up the string (remove trailing newline)
    command.trim();

    // Check which command was received and perform the corresponding action
    if (command == "ON") {
      digitalWrite(ledPin, HIGH);  // Turn the LED on
      Serial.println("LED is now ON.");
    } 
    else if (command == "OFF") {
      digitalWrite(ledPin, LOW);   // Turn the LED off
      Serial.println("LED is now OFF.");
    }
    else if (command == "PRINT") {
      // Send a serial message
      Serial.println("This is a message from Teensy 4.1!");
    }
    else {
      Serial.println("Unknown command. Try 'ON', 'OFF', or 'PRINT'.");
    }
  }
}
