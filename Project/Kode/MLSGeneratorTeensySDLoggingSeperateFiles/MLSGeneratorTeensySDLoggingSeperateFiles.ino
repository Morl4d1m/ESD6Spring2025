#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

// General audio preparation
AudioOutputI2S i2s1;
AudioControlSGTL5000 sgtl5000_1;

// Mixer
AudioMixer4 mixer;
AudioConnection patchCord5(mixer, 0, i2s1, 0);
AudioConnection patchCord6(mixer, 0, i2s1, 1);

// Signals
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
uint32_t LFSRBits = 27;
const unsigned long delayUS = 1;
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 1;

File logFile;  // <---- Added for SD card logging

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

  if (!SD.begin(BUILTIN_SDCARD)) {  // <- or SD.begin(CS_PIN) depending on your setup
    Serial.println("SD card failed or not present!");
    while (1)
      ;
  }
  Serial.println("SD card initialized.");
  //generateMLS();
}

void loop() {
  wait(1000);
  generateMLS();

  if (LFSRBits < 30) {
    LFSRBits++;
  } else if (LFSRBits >= 30) {
    wait(36000000);
    return;
  }
  delay(1000);
}

void wait(unsigned int milliseconds) {
  elapsedMillis msec = 0;
  while (msec <= milliseconds) {
    digitalWrite(ledPin, HIGH);
    delay(10);
    digitalWrite(ledPin, LOW);
    delay(20);
  }
}

// Feedback tap map for various left-shifting LFSR lengths (primitive polynomials) with correct taps
uint32_t feedbackTaps(uint8_t bits) {
  switch (bits) {
    case 2: return (1 << 1) | (1 << 0);                             // x^2 + x + 1
    case 3: return (1 << 2) | (1 << 0);                             // x^3 + x + 1
    case 4: return (1 << 3) | (1 << 0);                             // x^4 + x + 1
    case 5: return (1 << 4) | (1 << 2);                             // x^5 + x^3 + 1
    case 6: return (1 << 5) | (1 << 4);                             // x^6 + x^5 + 1
    case 7: return (1 << 6) | (1 << 5);                             // x^7 + x^6 + 1
    case 8: return (1 << 7) | (1 << 5) | (1 << 4) | (1 << 3);       // x^8 + x^6 + x^5 + x^4 + 1
    case 9: return (1 << 8) | (1 << 4);                             // x^9 + x^5 + 1
    case 10: return (1 << 9) | (1 << 6);                            // x^10 + x^7 + 1
    case 11: return (1 << 10) | (1 << 8);                           // x^11 + x^9 + 1
    case 12: return (1 << 11) | (1 << 5) | (1 << 3) | (1 << 0);     // x^12 + x^6 + x^4 + x + 1
    case 13: return (1 << 12) | (1 << 3) | (1 << 2) | (1 << 0);     // x^13 + x^4 + x^3 + x + 1
    case 14: return (1 << 13) | (1 << 12) | (1 << 11) | (1 << 1);   // x^14 + x^13 + x^3 + x 1
    case 15: return (1 << 14) | (1 << 13);                          // x^15 + x^14 + 1
    case 16: return (1 << 15) | (1 << 13) | (1 << 12) | (1 << 10);  // x^16 + x^14 + x^13 + x^11 + 1
    case 17: return (1 << 16) | (1 << 13);                          // x^17 + x^14 + 1
    case 18: return (1 << 17) | (1 << 10);                          // x^18 + x^11 + 1
    case 19: return (1 << 18) | (1 << 17) | (1 << 16) | (1 << 13);  // x^19 + x^18 + x^16 + x^14 + 1
    case 20: return (1 << 19) | (1 << 16);                          // x^20 + x^17 + 1
    case 21: return (1 << 20) | (1 << 18);                          // x^21 + x^19 + 1
    case 22: return (1 << 21) | (1 << 20);                          // x^22 + x^21 + 1
    case 23: return (1 << 22) | (1 << 17);                          // x^23 + x^18 + 1
    case 24: return (1 << 23) | (1 << 22) | (1 << 21) | (1 << 16);  // x^24 + x^23 + x^22 + x^17 + 1
    case 25: return (1 << 24) | (1 << 21);                          // x^25 + x^22 + 1
    case 26: return (1 << 25) | (1 << 5) | (1 << 1) | (1 << 0);     // x^26 + x^6 + x^2 + x 1
    case 27: return (1 << 26) | (1 << 4) | (1 << 1) | (1 << 0);     // x^27 + x^5 + x^2 + x + 1
    case 28: return (1 << 27) | (1 << 24);                          // x^28 + x^25 + 1
    case 29: return (1 << 28) | (1 << 26);                          // x^29 + x^27 + 1
    case 30: return (1 << 29) | (1 << 5) | (1 << 3) | (1 << 0);     // x^30 + x^6 + x^4 + x + 1
    case 31: return (1 << 30) | (1 << 27);                          // x^31 + x^28 + 1
    case 32: return (1 << 31) | (1 << 21) | (1 << 1) | (1 << 0);    // x^32 + x^22 + x^2 + x + 1
    default:
      Serial.println("Unsupported bit length!");
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
    while (1)
      ;
  }

  // ----- Create filename based on LFSRBits -----
  char filename[20];
  snprintf(filename, sizeof(filename), "%dBitMLS.txt", LFSRBits);
  logFile = SD.open(filename, FILE_WRITE);

  if (!logFile) {
    Serial.println("Failed to open file!");
    return;
  }

  mask = (1UL << LFSRBits) - 1;
  LFSR = mask;
  uint32_t taps = feedbackTaps(LFSRBits);

  Serial.print("Generating MLS with ");
  Serial.print(LFSRBits);
  Serial.println(" bits:");

  //logFile.print("Generating MLS with ");
  //logFile.print(LFSRBits);
  //logFile.println(" bits:");

  Serial.print("The MLS should be ");
  Serial.print((1UL << LFSRBits) - 1);
  Serial.println(" bits long.");

  //logFile.print("The MLS should be ");
  //logFile.print((1UL << LFSRBits) - 1);
  //logFile.println(" bits long.");

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
      logFile.print(feedback ? 1 : 0);

      playMLSBit(feedback);

      LFSR <<= 1;
      if (feedback) {
        LFSR |= 1;
      }
      LFSR &= mask;
    }

    Serial.println("Next");
    //logFile.println("Next");
  }

  uint32_t endMLSTime = micros();

  Serial.println("\nMLS generation complete.");
  Serial.print("It has taken ");
  Serial.print(endMLSTime - startMLSTime);
  Serial.println(" microseconds to calculate and print.");

  //logFile.println("\nMLS generation complete.");
  //logFile.print("It has taken ");
  //logFile.print(endMLSTime - startMLSTime);
  //logFile.println(" microseconds to calculate and print.");

  logFile.close();  // <--- Important to close after writing
}
