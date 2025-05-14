//===============================================================================
//  Find Optional Memory Chips on Teensy 4.1
//===============================================================================
#include "LittleFS.h"
extern "C" uint8_t external_psram_size;

//===============================================================================
//  Initialization
//===============================================================================
void setup() {
  Serial.begin(115200);       //Initialize USB serial port to computer

  // Check for PSRAM chip(s) installed
  uint8_t size = external_psram_size;
  Serial.printf("PSRAM Memory Size = %d Mbyte\n", size);
  if (size == 0) {
    Serial.println ("No PSRAM Installed");
  }

// Check for either NOR or NAND Flash chip installed
  LittleFS_QSPIFlash myfs_NOR;    // NOR FLASH
  LittleFS_QPINAND myfs_NAND;      // NAND FLASH 1Gb

  // Check for NOR Flash chip installed
  if (myfs_NOR.begin()) {
    Serial.printf("NOR Flash Memory Size = %d Mbyte / ", myfs_NOR.totalSize() / 1048576);
    Serial.printf("%d Mbit\n", myfs_NOR.totalSize() / 131072);
  }
  // Check for NAND Flash chip installed
  else if (myfs_NAND.begin()) {
    Serial.printf("NAND Flash Memory Size =  %d bytes / ", myfs_NAND.totalSize());
    Serial.printf("%d Mbyte / ", myfs_NAND.totalSize() / 1048576);
    Serial.printf("%d Gbit\n", myfs_NAND.totalSize() * 8 / 1000000000);
  }
  else {
    Serial.printf("No Flash Installed\n");
  }
}
void loop() {
  // put your main code here, to run repeatedly:

}