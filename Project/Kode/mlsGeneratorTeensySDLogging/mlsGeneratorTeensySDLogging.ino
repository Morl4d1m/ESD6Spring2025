#include <Audio.h>
#include <Wire.h>
#include <SD.h>         // Already included
#include <SPI.h>
#include <SerialFlash.h>

// SD Card setup
const int chipSelect = BUILTIN_SDCARD;   // Teensy Audio Shield SD
File logFile;                            // <-- Added for SD logging

// General audio preparation
AudioOutputI2S i2s1;
AudioControlSGTL5000 sgtl5000_1;
AudioMixer4 mixer;
AudioConnection patchCord5(mixer, 0, i2s1, 0);
AudioConnection patchCord6(mixer, 0, i2s1, 1);
AudioPlayQueue MLSSignal;
AudioConnection patchCord1(MLSSignal, 0, mixer, 0);
AudioSynthWaveform sineWave;
AudioConnection patchCord2(sineWave, 0, mixer, 1);
AudioSynthNoiseWhite whiteNoise;
AudioConnection patchCord3(whiteNoise, 0, mixer, 2);
AudioSynthToneSweep sineSweep;
AudioConnection patchCord4(sineSweep, 0, mixer, 3);

// Global variables
uint16_t count = 1;
const uint8_t ledPin = 13;
uint32_t LFSRBits = 25;
const unsigned long delayUS = 1;
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 1;

void setup() {
  AudioMemory(100);
  Serial.begin(115200);
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);
  sineWave.begin(WAVEFORM_SINE);
  pinMode(ledPin, OUTPUT);
  delay(1000);

  mixer.gain(0, 0.02);
  mixer.gain(1, 0.3);
  mixer.gain(2, 0.02);
  mixer.gain(3, 0.02);

  // Initialize SD card
  if (!SD.begin(chipSelect)) {                    // <-- Added for SD logging
    Serial.println("SD card initialization failed!"); 
  } else {
    logFile = SD.open("BitMLS.txt", FILE_WRITE);
    if (logFile) {
      logFile.println("Starting new log session...");
      logFile.flush();
    } else {
      Serial.println("Failed to open log file!");
    }
  }

  Serial.println("Tester");
  if (logFile) { logFile.println("Tester"); logFile.flush(); }   // <-- Added for SD logging

  //generateMLS();
}

void loop() {
  generateMLS();
  
  if (LFSRBits < 32) { // Looping through various lengths to listen to different MLS's
    LFSRBits++;
  } else if (LFSRBits >= 32) {
    LFSRBits = 2;
    return;
  delay(1000);
}}

void wait(unsigned int milliseconds) {
  elapsedMillis msec = 0;
  while (msec <= milliseconds) {
    digitalWrite(ledPin, HIGH);
    delay(10);
    digitalWrite(ledPin, LOW);
    delay(20);
  }
}

uint32_t feedbackTaps(uint8_t bits) {
  switch (bits) {
    case 2: return (1 << 1) | (1 << 0);
    case 3: return (1 << 2) | (1 << 0);
    case 4: return (1 << 3) | (1 << 0);
    case 5: return (1 << 4) | (1 << 2);
    case 6: return (1 << 5) | (1 << 4);
    case 7: return (1 << 6) | (1 << 5);
    case 8: return (1 << 7) | (1 << 5) | (1 << 4) | (1 << 3);
    case 9: return (1 << 8) | (1 << 4);
    case 10: return (1 << 9) | (1 << 6);
    case 11: return (1 << 10) | (1 << 8);
    case 12: return (1 << 11) | (1 << 10) | (1 << 4) | (1 << 1);
    case 13: return (1 << 12) | (1 << 3) | (1 << 2) | (1 << 0);
    case 14: return (1 << 13) | (1 << 11) | (1 << 9) | (1 << 8);
    case 15: return (1 << 14) | (1 << 13);
    case 16: return (1 << 15) | (1 << 13) | (1 << 12) | (1 << 10);
    case 17: return (1 << 16) | (1 << 13);
    case 18: return (1 << 17) | (1 << 10);
    case 19: return (1 << 18) | (1 << 5);
    case 20: return (1 << 19) | (1 << 16);
    case 21: return (1 << 20) | (1 << 18);
    case 22: return (1 << 21) | (1 << 20);
    case 23: return (1 << 22) | (1 << 17);
    case 24: return (1 << 23) | (1 << 22) | (1 << 21) | (1 << 16);
    case 25: return (1 << 24) | (1 << 22);
    case 26: return (1 << 25) | (1 << 6) | (1 << 2) | (1 << 1);
    case 27: return (1 << 26) | (1 << 4) | (1 << 1) | (1 << 0);
    case 28: return (1 << 27) | (1 << 24);
    case 29: return (1 << 28) | (1 << 26);
    case 30: return (1 << 29) | (1 << 5) | (1 << 3) | (1 << 0);
    case 31: return (1 << 30) | (1 << 27);
    case 32: return (1 << 31) | (1 << 21) | (1 << 1) | (1 << 0);
    default:
      Serial.println("Unsupported bit length!");
      if (logFile) { logFile.println("Unsupported bit length!"); logFile.flush(); }
      return 0;
  }
}

void playMLSBit(bool bit, int samplesPerBit = 1, int amplitude = 28000) {
  static int16_t buffer[128];
  static int bufferIndex = 0;

  int16_t value = bit ? amplitude : -amplitude;

  for (int i = 0; i < samplesPerBit; i++) {
    buffer[bufferIndex++] = value;

    if (bufferIndex == 128) {
      memcpy(MLSSignal.getBuffer(), buffer, sizeof(buffer));
      MLSSignal.playBuffer();
      bufferIndex = 0;
    }
  }
}

void generateMLS() {
  if (LFSRBits < 2 || LFSRBits > 32) {
    Serial.println("LFSRBits must be between 2 and 32.");
    if (logFile) { logFile.println("LFSRBits must be between 2 and 32."); logFile.flush(); }
    while (1);
  }

  mask = (1UL << LFSRBits) - 1;
  LFSR = mask;
  uint32_t taps = feedbackTaps(LFSRBits);

  Serial.print("Generating MLS with ");
  Serial.print(LFSRBits);
  Serial.println(" bits:");
  if (logFile) {
    logFile.print("Generating MLS with ");
    logFile.print(LFSRBits);
    logFile.println(" bits:");
    logFile.flush();
  }

  Serial.print("The MLS should be ");
  Serial.print((1UL << LFSRBits) - 1);
  Serial.println(" bits long.");
  if (logFile) {
    logFile.print("The MLS should be ");
    logFile.print((1UL << LFSRBits) - 1);
    logFile.println(" bits long.");
    logFile.flush();
  }

  uint32_t startMLSTime = micros();
  uint32_t MLSLength = (1UL << LFSRBits) - 1;
  uint32_t increment = 1048576;

  for (uint32_t start = 0; start < MLSLength; start += increment) {
    uint32_t end = start + increment;
    if (end > MLSLength) {
      end = MLSLength;
    }

    for (uint32_t i = start; i < end; i++) {
      bool feedback = __builtin_parity(LFSR & taps);
      
      //Serial.print(feedback ? 1 : 0);
      if (logFile) { logFile.print(feedback ? 1 : 0); }

      playMLSBit(feedback);

      LFSR <<= 1;
      if (feedback) {
        LFSR |= 1;
      }
      LFSR &= mask;
    }

    Serial.println("Next");
    //if (logFile) { logFile.println("Next"); logFile.flush(); }
  }

  uint32_t endMLSTime = micros();
  Serial.println("\nMLS generation complete.");
  if (logFile) { logFile.println("\nMLS generation complete."); logFile.flush(); }

  Serial.print("It has taken ");
  Serial.print(endMLSTime - startMLSTime);
  Serial.println(" microseconds to calculate and print.");
  if (logFile) {
    logFile.print("It has taken ");
    logFile.print(endMLSTime - startMLSTime);
    logFile.println(" microseconds to calculate and print.");
    logFile.flush();
  }
}
