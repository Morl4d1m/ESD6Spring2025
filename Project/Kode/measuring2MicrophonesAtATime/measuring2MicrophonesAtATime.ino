#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>

AudioInputI2S i2sMic1;
AudioInputI2S2 i2sMic2;
AudioAnalyzeFFT1024 fft10241;
AudioConnection patchCord1(i2sMic1, 0, fft10241, 0);  // 1 taller afgør om den modtager på L eller R. Sæt til 0 for L og 1 for R, og husk at sætte microphonen til hhv GND og VDD

AudioAnalyzeFFT1024 fft10242;
AudioConnection patchCord2(i2sMic2, 0, fft10242, 0);  // 1 taller afgør om den modtager på L eller R. Sæt til 0 for L og 1 for R, og husk at sætte microphonen til hhv GND og VDD

const float binWidth = 20;  //44100.0 / 1024.0;  // ≈ 43.07 Hz bandwidth of bins
int loopNumber = 1;

void setup() {
  Serial.begin(115200);
  AudioMemory(120);
  fft10241.windowFunction(AudioWindowRectangular1024);
  fft10242.windowFunction(AudioWindowRectangular1024);
  Serial.println("Setup done");
}

void loop() {
  //Serial.print("This is loop number");
  //Serial.println(loopNumber);
  if (fft10241.available()) {
    Serial.println("Channel 1:");
    for (int i = 20; i < 50; i++) {  // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft10241.read(i);

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(val, 6);
      Serial.print("  ");
    }
    Serial.println();
  }
  if (fft10242.available()) {
    Serial.println("Channel 2:");
    for (int i = 20; i < 50; i++) {  // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft10242.read(i);

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(val, 6);
      Serial.print("  ");
    }
    Serial.println();
  }
  //delay(10);
  //loopNumber++;
}
