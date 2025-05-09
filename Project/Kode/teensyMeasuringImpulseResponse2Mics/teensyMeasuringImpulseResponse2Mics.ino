// Audio player developed from the 1-2: Test Hardware example available to Teensy, with specific intention of playing signals used in my ESD6 project
// Current code is meant to function with just the audio shield attached.

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

// General audio preparation
AudioOutputI2S i2s1;
AudioInputI2S i2sMicSet1;
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
AudioConnection patchCord8(i2sMicSet1, 0, mic1Queue, 0);
AudioRecordQueue mic2Queue;  // Recordqueue for the 2nd microphone
AudioConnection patchCord9(i2sMicSet1, 1, mic2Queue, 0);

// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
File mixerFile;
File micFile;
File combinedFile;

// Global variables
uint16_t count = 1;
const uint8_t ledPin = 13;        // Pin 13 is the builtin LED
uint32_t LFSRBits = 21;           // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 10;                            // Practically decides how fast the frequency is changed in the sine sweep, by saying deltaF/time
const uint16_t SAMPLE_RATE = 44100;                  // Default sample rate
const uint8_t BLOCK_SIZE = 128;                      // Default block size for the audio.h library
const int BLOCKS_NEEDED = SAMPLE_RATE / BLOCK_SIZE;  // 1 second of samples
uint16_t TOTAL_SAMPLES = 441000;

void setup() {
  Serial.begin(115200);
  AudioMemory(1000);

  // SD setup
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1)
      ;
  }
  Serial.println("SD card initialized");

  // Audio setup
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);
  mixer.gain(0, 0.3);  // sine
  mixer.gain(1, 0.3);  // noise
  mixer.gain(2, 0.3);  // sweep

  recordQueue.begin();
  mic1Queue.begin();
  mic2Queue.begin();
  delay(1000);  // Ensure system stabilizes
}

void loop() {
  recordSine();
  delay(1000);

  recordPhaseShift();
  delay(1000);

  recordWhiteNoise();
  delay(1000);

  recordSineSweep();
  delay(1000);

  Serial.println("All recordings done. Halting.");
  Serial.println(AudioProcessorUsageMax());
  while (true)
    ;
}

void recordSine() {
  Serial.println("Recording sine wave");
  sineWave.begin(WAVEFORM_SINE);          // Specifies a sine wave. Could also be sawtooth, square, triangle etc
  sineWave.amplitude(0.3);                // Sets the gain/amplitude of the sinusoid
  for (int f = 20; f <= 1220; f += 50) {  // Increments frequency by 50 Hz
    sineWave.frequency(f);                // Sets the current frequency
    char filenameCombined[30];
    snprintf(filenameCombined, sizeof(filenameCombined), "sine%dCombined2Mics.csv", f);
    Serial.println(filenameCombined);
    removeIfExists(filenameCombined);                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      Serial.println("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
    combinedFile.close();                                   // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Sine done");
}

void recordPhaseShift() {
  Serial.println("Recording phase shift at 1kHz");
  sineWave.amplitude(0.3);                          // Sets the gain/amplitude of the sinusoid
  sineWave.frequency(1000);                         // Sets the frequency
  for (int phase = 0; phase <= 360; phase += 90) {  // Increments phase shift by 90 degrees
    sineWave.phase(0);                              // Initializes phase at 0 degrees
    char filenameCombined[40];
    snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dCombined2Mics.csv", phase);
    Serial.println(filenameCombined);
    removeIfExists(filenameCombined);                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      Serial.println("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
    combinedFile.close();                                   // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Phaseshift done");
}

void recordWhiteNoise() {
  Serial.println("Recording white noise");
  whiteNoise.amplitude(0.3);                                     // Sets the amplitude for white noise signal
  removeIfExists("whiteNoiseCombined2Mics.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
  combinedFile = SD.open("whiteNoiseCombined2Mics.csv", FILE_WRITE);  // Creates file on the SD
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  recordThreeToFileSingleFile(combinedFile, SAMPLE_RATE);  // Saves the current audio to SD
  combinedFile.close();                                   // Closes the file on SD
  whiteNoise.amplitude(0);                                // Turns off the white noise
  Serial.println("White noise done.");
}

void recordSineSweep() {
  Serial.println("Recording sine sweep");
  uint32_t sweepSamples = sineSweepTime * SAMPLE_RATE;
  removeIfExists("sweepCombined2Mics.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
  combinedFile = SD.open("sweepCombined2Mics.csv", FILE_WRITE);  // Create file on SD card
  sineSweep.play(1, 20, 1220, sineSweepTime);                      // Initializes sine sweep from 20Hz to 1220Hz over 1 second
  recordThreeToFileSingleFile(combinedFile, sweepSamples);          // Save both inputs in a 2Mics.csv file
  combinedFile.close();                                            // Closes the file on SD
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
    if (recordQueue.available() && mic1Queue.available()) {  // Checks for data on all signal lines, ensuring that data is present on all lines simultaneously
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
