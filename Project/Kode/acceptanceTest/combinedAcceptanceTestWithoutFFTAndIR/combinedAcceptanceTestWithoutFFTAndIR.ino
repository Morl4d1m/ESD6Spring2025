// Audio player developed from the 1-2: Test Hardware example available to Teensy, with specific intention of playing signals used in my ESD6 project
// Current code is meant to function with just the audio shield attached.

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

#define CH12Pin 30
#define CH34Pin 31
#define CH56Pin 32
#define CH12Pin2 35
#define CH34Pin2 34
#define CH56Pin2 33

// General audio preparation
AudioOutputI2S i2s1;
AudioInputI2S i2sMic1;
AudioControlSGTL5000 sgtl5000_1;

// Mixer to enable all signals being "active" at once.
AudioMixer4 mixer;                              // Has 4 channels to mix on
AudioConnection patchCord5(mixer, 0, i2s1, 0);  // Mixer to left output
AudioConnection patchCord6(mixer, 0, i2s1, 1);  // Mixer to right output

// When MLS is generated
AudioPlayQueue MLSSignal;
AudioConnection patchCord1(MLSSignal, 0, mixer, 0);  // Sends the MLS to channel 0 in the mixer

// When pure sine is generated
AudioSynthWaveform sineWave;                        // Utilizes pre-made sine wave generator
AudioConnection patchCord2(sineWave, 0, mixer, 1);  // Sends the pure sine to channel 1 in the mixer

// When white noise is generated
AudioSynthNoiseWhite whiteNoise;                      // Utilizes pre-made white noise generator
AudioConnection patchCord3(whiteNoise, 0, mixer, 2);  // Sends the white noise to channel 2 in the mixer

// When sine sweep is generated
AudioSynthToneSweep sineSweep;                       // Utilizes pre-made sine sweep generator
AudioConnection patchCord4(sineSweep, 0, mixer, 3);  // Sends the sine sweep to channel 3 in the mixer

// Signal recorder for testing:
AudioRecordQueue recordQueue;  // To capture audio data directly from mixer
AudioConnection patchCord7(mixer, 0, recordQueue, 0);

// Microphone recording for impulse responses:
AudioRecordQueue mic1Queue;  // Recordqueue for the 1st microphone
AudioConnection patchCord8(i2sMic1, 0, mic1Queue, 0);
AudioRecordQueue mic2Queue;  // Recordqueue for the 2nd microphone
AudioConnection patchCord9(i2sMic1, 1, mic2Queue, 0);

// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
File mixerFile;
File micFile;
File combinedFile;

// Global variables
uint16_t count = 1;
const uint8_t ledPin = 13;        // Pin 13 is the builtin LED
uint32_t LFSRBits = 15;           // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 0.743;                 // Practically decides how fast the frequency is changed in the sine sweep, by saying deltaF/time
const uint16_t SAMPLE_RATE = 44100;                  // Default sample rate
const uint8_t BLOCK_SIZE = 128;                      // Default block size for the audio.h library
const int BLOCKS_NEEDED = SAMPLE_RATE / BLOCK_SIZE;  // 1 second of samples
uint16_t TOTAL_SAMPLES = 441000;
uint8_t iteration = 0;

void setup() {
  Serial.begin(115200);
  AudioMemory(1500);

  // SD setup
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1)
      ;
  }
  Serial.println("SD card initialized");

  // Pin setup
  pinMode(CH12Pin, OUTPUT);
  pinMode(CH34Pin, OUTPUT);
  pinMode(CH56Pin, OUTPUT);
  pinMode(CH12Pin2, OUTPUT);
  pinMode(CH34Pin2, OUTPUT);
  pinMode(CH56Pin2, OUTPUT);
  digitalWrite(CH12Pin, LOW);
  digitalWrite(CH34Pin, LOW);
  digitalWrite(CH56Pin, LOW);
  digitalWrite(CH12Pin2, LOW);
  digitalWrite(CH34Pin2, LOW);
  digitalWrite(CH56Pin2, LOW);

  // Audio setup
  sgtl5000_1.enable();
  sgtl5000_1.volume(1);
  mixer.gain(0, 0.0653);  // Gain for MLS
  mixer.gain(1, 0.0793);  // Gain for pure sine
  mixer.gain(2, 0.105);   // Gain for white noise
  mixer.gain(3, 0.078);   // Gain for sine sweep
  recordQueue.begin();
  mic1Queue.begin();
  mic2Queue.begin();

  delay(1000);  // Ensure system stabilizes
}

void loop() {
  switch (iteration) {
    case 0:
      digitalWrite(CH12Pin, HIGH);
      digitalWrite(CH12Pin2, HIGH);
      delay(50);
      recordSine();
      delay(1000);
      recordPhaseShift();
      delay(1000);
      recordWhiteNoise();
      delay(1000);
      recordSineSweep();
      delay(1000);
      //recordMLS();
      delay(1000);
      digitalWrite(CH12Pin, LOW);
      digitalWrite(CH12Pin2, LOW);
      Serial.println("Recordings on channel 1/2 done.");
      iteration++;
      delay(1000);
    case 1:
      digitalWrite(CH34Pin, HIGH);
      digitalWrite(CH34Pin2, HIGH);
      delay(50);
      recordSine();
      delay(1000);
      recordPhaseShift();
      delay(1000);
      recordWhiteNoise();
      delay(1000);
      recordSineSweep();
      delay(1000);
      //recordMLS();
      delay(1000);
      digitalWrite(CH34Pin, LOW);
      digitalWrite(CH34Pin2, LOW);
      Serial.println("Recordings on channel 3/4 done.");
      iteration++;
      delay(1000);
    case 2:
      digitalWrite(CH56Pin, HIGH);
      digitalWrite(CH56Pin2, HIGH);
      delay(50);
      recordSine();
      delay(1000);
      recordPhaseShift();
      delay(1000);
      recordWhiteNoise();
      delay(1000);
      recordSineSweep();
      delay(1000);
      //recordMLS();
      delay(1000);
      digitalWrite(CH56Pin, LOW);
      digitalWrite(CH56Pin2, LOW);
      Serial.println("Recordings on channel 5/6 done.");
      iteration++;
      delay(1000);
    case 3:
      Serial.println("All recordings done. Halting.");
      Serial.println(AudioProcessorUsageMax());
      while (true)
        ;
  }
}

void recordSine() {
  Serial.println("Recording sine wave");
  sineWave.begin(WAVEFORM_SINE);  // Specifies a sine wave. Could also be sawtooth, square, triangle etc
  sineWave.amplitude(1);          // Sets the gain/amplitude of the sinusoid

  for (int f = 1; f <= 21; f++) {            // Increments frequency by 50 Hz
    sineWave.frequency(octaveFrequency(f));  // Sets the current frequency
    char filenameCombined[45];
    if (iteration == 0) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sine%dGreyFeltCH12.csv", octaveFrequency(f));
    } else if (iteration == 1) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sine%dGreyFeltCH34.csv", octaveFrequency(f));
    } else if (iteration == 2) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sine%dGreyFeltCH56.csv", octaveFrequency(f));
    } else {
      Serial.println("Unknown iteration number - Shutting down!");
      while (true)
        ;
    }
    Serial.println(filenameCombined);
    removeIfExists(filenameCombined);                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      Serial.println("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
    combinedFile.close();                                    // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Sine done");
}

void recordPhaseShift() {
  Serial.println("Recording phase shift at 1kHz");
  sineWave.amplitude(1);                          // Sets the gain/amplitude of the sinusoid
  sineWave.frequency(1000);                         // Sets the frequency
  for (int phase = 0; phase <= 360; phase += 90) {  // Increments phase shift by 90 degrees
    sineWave.phase(0);                              // Initializes phase at 0 degrees
    char filenameCombined[45];
    if (iteration == 0) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dGreyFeltCH12.csv", phase);
    } else if (iteration == 1) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dGreyFeltCH34.csv", phase);
    } else if (iteration == 2) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dGreyFeltCH56.csv", phase);
    } else {
      Serial.println("Unknown iteration number - Shutting down!");
      while (true)
        ;
    }
    Serial.println(filenameCombined);
    removeIfExists(filenameCombined);                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      Serial.println("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
    combinedFile.close();                                    // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Phaseshift done");
}

void recordWhiteNoise() {
  Serial.println("Recording white noise");
  whiteNoise.amplitude(1);  // Sets the amplitude for white noise signal
  if (iteration == 0) {
    removeIfExists("whiteNoiseGreyFeltCH12.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("whiteNoiseGreyFeltCH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    removeIfExists("whiteNoiseGreyFeltCH34.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("whiteNoiseGreyFeltCH34.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    removeIfExists("whiteNoiseGreyFeltCH56.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("whiteNoiseGreyFeltCH56.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    Serial.println("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
  combinedFile.close();                                    // Closes the file on SD
  whiteNoise.amplitude(0);                                 // Turns off the white noise
  Serial.println("White noise done.");
}

void recordSineSweep() {
  Serial.println("Recording sine sweep");
  uint32_t sweepSamples = sineSweepTime * 44100;
  if (iteration == 0) {
  removeIfExists("sweepGreyFeltCH12.csv");  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("sweepGreyFeltCH12.csv", FILE_WRITE);  // Create file on SD card
  } else if (iteration == 1) {
  removeIfExists("sweepGreyFeltCH34.csv");  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("sweepGreyFeltCH34.csv", FILE_WRITE);  // Create file on SD card
  } else if (iteration == 2) {
  removeIfExists("sweepGreyFeltCH56.csv");  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("sweepGreyFeltCH56.csv", FILE_WRITE);  // Create file on SD card
  } else {
    Serial.println("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  sineSweep.play(1, 20, 1220, sineSweepTime);               // Initializes sine sweep from 20Hz to 1220Hz over 1 second
  recordThreeToFileSingleFile(combinedFile, sweepSamples);  // Save both inputs in a 6Mics.csv file
  combinedFile.close();                                     // Closes the file on SD
  Serial.println("Sine sweep done.");
}

void recordBothToFile(File& mixerFile, File& micFile, uint32_t totalSamples) {  // Doesn't work as well as expected
  uint32_t samplesRecorded = 0;
  while (samplesRecorded < totalSamples) {
    bool wroteBlock = false;

    // Mixer
    if (recordQueue.available()) {
      int16_t* buf = recordQueue.readBuffer();
      for (int i = 0; i < BLOCK_SIZE; i++) {
        float s = (float)buf[i] / 32768.0f;
        mixerFile.print(s, 6);
        mixerFile.print(i < BLOCK_SIZE - 1 ? "," : "\n");
      }
      recordQueue.freeBuffer();
      wroteBlock = true;
    }

    // Mic
    if (mic1Queue.available()) {
      int16_t* buf = mic1Queue.readBuffer();
      for (int i = 0; i < BLOCK_SIZE; i++) {
        float s = (float)buf[i] / 32768.0f;
        micFile.print(s, 6);
        micFile.print(i < BLOCK_SIZE - 1 ? "," : "\n");
      }
      mic1Queue.freeBuffer();
      wroteBlock = true;
    }

    if (wroteBlock) {
      samplesRecorded += BLOCK_SIZE;
    }
  }
}

void recordBothToFileSingleFile(File& file, uint32_t totalSamples) {  // Works much better
  uint32_t samplesRecorded = 0;

  while (samplesRecorded < totalSamples) {
    if (recordQueue.available() && mic1Queue.available()) {
      int16_t* bufMixer = recordQueue.readBuffer();
      int16_t* bufMic = mic1Queue.readBuffer();

      for (int i = 0; i < BLOCK_SIZE; i++) {
        float sMixer = (float)bufMixer[i] / 32768.0f;
        float sMic = (float)bufMic[i] / 32768.0f;
        file.print(sMixer, 6);
        file.print(",");
        file.print(sMic, 6);
        file.print("\n");
      }

      recordQueue.freeBuffer();
      mic1Queue.freeBuffer();
      samplesRecorded += BLOCK_SIZE;
    }
  }
}

void recordThreeToFileSingleFile(File& file, uint32_t totalSamples) {  // Works much better
  uint32_t samplesRecorded = 0;                                        // Checker for when to stop

  while (samplesRecorded < totalSamples) {
    if (recordQueue.available() && mic1Queue.available() && mic2Queue.available()) {  // Checks for data on all signal lines, ensuring that data is present on all lines simultaneously
      int16_t* bufMixer = recordQueue.readBuffer();
      int16_t* bufMic1 = mic1Queue.readBuffer();
      int16_t* bufMic2 = mic2Queue.readBuffer();

      for (int i = 0; i < BLOCK_SIZE; i++) {  // Divides signals into blocks to ensure no overflow is happening
        float sMixer = (float)bufMixer[i] / 32768.0f;
        float sMic1 = (float)bufMic1[i] / 32768.0f;
        float sMic2 = (float)bufMic2[i] / 32768.0f;
        file.print(sMixer, 6);  // Saves mixer signal to SD card
        file.print(",");        // Saves to SD card
        file.print(sMic1, 6);   // Saves mic 1 signal to SD card
        file.print(",");        // Saves to SD card
        file.print(sMic2, 6);   // Saves mic2 signal to SD card
        file.print("\n");
      }

      recordQueue.freeBuffer();       // Frees buffer
      mic1Queue.freeBuffer();         // Frees buffer
      mic2Queue.freeBuffer();         // Frees buffer
      samplesRecorded += BLOCK_SIZE;  // Keeps track of how many samples has been saved
    }
  }
}

void removeIfExists(const char* filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.print("Deleted previous version of: ");
    Serial.println(filename);
  }
}

uint16_t octaveFrequency(uint16_t octaveBand) {
  switch (octaveBand) {
    case 1: return 12.5;
    case 2: return 16;
    case 3: return 20;
    case 4: return 25;
    case 5: return 31.5;
    case 6: return 40;
    case 7: return 50;
    case 8: return 63;
    case 9: return 80;
    case 10: return 100;
    case 11: return 125;
    case 12: return 160;
    case 13: return 200;
    case 14: return 250;
    case 15: return 315;
    case 16: return 400;
    case 17: return 500;
    case 18: return 630;
    case 19: return 800;
    case 20: return 1000;
    case 21: return 1250;
    default:
      Serial.println("Unsupported bit length!");
      return 0;
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

void recordMLS() {
  Serial.println("Recording MLS");
  char filename[45];
  if (iteration == 0) {
    removeIfExists("15BitMLSGreyFeltCH12.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSGreyFeltCH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    removeIfExists("15BitMLSGreyFeltCH34.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSGreyFeltCH34.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    removeIfExists("15BitMLSGreyFeltCH56.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSGreyFeltCH56.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    Serial.println("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  generateMLS();  // Sets the amplitude for white noise signal
  recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
  combinedFile.close();                                    // Closes the file on SD
  Serial.println("MLS done.");
}

void playMLSBit(bool bit, int samplesPerBit = 1, int amplitude = 28000) {  // Bool is the actual input parameter of the function (0 or 1), while samplesPerBit and amplitude are static. Amplitude is set at 28000 to not risk clipping the signal on a speaker
  static int16_t buffer[128];
  static int bufferIndex = 0;

  int16_t value = bit ? amplitude : -amplitude;  // Ternary operator deciding to set the amplitude at + or - the given amplitude based on boolean value

  for (int i = 0; i < samplesPerBit; i++) {
    buffer[bufferIndex++] = value;

    if (bufferIndex == 128) {                                 // Buffer set to 128 bits, conforming to I2S standards
      memcpy(MLSSignal.getBuffer(), buffer, sizeof(buffer));  // Copies buffer data to the AudioPlayQueue, enabling the Teensy to play the MLS
      MLSSignal.playBuffer();                                 // Proprietary play function for the AudioPlayQueue
      bufferIndex = 0;                                        // Resets the bufferindex, so a new sequence can be initialized
    }
  }
}

void generateMLS() {
  if (LFSRBits < 2 || LFSRBits > 32) {
    Serial.println("LFSRBits must be between 2 and 32.");
    while (1)
      ;
  }
  mask = (1UL << LFSRBits) - 1;
  LFSR = mask;
  uint32_t taps = feedbackTaps(LFSRBits);
  Serial.print("Generating MLS with ");
  Serial.print(LFSRBits);
  Serial.println(" bits:");
  Serial.print("The MLS should be ");
  Serial.print((1UL << LFSRBits) - 1);
  Serial.println(" bits long.");
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
      //logFile.print(feedback ? 1 : 0);
      playMLSBit(feedback);
      LFSR <<= 1;
      if (feedback) {
        LFSR |= 1;
      }
      LFSR &= mask;
    }
  }

  uint32_t endMLSTime = micros();  // Timer end to monitor execution time
  Serial.println("\nMLS generation complete.");
  Serial.print("It has taken ");
  Serial.print(endMLSTime - startMLSTime);  // Total execution time given in serial monitor
  Serial.println(" microseconds to calculate and print.");
}
