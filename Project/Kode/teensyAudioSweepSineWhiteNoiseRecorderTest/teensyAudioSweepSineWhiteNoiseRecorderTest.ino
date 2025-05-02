// Audio player developed from the 1-2: Test Hardware example available to Teensy, with specific intention of playing signals used in my ESD6 project
// Current code is meant to function with just the audio shield attached.

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

// General audio preparation
AudioOutputI2S i2s1;
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
AudioRecordQueue recordQueue;  // To capture audio data
AudioConnection patchCord7(mixer, 0, recordQueue, 0);

// Global variables
uint16_t count = 1;
const uint8_t ledPin = 13;        // Pin 13 is the builtin LED
uint32_t LFSRBits = 21;           // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 10;  // Practically decides how fast the frequency is changed in the sine sweep, by saying deltaF/time

// === SD Card ===
const int chipSelect = BUILTIN_SDCARD;
File logFile;

// === Config ===
const int SAMPLE_RATE = 44100;                       // Default sample rate
const int BLOCK_SIZE = 128;                          // Default block size for the audio.h library
const int BLOCKS_NEEDED = SAMPLE_RATE / BLOCK_SIZE;  // 1 second of samples
int TOTAL_SAMPLES = 441000;
void setup() {
  Serial.begin(115200);
  AudioMemory(100);

  // SD setup
  /*
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1)
      ;
  }*/
  Serial.println("SD card initialized");

  // Audio setup
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);
  mixer.gain(0, 0.3);  // sine
  mixer.gain(1, 0.3);  // noise
  mixer.gain(2, 0.3);  // sweep

  recordQueue.begin();
  delay(1000);  // Ensure system stabilizes
}

void loop() {
  //recordSine();
  delay(1000);

  //recordPhaseShift();
  delay(1000);

  recordWhiteNoise();
  delay(1000);

  //recordSineSweep();
  delay(1000);

  Serial.println("All recordings done. Halting.");
  while (true)
    ;
}

void recordSine() {
  Serial.println("Recording sine wave");
  sineWave.begin(WAVEFORM_SINE);          // Specifies a sine wave. Could also be sawtooth, square, triangle etc
  sineWave.amplitude(0.3);                // Sets the gain/amplitude of the sinusoid
  for (int f = 20; f <= 1220; f += 50) {  // Increments frequency by 50 Hz
    sineWave.frequency(f);                // Sets the current frequency
    char filename[20];
    snprintf(filename, sizeof(filename), "sine%d.csv", f);
    logFile = SD.open(filename, FILE_WRITE);  // Creates file on the SD card
    recordAudioToFile();                      // Saves the current audio to SD
    logFile.close();                          // Closes the file on SD
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
    char filename[20];
    snprintf(filename, sizeof(filename), "sineShifted%d.csv", phase);
    logFile = SD.open(filename, FILE_WRITE);  // Creates file on the SD card
    recordAudioToFile();                      // Saves the current audio to SD
    logFile.close();                          // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Phaseshift done");
}

void recordWhiteNoise() {
  Serial.println("Recording white noise");
  whiteNoise.amplitude(0.3);                   // Sets the amplitude for white noise signal
  logFile = SD.open("white.csv", FILE_WRITE);  // Creates a file on the SD card
  recordAudioToFile();                         // Saves the current audio to SD
  logFile.close();                             // Closes the file on SD
  whiteNoise.amplitude(0);                     // Turns off the white noise
  Serial.println("White noise done.");
}

void recordSineSweep() {
  Serial.println("Recording sine sweep");
  //TOTAL_SAMPLES = TOTAL_SAMPLES * 10;
  Serial.println("50");
  logFile = SD.open("sweep.csv", FILE_WRITE);  // Creates a file on the SD card
  sineSweep.play(1, 1, 50, sineSweepTime);     // Initializes sine sweep from 20Hz to 1220Hz over 1 second
  if (sineSweep.isPlaying()==1) {
    recordAudioToFile();  // Saves the current audio to SD
    return;
  }
  logFile.close();  // Closes the file on SD
  Serial.println("Sine sweep done.");
}

void recordAudioToFile() {
  int samplesRecorded = 0;
  while (samplesRecorded < TOTAL_SAMPLES) {
    if (recordQueue.available() > 0) {
      int16_t* buffer = recordQueue.readBuffer();
      int samplesToWrite = min(BLOCK_SIZE, TOTAL_SAMPLES - samplesRecorded);
      for (int i = 0; i < samplesToWrite; i++) {
        float sample = (float)buffer[i] / 32768.0f;
        logFile.print(sample, 6);
        logFile.print(i < samplesToWrite - 1 ? "," : "\n");
      }
      recordQueue.freeBuffer();
      samplesRecorded += samplesToWrite;
    }
  }
}