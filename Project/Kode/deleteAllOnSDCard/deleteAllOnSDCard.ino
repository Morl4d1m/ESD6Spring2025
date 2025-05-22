#include <SD.h>
#include <SPI.h>

const int chipSelect = BUILTIN_SDCARD; // Use BUILTIN_SDCARD for Teensy 3.5/3.6/4.x
// For external module, set appropriate CS pin, e.g., const int chipSelect = 10;

void deleteRecursive(File dir);

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for Serial Monitor
  }

  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  File root = SD.open("/");
  if (root) {
    deleteRecursive(root);
    root.close();
    Serial.println("All matching files deleted.");
  } else {
    Serial.println("Failed to open root directory.");
  }
}

void loop() {
  // Do nothing
}

void deleteRecursive(File dir) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;  // No more files in the directory
    
    if (entry.isDirectory()) {
      deleteRecursive(entry);  // Recursively go into subdirectories
      SD.rmdir(entry.name());   // Remove the empty directory
      Serial.print("Deleted folder: ");
      Serial.println(entry.name());
    } else {
      // Check if the file ends with "IRAndFFT.csv"
      String fileName = entry.name();
      if (fileName.endsWith("IRAndFFT.csv")) {
        SD.remove(entry.name());  // Delete the matching file
        Serial.print("Deleted file: ");
        Serial.println(entry.name());
      }
    }
    entry.close();
  }
}
