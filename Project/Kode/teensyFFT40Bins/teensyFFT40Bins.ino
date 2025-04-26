#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>

AudioInputI2S            i2sMic;
AudioAnalyzeFFT1024      fft1024;
AudioConnection          patchCord1(i2sMic, 0, fft1024, 0);

const float binWidth = 44100.0 / 1024.0;  // ≈ 43.07 Hz bandwidth of bins

void setup() {
  Serial.begin(115200);
  AudioMemory(12);
  fft1024.windowFunction(AudioWindowHanning1024);
}

void loop() {
  if (fft1024.available()) {
    for (int i = 0; i < 40; i++) {
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024.read(i);

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(val, 6);
      Serial.print("  ");
    }
    Serial.println();
  }
}
