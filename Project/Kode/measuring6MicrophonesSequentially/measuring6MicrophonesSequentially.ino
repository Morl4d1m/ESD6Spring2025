#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>

AudioInputI2SHex i2sHex1;
AudioAnalyzeFFT1024 fft1024CH1;
AudioAnalyzeFFT1024 fft1024CH2;
AudioAnalyzeFFT1024 fft1024CH3;
AudioAnalyzeFFT1024 fft1024CH4;
AudioAnalyzeFFT1024 fft1024CH5;
AudioAnalyzeFFT1024 fft1024CH6;
AudioConnection* patchCord1 = nullptr;
AudioConnection* patchCord2 = nullptr;
AudioConnection* patchCord3 = nullptr;
AudioConnection* patchCord4 = nullptr;
AudioConnection* patchCord5 = nullptr;
AudioConnection* patchCord6 = nullptr;
/*
AudioConnection patchCord1(i2sHex1, 0, fft1024CH1, 0);
AudioConnection patchCord2(i2sHex1, 1, fft1024CH2, 0);
AudioConnection patchCord3(i2sHex1, 2, fft1024CH3, 0);
AudioConnection patchCord4(i2sHex1, 3, fft1024CH4, 0);
AudioConnection patchCord5(i2sHex1, 4, fft1024CH5, 0);
AudioConnection patchCord6(i2sHex1, 5, fft1024CH6, 0);
*/

#define CH12Pin 10
#define CH34Pin 11
#define CH56Pin 12

const float binWidth = 44100.0 / 1024.0;  // ≈ 43.07 Hz bandwidth of bins
const uint8_t numberOfBins = 30;
float fftCH1Bins[30];
float fftCH2Bins[30];
float fftCH3Bins[30];
float fftCH4Bins[30];
float fftCH5Bins[30];
float fftCH6Bins[30];
uint8_t numberOfRunsToAverage = 10;
uint8_t loopNumber = 1;

int startTimePair = 0;
int endTimePair = 0;
int pairTime = 0;
int startTimeTotal = 0;
int endTimeTotal = 0;
int totalTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Setup init");
  pinMode(CH12Pin, OUTPUT);
  pinMode(CH34Pin, OUTPUT);
  pinMode(CH56Pin, OUTPUT);
  AudioMemory(120);
  fft1024CH1.windowFunction(AudioWindowHanning1024);
  fft1024CH2.windowFunction(AudioWindowHanning1024);
  fft1024CH3.windowFunction(AudioWindowHanning1024);
  fft1024CH4.windowFunction(AudioWindowHanning1024);
  fft1024CH5.windowFunction(AudioWindowHanning1024);
  fft1024CH6.windowFunction(AudioWindowHanning1024);
  Serial.println("Setup done");
}

void loop() {
  Serial.print("This is loop number");
  Serial.println(loopNumber);
  startTimeTotal = micros();
  startTimePair = micros();
  AudioNoInterrupts();
  digitalWrite(CH12Pin, HIGH);
  delay(1000);
  if (patchCord1) {
    delete patchCord1;
    patchCord1 = nullptr;
  }
  if (patchCord2) {
    delete patchCord2;
    patchCord2 = nullptr;
  }
  patchCord1 = new AudioConnection(i2sHex1, 0, fft1024CH1, 0);
  patchCord2 = new AudioConnection(i2sHex1, 1, fft1024CH2, 0);
  AudioInterrupts();
  if (fft1024CH1.available()) {
    Serial.print("Channel 1 ");
    //for (int j = 0; j < numberOfRunsToAverage; j++) {
    memset(fftCH1Bins, 0, sizeof(fftCH1Bins));  // Reset the averaged values
    for (int i = 0; i < numberOfBins; i++) {    // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024CH1.read(i);
      fftCH1Bins[i] += val;
        /*
        Serial.print(startFreq, 1);
        Serial.print("-");
        Serial.print(endFreq, 1);
        Serial.print(": ");
        Serial.print(val, 6);
        Serial.print("  ");
        
      }

      //Serial.println("End of run d");
    }
    Serial.println("Averages for each bin:");
    for (int k = 0; k < numberOfBins; k++) {
      float avgVal = fftCH1Bins[k] / numberOfRunsToAverage;  // Calculate the average
      float startFreq = k * binWidth;
      float endFreq = (k + 1) * binWidth;

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(avgVal, 6);
      Serial.print("  ");
    */}
        Serial.println("  ");
  }
  if (fft1024CH2.available()) {
    Serial.print("Channel 2 ");
    //for (int j = 0; j < numberOfRunsToAverage; j++) {
    memset(fftCH2Bins, 0, sizeof(fftCH2Bins));  // Reset the averaged values
    for (int i = 0; i < numberOfBins; i++) {    // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024CH2.read(i);
      fftCH2Bins[i] += val;
        /*
        Serial.print(startFreq, 1);
        Serial.print("-");
        Serial.print(endFreq, 1);
        Serial.print(": ");
        Serial.print(val, 6);
        Serial.print("  ");
        
      }

      //Serial.println("End of run d");
    }
    Serial.println("Averages for each bin:");
    for (int k = 0; k < numberOfBins; k++) {
      float avgVal = fftCH2Bins[k] / numberOfRunsToAverage;  // Calculate the average
      float startFreq = k * binWidth;
      float endFreq = (k + 1) * binWidth;

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(avgVal, 6);
      Serial.print("  ");
    */}
        Serial.println("  ");
  }
  endTimePair = micros();
  pairTime = endTimePair - startTimePair;
  Serial.print("Measurement on mic 1/2 done in ");
  Serial.print(pairTime);
  Serial.println(" microseconds");

  for (int q = 0; q < numberOfBins; q++) {
    float startFreq = q * binWidth;
    float endFreq = (q + 1) * binWidth;

    Serial.print(startFreq, 1);
    Serial.print("-");
    Serial.print(endFreq, 1);
    Serial.print(": ");
    Serial.print(fftCH1Bins[q], 6);
    Serial.print("  ");
  }
  Serial.println();

  for (int q = 0; q < numberOfBins; q++) {
    float startFreq = q * binWidth;
    float endFreq = (q + 1) * binWidth;

    Serial.print(startFreq, 1);
    Serial.print("-");
    Serial.print(endFreq, 1);
    Serial.print(": ");
    Serial.print(fftCH2Bins[q], 6);
    Serial.print("  ");
  }

  startTimePair = micros();
  AudioNoInterrupts();
  digitalWrite(CH12Pin, LOW);
  delay(1000);
  digitalWrite(CH34Pin, HIGH);
  delay(1000);
  if (patchCord3) {
    delete patchCord3;
    patchCord3 = nullptr;
  }
  if (patchCord4) {
    delete patchCord4;
    patchCord4 = nullptr;
  }
  patchCord3 = new AudioConnection(i2sHex1, 0, fft1024CH3, 0);
  patchCord4 = new AudioConnection(i2sHex1, 1, fft1024CH4, 0);
  AudioInterrupts();
  if (fft1024CH3.available()) {
    Serial.print("Channel 3 ");
    //for (int j = 0; j < numberOfRunsToAverage; j++) {
    memset(fftCH3Bins, 0, sizeof(fftCH3Bins));  // Reset the averaged values
    for (int i = 0; i < numberOfBins; i++) {    // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024CH3.read(i);
      fftCH3Bins[i] += val;
        /*
        Serial.print(startFreq, 1);
        Serial.print("-");
        Serial.print(endFreq, 1);
        Serial.print(": ");
        Serial.print(val, 6);
        Serial.print("  ");
        
      }

      //Serial.println("End of run d");
    }
    Serial.println("Averages for each bin:");
    for (int k = 0; k < numberOfBins; k++) {
      float avgVal = fftCH3Bins[k] / numberOfRunsToAverage;  // Calculate the average
      float startFreq = k * binWidth;
      float endFreq = (k + 1) * binWidth;

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(avgVal, 6);
      Serial.print("  ");
    */}
        Serial.println("  ");
  }
  if (fft1024CH4.available()) {
    Serial.print("Channel 4 ");
    //for (int j = 0; j < numberOfRunsToAverage; j++) {
    memset(fftCH4Bins, 0, sizeof(fftCH4Bins));  // Reset the averaged values
    for (int i = 0; i < numberOfBins; i++) {    // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024CH4.read(i);
      fftCH4Bins[i] += val;
        /*
        Serial.print(startFreq, 1);
        Serial.print("-");
        Serial.print(endFreq, 1);
        Serial.print(": ");
        Serial.print(val, 6);
        Serial.print("  ");
        
      }

      //Serial.println("End of run d");
    }
    Serial.println("Averages for each bin:");
    for (int k = 0; k < numberOfBins; k++) {
      float avgVal = fftCH4Bins[k] / numberOfRunsToAverage;  // Calculate the average
      float startFreq = k * binWidth;
      float endFreq = (k + 1) * binWidth;

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(avgVal, 6);
      Serial.print("  ");
    */}
        Serial.println("  ");
  }
  endTimePair = micros();
  pairTime = endTimePair - startTimePair;
  Serial.print("Measurement on mic 3/4 done in ");
  Serial.print(pairTime);
  Serial.println(" microseconds");


  for (int q = 0; q < numberOfBins; q++) {
    float startFreq = q * binWidth;
    float endFreq = (q + 1) * binWidth;

    Serial.print(startFreq, 1);
    Serial.print("-");
    Serial.print(endFreq, 1);
    Serial.print(": ");
    Serial.print(fftCH3Bins[q], 6);
    Serial.print("  ");
  }
  Serial.println();

  for (int q = 0; q < numberOfBins; q++) {
    float startFreq = q * binWidth;
    float endFreq = (q + 1) * binWidth;

    Serial.print(startFreq, 1);
    Serial.print("-");
    Serial.print(endFreq, 1);
    Serial.print(": ");
    Serial.print(fftCH4Bins[q], 6);
    Serial.print("  ");
  }
  startTimePair = micros();
  AudioNoInterrupts();
  digitalWrite(CH34Pin, LOW);
  delay(1000);
  digitalWrite(CH56Pin, HIGH);
  delay(1000);
  if (patchCord5) {
    delete patchCord5;
    patchCord5 = nullptr;
  }
  if (patchCord6) {
    delete patchCord6;
    patchCord6 = nullptr;
  }
  patchCord5 = new AudioConnection(i2sHex1, 0, fft1024CH5, 0);
  patchCord6 = new AudioConnection(i2sHex1, 1, fft1024CH6, 0);
  AudioInterrupts();

  if (fft1024CH5.available()) {
    Serial.print("Channel 5 ");
    //for (int j = 0; j < numberOfRunsToAverage; j++) {
    memset(fftCH5Bins, 0, sizeof(fftCH5Bins));  // Reset the averaged values
    for (int i = 0; i < numberOfBins; i++) {    // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024CH5.read(i);
      fftCH5Bins[i] += val;
        /*
        Serial.print(startFreq, 1);
        Serial.print("-");
        Serial.print(endFreq, 1);
        Serial.print(": ");
        Serial.print(val, 6);
        Serial.print("  ");

      }

      //Serial.println("End of run d");
    }
    Serial.println("Averages for each bin:");
    for (int k = 0; k < numberOfBins; k++) {
      float avgVal = fftCH5Bins[k] / numberOfRunsToAverage;  // Calculate the average
      float startFreq = k * binWidth;
      float endFreq = (k + 1) * binWidth;

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(avgVal, 6);
      Serial.print("  ");
            */}
        Serial.println("  ");
  }
  if (fft1024CH6.available()) {
    Serial.print("Channel 6 ");
    //for (int j = 0; j < numberOfRunsToAverage; j++) {
    memset(fftCH6Bins, 0, sizeof(fftCH6Bins));  // Reset the averaged values
    for (int i = 0; i < numberOfBins; i++) {    // 465 bins go from 0-20025Hz
      float startFreq = i * binWidth;
      float endFreq = (i + 1) * binWidth;
      float val = fft1024CH6.read(i);
      fftCH6Bins[i] += val;
      /*
        Serial.print(startFreq, 1);
        Serial.print("-");
        Serial.print(endFreq, 1);
        Serial.print(": ");
        Serial.print(val, 6);
        Serial.print("  ");
        */
    }

    //Serial.println("End of run d");
    /*}
    Serial.println("Averages for each bin:");
    for (int k = 0; k < numberOfBins; k++) {
      float avgVal = fftCH6Bins[k] / numberOfRunsToAverage;  // Calculate the average
      float startFreq = k * binWidth;
      float endFreq = (k + 1) * binWidth;

      Serial.print(startFreq, 1);
      Serial.print("-");
      Serial.print(endFreq, 1);
      Serial.print(": ");
      Serial.print(avgVal, 6);
      Serial.print("  ");
    }*/
    Serial.println("  ");
  }
  endTimePair = micros();
  pairTime = endTimePair - startTimePair;
  Serial.print("Measurement on mic 5/6 done in ");
  Serial.print(pairTime);
  Serial.println(" microseconds");

  for (int q = 0; q < numberOfBins; q++) {
    float startFreq = q * binWidth;
    float endFreq = (q + 1) * binWidth;

    Serial.print(startFreq, 1);
    Serial.print("-");
    Serial.print(endFreq, 1);
    Serial.print(": ");
    Serial.print(fftCH5Bins[q], 6);
    Serial.print("  ");
  }
  Serial.println();

  for (int q = 0; q < numberOfBins; q++) {
    float startFreq = q * binWidth;
    float endFreq = (q + 1) * binWidth;

    Serial.print(startFreq, 1);
    Serial.print("-");
    Serial.print(endFreq, 1);
    Serial.print(": ");
    Serial.print(fftCH6Bins[q], 6);
    Serial.print("  ");
  }
  digitalWrite(CH56Pin, LOW);
  delay(1000);
  Serial.println();
  endTimeTotal = micros();
  totalTime = endTimeTotal - startTimeTotal;
  Serial.print("Measurement in total done in ");
  Serial.print(totalTime);
  Serial.println(" microseconds");
  delay(2000);
  loopNumber++;
}