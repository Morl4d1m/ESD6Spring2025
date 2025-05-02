#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>

// Audio hardware setup
AudioOutputI2S         i2s1;
AudioControlSGTL5000   sgtl5000_1;

// Mixer: 4 channels (we'll use only channel 3 for the sweep here)
AudioMixer4            mixer;
AudioConnection        patchCordL(mixer, 0, i2s1, 0);
AudioConnection        patchCordR(mixer, 0, i2s1, 1);

// Sine sweep generator and routing
AudioSynthToneSweep    sineSweep;
AudioConnection        patchSweep(sineSweep, 0, mixer, 3);

// Recorder
AudioRecordQueue       recordQueue;
AudioConnection        patchRec(mixer, 0, recordQueue, 0);

// SD card
const int             chipSelect = BUILTIN_SDCARD;
File                   logFile;

// Audio parameters
const float           sineSweepTime    = 10.0f;            // seconds
const int             SAMPLE_RATE      = 44100;
const int             BLOCK_SIZE       = 128;
const int             BLOCKS_PER_SEC   = SAMPLE_RATE / BLOCK_SIZE;
const uint32_t        TOTAL_SAMPLES    = SAMPLE_RATE * (uint32_t)(sineSweepTime + 0.5f);

void setup() {
  Serial.begin(115200);
  AudioMemory(12);
  // SD init
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card init failed!");
    while (1);
  }
  Serial.println("SD card ready");
  // Audio codec
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.3);
  // Mixer gains: only channel 3 (sweep) on at 0.5
  for (int ch = 0; ch < 4; ch++) mixer.gain(ch, (ch == 3) ? 0.5 : 0.0);
  // Start recorder
  recordQueue.begin();
  delay(500);
}

void loop() {
  recordSineSweep();
  Serial.println("Done. Halting.");
  while (1) ;  // stop
}

void recordSineSweep() {
  Serial.println("Recording 1–50 Hz sweep over 10 s");
  logFile = SD.open("sweep.csv", FILE_WRITE);
  if (!logFile) {
    Serial.println("Failed to open file");
    return;
  }
  // Start sweep: from 1 Hz to 50 Hz in sineSweepTime seconds
  sineSweep.play(1, 20, 1220, sineSweepTime);

  uint32_t samplesRecorded = 0;
  while (sineSweep.isPlaying() || samplesRecorded < TOTAL_SAMPLES) {
    if (recordQueue.available() > 0) {
      int16_t* buf = recordQueue.readBuffer();
      int toWrite = min(BLOCK_SIZE, (int)(TOTAL_SAMPLES - samplesRecorded));
      for (int i = 0; i < toWrite; i++) {
        // Normalize to ±1.0 float
        float samp = buf[i] / 32768.0f;
        logFile.print(samp, 6);
        logFile.print(i < toWrite - 1 ? "," : "\n");
      }
      recordQueue.freeBuffer();
      samplesRecorded += toWrite;
    }
  }
    logFile.close();
  Serial.println("Sweep recording complete.");
}
