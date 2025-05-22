#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include "kiss_fft.h"

#define CH12Pin 30
#define CH34Pin 31
#define CH56Pin 32
#define CH12Pin2 35  // Two pins are used to absolutely ensure relays are switched
#define CH34Pin2 34
#define CH56Pin2 33

// PSRAM allocator for KissFFT
kiss_fft_cfg kiss_fft_alloc_psram(int nfft, int inverse_fft) {
  size_t len_needed = 0;
  // Query memory requirement
  kiss_fft_alloc(nfft, inverse_fft, nullptr, &len_needed);
  Serial.printf("Allocating %zu bytes for KissFFT config\n", len_needed);

  void* buffer = extmem_malloc(len_needed);
  if (!buffer) {
    Serial.println("[ERROR] PSRAM allocation for KissFFT failed!");
    return nullptr;
  }
  kiss_fft_cfg cfg = kiss_fft_alloc(nfft, inverse_fft, buffer, &len_needed);
  if (!cfg) {
    Serial.println("[ERROR] kiss_fft_alloc returned null even after buffer allocation");
    extmem_free(buffer);
  }
  return cfg;
}

// Wrapper to force PSRAM usage
__attribute__((section(".psram"))) void kiss_fft_psram(kiss_fft_cfg cfg, const kiss_fft_cpx* in, kiss_fft_cpx* out) {
  kiss_fft(cfg, in, out);
}

size_t next_power_of_2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}


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
const uint8_t ledPin = 13;        // Pin 13 is the builtin LED
uint32_t LFSRBits = 15;           // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 0.743;                      // Practically decides how fast the frequency is changed in the sine sweep, by saying deltaF/time
const uint16_t sampleRate = 44100;                // Default sample rate
const uint8_t blockSize = 128;                    // Default block size for the audio.h library
const int blocksNeeded = sampleRate / blockSize;  // 1 second of samples
uint16_t totalSamples = 441000;                   // Desired length of signal sequence
uint8_t iteration = 0;                            // Counter for alternating between microphone pairs
const float epsilon = 1e-10f;

void setup() {
  Serial.begin(115200);
  AudioMemory(1500);  // Way too much audiomemory set aside, but currently functional

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
      digitalWrite(CH12Pin, HIGH);  // Switch the relay for channel 1 and 2
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
      digitalWrite(CH12Pin, LOW);  // Switch the relay for channel 1 and 2
      digitalWrite(CH12Pin2, LOW);
      Serial.println("Recordings on channel 1/2 done.");
      iteration++;
      delay(1000);
    case 1:
      digitalWrite(CH34Pin, HIGH);  // Switch the relay for channel 3 and 4
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
      digitalWrite(CH34Pin, LOW);  // Switch the relay for channel 3 and 4
      digitalWrite(CH34Pin2, LOW);
      Serial.println("Recordings on channel 3/4 done.");
      iteration++;
      delay(1000);
    case 2:
      digitalWrite(CH56Pin, HIGH);  // Switch the relay for channel 5 and 6
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
      //recordMLS(); // Not functional :(
      delay(1000);
      digitalWrite(CH56Pin, LOW);  // Switch the relay for channel 5 and 6
      digitalWrite(CH56Pin2, LOW);
      Serial.println("Recordings on channel 5/6 done.");
      iteration++;
      delay(1000);
    case 3:
      Serial.println("All recordings done. Beginning FFT and IR computations.");
      File root = SD.open("/");
      while (true) {  // Flips through all files on the SD card
        File entry = root.openNextFile();
        if (!entry) break;
        String name = entry.name();
        entry.close();
        // Process only CSVs matching the sample name
        if (name.endsWith("SAMPLENAMECH12.csv") || name.endsWith("SAMPLENAMECH34.csv") || name.endsWith("SAMPLENAMECH56.csv")) {  // Remember to set SAMPLENAME to match sample across all functions
          computeImpulseResponse(name.c_str());
        }
      }
      Serial.println("All files processed.");
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
      snprintf(filenameCombined, sizeof(filenameCombined), "sine%dSAMPLENAMECH12.csv", octaveFrequency(f));
    } else if (iteration == 1) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sine%dSAMPLENAMECH34.csv", octaveFrequency(f));
    } else if (iteration == 2) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sine%dSAMPLENAMECH56.csv", octaveFrequency(f));
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
    recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
    combinedFile.close();                                   // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Sine done");
}

void recordPhaseShift() {
  Serial.println("Recording phase shift at 1kHz");
  sineWave.amplitude(1);                            // Sets the gain/amplitude of the sinusoid
  sineWave.frequency(1000);                         // Sets the frequency
  for (int phase = 0; phase <= 360; phase += 90) {  // Increments phase shift by 90 degrees
    sineWave.phase(0);                              // Initializes phase at 0 degrees
    char filenameCombined[45];
    if (iteration == 0) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dSAMPLENAMECH12.csv", phase);
    } else if (iteration == 1) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dSAMPLENAMECH34.csv", phase);
    } else if (iteration == 2) {
      snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dSAMPLENAMECH56.csv", phase);
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
    recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
    combinedFile.close();                                   // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Phaseshift done");
}

void recordWhiteNoise() {
  Serial.println("Recording white noise");
  whiteNoise.amplitude(1);  // Sets the amplitude for white noise signal
  if (iteration == 0) {
    removeIfExists("whiteNoiseSAMPLENAMECH12.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("whiteNoiseSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    removeIfExists("whiteNoiseSAMPLENAMECH34.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("whiteNoiseSAMPLENAMECH34.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    removeIfExists("whiteNoiseSAMPLENAMECH56.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("whiteNoiseSAMPLENAMECH56.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    Serial.println("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
  combinedFile.close();                                   // Closes the file on SD
  whiteNoise.amplitude(0);                                // Turns off the white noise
  Serial.println("White noise done.");
}

void recordSineSweep() {
  Serial.println("Recording sine sweep");
  uint32_t sweepSamples = sineSweepTime * 44100;
  if (iteration == 0) {
    removeIfExists("sweepSAMPLENAMECH12.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("sweepSAMPLENAMECH12.csv", FILE_WRITE);  // Create file on SD card
  } else if (iteration == 1) {
    removeIfExists("sweepSAMPLENAMECH34.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("sweepSAMPLENAMECH34.csv", FILE_WRITE);  // Create file on SD card
  } else if (iteration == 2) {
    removeIfExists("sweepSAMPLENAMECH56.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("sweepSAMPLENAMECH56.csv", FILE_WRITE);  // Create file on SD card
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

void recordThreeToFileSingleFile(File& file, uint32_t totalSamples) {  // Records three audioqueues to a .csv file
  uint32_t samplesRecorded = 0;                                        // Checker for when to stop
  while (samplesRecorded < totalSamples) {
    if (recordQueue.available() && mic1Queue.available() && mic2Queue.available()) {  // Checks for data on all signal lines, ensuring that data is present on all lines simultaneously
      int16_t* bufMixer = recordQueue.readBuffer();
      int16_t* bufMic1 = mic1Queue.readBuffer();
      int16_t* bufMic2 = mic2Queue.readBuffer();
      for (int i = 0; i < blockSize; i++) {  // Divides signals into blocks to ensure no overflow is happening
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
      recordQueue.freeBuffer();      // Frees buffer
      mic1Queue.freeBuffer();        // Frees buffer
      mic2Queue.freeBuffer();        // Frees buffer
      samplesRecorded += blockSize;  // Keeps track of how many samples has been saved
    }
  }
}

void removeIfExists(const char* filename) { // Function to remove files, ensuring that new measurements are not just appended to previous
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.print("Deleted previous version of: ");
    Serial.println(filename);
  }
}

uint16_t octaveFrequency(uint16_t octaveBand) { // All 1/3 octave frequencies the impedance tube can measure
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
    case 21: return 1250; // Might have modal irregularities
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
    removeIfExists("15BitMLSSAMPLENAMECH12.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSSAMPLENAMECH12.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 1) {
    removeIfExists("15BitMLSSAMPLENAMECH34.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSSAMPLENAMECH34.csv", FILE_WRITE);  // Creates file on the SD
  } else if (iteration == 2) {
    removeIfExists("15BitMLSSAMPLENAMECH56.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open("15BitMLSSAMPLENAMECH56.csv", FILE_WRITE);  // Creates file on the SD
  } else {
    Serial.println("Unknown iteration number - Shutting down!");
    while (true)
      ;
  }
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  generateMLS();                                          // Sets the amplitude for white noise signal
  recordThreeToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
  combinedFile.close();                                   // Closes the file on SD
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
  uint32_t startMLSTime = micros(); // Timer for MLS generation
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

// Compute and save impulse responses for two mics from a given CSV
void computeImpulseResponse(const char* inputFilename) {
  Serial.printf("\n--- Processing %s ---\n", inputFilename);
  File infile = SD.open(inputFilename);
  if (!infile) {
    Serial.println("Failed to open input file!");
    return;
  }

  const size_t max_samples = 32768;
  float* x = (float*)extmem_malloc(max_samples * sizeof(float));   // mixer
  float* y1 = (float*)extmem_malloc(max_samples * sizeof(float));  // mic1
  float* y2 = (float*)extmem_malloc(max_samples * sizeof(float));  // mic2
  if (!x || !y1 || !y2) {
    Serial.println("Memory allocation failed for input arrays");
    return;
  }

  // Read CSV: mixer, mic1, mic2
  size_t N = 0;
  while (infile.available() && N < max_samples) {
    String line = infile.readStringUntil('\n');
    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    if (idx1 > 0 && idx2 > idx1) {
      x[N] = line.substring(0, idx1).toFloat();
      y1[N] = line.substring(idx1 + 1, idx2).toFloat();
      y2[N] = line.substring(idx2 + 1).toFloat();
      N++;
    }
  }
  infile.close();
  Serial.printf("Loaded %zu samples from %s\n", N, inputFilename);

  // FFT length = next power of two of 2*N
  size_t Nfft = next_power_of_2(2 * N);
  Serial.printf("FFT length: %zu\n", Nfft);

  // Allocate FFT buffers
  kiss_fft_cpx* X = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* Y1 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* Y2 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H1 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H2 = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H1c = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H2c = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* h1t = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* h2t = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  if (!X || !Y1 || !Y2 || !H1 || !H2 || !H1c || !H2c || !h1t || !h2t) {
    Serial.println("Memory allocation failed for FFT buffers");
    return;
  }

  // Zero-pad inputs
  for (size_t i = 0; i < Nfft; i++) {
    X[i].r = (i < N ? x[i] : 0.0f);
    X[i].i = 0.0f;
    Y1[i].r = (i < N ? y1[i] : 0.0f);
    Y1[i].i = 0.0f;
    Y2[i].r = (i < N ? y2[i] : 0.0f);
    Y2[i].i = 0.0f;
  }

  // Create FFT configs
  kiss_fft_cfg fwd = kiss_fft_alloc_psram(Nfft, 0);
  kiss_fft_cfg inv = kiss_fft_alloc_psram(Nfft, 1);
  if (!fwd || !inv) {
    Serial.println("KissFFT config alloc failed");
    return;
  }

  // Forward FFTs
  kiss_fft_psram(fwd, X, X);
  kiss_fft_psram(fwd, Y1, Y1);
  kiss_fft_psram(fwd, Y2, Y2);
  Serial.println("FFTs Done");

  // Frequency response: H1 = Y1/X, H2 = Y2/X
  for (size_t i = 0; i < Nfft; i++) {
    float xr = X[i].r, xi = X[i].i;
    float denom = xr * xr + xi * xi + epsilon;
    // mic1
    {
      float yr = Y1[i].r, yi = Y1[i].i;
      H1[i].r = (yr * xr + yi * xi) / denom;
      H1[i].i = (yi * xr - yr * xi) / denom;
    }
    // mic2
    {
      float yr = Y2[i].r, yi = Y2[i].i;
      H2[i].r = (yr * xr + yi * xi) / denom;
      H2[i].i = (yi * xr - yr * xi) / denom;
    }
  }

  // Copy for output
  memcpy(H1c, H1, Nfft * sizeof(kiss_fft_cpx));
  memcpy(H2c, H2, Nfft * sizeof(kiss_fft_cpx));

  // Inverse FFT to get impulse responses
  kiss_fft(inv, H1, h1t);
  kiss_fft(inv, H2, h2t);
  // Normalize
  for (size_t i = 0; i < Nfft; i++) {
    h1t[i].r /= (float)Nfft;
    h2t[i].r /= (float)Nfft;
  }
  Serial.println("IFFTs Done");

  // Write CSV
  String outName = String(inputFilename);
  outName.replace(".csv", "IRAndFFT.csv");
  removeIfExists(outName.c_str());
  File out = SD.open(outName.c_str(), FILE_WRITE);
  out.println("FreqHz,Xreal,Ximag,Y1real,Y1imag,H1real,H1imag,H1magdB,H1phaserad,h1Impulse,Y2real,Y2imag,H2real,H2imag,H2magdB,H2phaserad,h2Impulse");

  float freqRes = (float)sampleRate / Nfft;
  size_t Nu = Nfft / 2 + 1;
  for (size_t i = 0; i < Nu; i++) {
    float freq = i * freqRes;
    float xr = X[i].r, xi = X[i].i;
    // mic1
    float y1r = Y1[i].r, y1i = Y1[i].i;
    float h1r = H1c[i].r, h1i = H1c[i].i;
    float mag1 = abs(sqrtf(h1r * h1r + h1i * h1i));
    float dB1 = 20.0f * log10f(mag1 + epsilon);
    float ph1 = atan2f(h1i, h1r);
    float imp1 = h1t[i].r;
    // mic2
    float y2r = Y2[i].r, y2i = Y2[i].i;
    float h2r = H2c[i].r, h2i = H2c[i].i;
    float mag2 = abs(sqrtf(h2r * h2r + h2i * h2i));
    float dB2 = 20.0f * log10f(mag2 + epsilon);
    float ph2 = atan2f(h2i, h2r);
    float imp2 = h2t[i].r;

    out.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
               freq, xr, xi,
               y1r, y1i, h1r, h1i, dB1, ph1, imp1,
               y2r, y2i, h2r, h2i, dB2, ph2, imp2);
  }
  out.close();
  Serial.printf("Output written to %s\n", outName.c_str());

  // Cleanup
  extmem_free(x);
  extmem_free(y1);
  extmem_free(y2);
  extmem_free(X);
  extmem_free(Y1);
  extmem_free(Y2);
  extmem_free(H1);
  extmem_free(H2);
  extmem_free(H1c);
  extmem_free(H2c);
  extmem_free(h1t);
  extmem_free(h2t);
  extmem_free(fwd);
  extmem_free(inv);
  Serial.println("Cleanup Done!");
}
