// Audio player developed from the 1-2: Test Hardware example available to Teensy, with specific intention of playing signals used in my ESD6 project
// Current code is meant to function with just the audio shield attached.

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <arm_math.h>
#include <arm_const_structs.h>
#include <vector>
// SD filenames
const char* CSV_FILE    = "sweepCombined.csv";
const char* X_FFT_FILE  = "X_fft.bin";
const char* Y_FFT_FILE  = "Y_fft.bin";
const char* H_FFT_FILE  = "H_fft.bin";
const char* OUT_FILE    = "IRTimeSweep.csv";

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
AudioRecordQueue recordQueue;  // To capture audio data
AudioConnection patchCord7(mixer, 0, recordQueue, 0);

// Microphone recording for impulse responses:
AudioRecordQueue mic1Queue;  // Recordqueue for the microphone
AudioConnection patchCord8(i2sMic1, 0, mic1Queue, 0);

// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
File mixerFile;
File micFile;
File combinedFile;
File impulseResponseFile;

// FFT variables
//const uint16_t blockSizeFFT = 1024;  // Block size for FFT
//const uint32_t maxBlocks = 512;      // Total blocks to use for each signal (1024 * 512 = 524,288 samples)
//const uint32_t N = blockSizeFFT * maxBlocks;
//const uint32_t maxSamples=1024;
//const float epsilon = 1e-10f;

// Impulse response output modes
enum IRMode {
  IR_TIME,
  IR_FREQ,
  IR_SAVE
};

// Global variables
uint16_t count = 1;
const uint8_t ledPin = 13;        // Pin 13 is the builtin LED
uint32_t LFSRBits = 21;           // Change this between 2 and 32
const unsigned long delayUS = 1;  // Delay in microseconds between bits
uint32_t LFSR;
uint32_t mask;
float sineSweepTime = 10;           // Practically decides how fast the frequency is changed in the sine sweep, by saying deltaF/time
const uint16_t sampleRate = 44100;  // Default sample rate
const uint8_t blockSize = 128;      // Default block size for the audio.h library


void setup() {
  Serial.begin(115200);
  AudioMemory(500);

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
  delay(1000);  // Ensure system stabilizes
}


void loop() {
  pass1_hybrid_fft(CSV_FILE,X_FFT_FILE,false);
  pass1_hybrid_fft(CSV_FILE,Y_FFT_FILE,true);
  pass2_H();
  pass3_ifft();

  //recordSine();
  delay(1000);

  //recordPhaseShift();
  delay(1000);

  //recordWhiteNoise();
  delay(1000);

  //recordSineSweep();
  delay(1000);

  //removeIfExists("IRTimeSweep.csv");  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity

  //computeImpulseResponseBlockwise("sweepCombined.csv");//, "IRTimeSweep.csv");
  //computeImpulseResponseFromCSV("sweepCombined.csv");//, IR_TIME);

  //computeImpulseResponseFromCSV("sweepCombined.csv", IR_FREQ);



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
    snprintf(filenameCombined, sizeof(filenameCombined), "sine%dCombined.csv", f);
    Serial.println(filenameCombined);
    removeIfExists(filenameCombined);                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      Serial.println("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordBothToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
    combinedFile.close();                                  // Closes the file on SD
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
    char filenameCombined[30];
    snprintf(filenameCombined, sizeof(filenameCombined), "sineShifted%dCombined.csv", phase);
    Serial.println(filenameCombined);
    removeIfExists(filenameCombined);                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    combinedFile = SD.open(filenameCombined, FILE_WRITE);  // Creates file on the SD card
    if (!combinedFile) {
      Serial.println("Failed to open file.");
      return;  // Exit the function or handle the error
    }
    recordBothToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
    combinedFile.close();                                  // Closes the file on SD
  }
  sineWave.amplitude(0);  // Turns off the sinusoid
  Serial.println("Phaseshift done");
}

void recordWhiteNoise() {
  Serial.println("Recording white noise");
  whiteNoise.amplitude(0.3);                                     // Sets the amplitude for white noise signal
  removeIfExists("whiteNoiseCombined.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
  combinedFile = SD.open("whiteNoiseCombined.csv", FILE_WRITE);  // Creates file on the SD
  if (!combinedFile) {
    Serial.println("Failed to open file.");
    return;  // Exit the function or handle the error
  }
  recordBothToFileSingleFile(combinedFile, sampleRate);  // Saves the current audio to SD
  combinedFile.close();                                  // Closes the file on SD
  whiteNoise.amplitude(0);                               // Turns off the white noise
  Serial.println("White noise done.");
}

void recordSineSweep() {
  Serial.println("Recording sine sweep");
  uint32_t sweepSamples = sineSweepTime * sampleRate;
  removeIfExists("sweepCombined.csv");                      // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
  combinedFile = SD.open("sweepCombined.csv", FILE_WRITE);  // Create file on SD card
  sineSweep.play(1, 20, 1220, sineSweepTime);               // Initializes sine sweep from 20Hz to 1220Hz over 1 second
  recordBothToFileSingleFile(combinedFile, sweepSamples);   // Save both inputs in a .CSV file
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
      for (int i = 0; i < blockSize; i++) {
        float s = (float)buf[i] / 32768.0f;
        mixerFile.print(s, 6);
        mixerFile.print(i < blockSize - 1 ? "," : "\n");
      }
      recordQueue.freeBuffer();
      wroteBlock = true;
    }

    // Mic
    if (mic1Queue.available()) {
      int16_t* buf = mic1Queue.readBuffer();
      for (int i = 0; i < blockSize; i++) {
        float s = (float)buf[i] / 32768.0f;
        micFile.print(s, 6);
        micFile.print(i < blockSize - 1 ? "," : "\n");
      }
      mic1Queue.freeBuffer();
      wroteBlock = true;
    }

    if (wroteBlock) {
      samplesRecorded += blockSize;
    }
  }
}

void recordBothToFileSingleFile(File& file, uint32_t totalSamples) {  // Works much better
  uint32_t samplesRecorded = 0;                                       // Checker for when to stop
  while (samplesRecorded < totalSamples) {
    if (recordQueue.available() && mic1Queue.available()) {  // Checks for data on both signal lines, ensuring that data is present on both simultaneously
      int16_t* bufMixer = recordQueue.readBuffer();
      int16_t* bufMic = mic1Queue.readBuffer();
      for (int i = 0; i < blockSize; i++) {  // Divides signals into blocks to ensure no overflow is happening
        float sMixer = (float)bufMixer[i] / 32768.0f;
        float sMic = (float)bufMic[i] / 32768.0f;
        file.print(sMixer, 6);  // Saves mixer signal to SD card
        file.print(",");        // Saves to SD card
        file.print(sMic, 6);    // Saves mic signal to SD card
        file.print("\n");       // Saves to SD card
      }
      recordQueue.freeBuffer();      // Frees buffer
      mic1Queue.freeBuffer();        // Frees buffer
      samplesRecorded += blockSize;  // Keeps track of how many samples has been saved
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
/*
void computeImpulseResponseFromCSV(const char* filename, IRMode mode) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file.");
    return;
  }

  Serial.println("Starting block-based impulse response computation...");

  // --- Preallocate FFT buffers ---
  float32_t* x_block = (float32_t*)malloc(sizeof(float32_t) * blockSizeFFT);
  float32_t* y_block = (float32_t*)malloc(sizeof(float32_t) * blockSizeFFT);
  float32_t* x_fft = (float32_t*)malloc(sizeof(float32_t) * 2 * blockSizeFFT);  // complex (real+imag)
  float32_t* y_fft = (float32_t*)malloc(sizeof(float32_t) * 2 * blockSizeFFT);
  float32_t* H_fft = (float32_t*)malloc(sizeof(float32_t) * 2 * blockSizeFFT);
  float32_t* h_time = (float32_t*)malloc(sizeof(float32_t) * blockSizeFFT);

  if (!x_block || !y_block || !x_fft || !y_fft || !H_fft || !h_time) {
    Serial.println("Memory allocation failed.");
    file.close();
    return;
  }

  arm_rfft_fast_instance_f32 fft_instance;
  arm_rfft_fast_init_f32(&fft_instance, blockSizeFFT);

  uint32_t samplesRead = 0;
  uint32_t segment = 0;

  if (mode == IR_TIME) {
    removeIfExists("IRTimeSweep.csv");                             // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    impulseResponseFile = SD.open("IRTimeSweep.csv", FILE_WRITE);  // Create file on SD card
  } else if (mode == IR_FREQ) {
    removeIfExists("IRFrequencySweep.csv");                             // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
    impulseResponseFile = SD.open("IRFrequencySweep.csv", FILE_WRITE);  // Create file on SD card
  } else if (mode == IR_SAVE) {
    removeIfExists("IRFrequencySweep.csv");  // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
  }


  while (file.available() && segment < maxBlocks) {
    // --- Load blockSizeFFT samples from file ---
    uint32_t i = 0;
    while (i < blockSizeFFT && file.available()) {
      String line = file.readStringUntil('\n');
      int commaIndex = line.indexOf(',');
      if (commaIndex < 0) continue;

      float x_val = line.substring(0, commaIndex).toFloat();
      float y_val = line.substring(commaIndex + 1).toFloat();

      x_block[i] = x_val;
      y_block[i] = y_val;
      i++;
    }

    if (i < blockSizeFFT) break;  // end of file

    // --- FFT ---
    arm_rfft_fast_f32(&fft_instance, x_block, x_fft, 0);
    arm_rfft_fast_f32(&fft_instance, y_block, y_fft, 0);

    // --- Complex division: H = Y / (X + ε) ---
    for (uint32_t k = 0; k < blockSizeFFT; k += 2) {
      float xr = x_fft[k];
      float xi = x_fft[k + 1];
      float yr = y_fft[k];
      float yi = y_fft[k + 1];

      float denom = xr * xr + xi * xi + epsilon;

      H_fft[k] = (yr * xr + yi * xi) / denom;
      H_fft[k + 1] = (yi * xr - yr * xi) / denom;
    }

    // --- Inverse FFT to get time-domain impulse response block ---
    arm_rfft_fast_f32(&fft_instance, H_fft, h_time, 1);

    // --- Output results ---
    if (mode == IR_TIME) {
      Serial.print("Impulse segment ");
      Serial.println(segment);
      for (uint32_t j = 0; j < blockSizeFFT; j++) {
        impulseResponseFile.println(h_time[j], 6);
        //Serial.println(h_time[j], 6);
      }
    } else if (mode == IR_FREQ) {
      Serial.print("Magnitude spectrum segment ");
      Serial.println(segment);
      for (uint32_t j = 0; j < blockSizeFFT; j += 2) {
        float mag = sqrtf(H_fft[j] * H_fft[j] + H_fft[j + 1] * H_fft[j + 1]);
        impulseResponseFile.println(mag, 6);
        //Serial.println(mag, 6);
      }
      //impulseResponseFile.close();  // Closes the file on SD
    } else if (mode == IR_SAVE) {
      char outName[32];
      snprintf(outName, sizeof(outName), "impulse_seg_%03d.csv", segment);
      File out = SD.open(outName, FILE_WRITE);
      if (out) {
        for (uint32_t j = 0; j < blockSizeFFT; j++) {
          out.println(h_time[j], 6);
        }
        out.close();
        Serial.print("Saved segment: ");
        Serial.println(outName);
      }
    }
    segment++;
  }

  // Cleanup
  free(x_block);
  free(y_block);
  free(x_fft);
  free(y_fft);
  free(H_fft);
  free(h_time);
  impulseResponseFile.close();  // Closes the file on SD
  file.close();

  Serial.println("Impulse response processing complete.");
}
*/

/*

void computeTimeDomainImpulseResponse(const char* inputFilename) {
  File inputFile = SD.open(inputFilename);
  if (!inputFile) {
    Serial.println("Failed to open input file.");
    return;
  }

  removeIfExists("IRTimeSweep.csv");  // Ensure fresh output
  File outputFile = SD.open("IRTimeSweep.csv", FILE_WRITE);
  if (!outputFile) {
    Serial.println("Failed to open output file.");
    inputFile.close();
    return;
  }

  Serial.println("Starting time-domain impulse response calculation...");

  float expected[blockSizeFFT];
  float measured[blockSizeFFT];
  uint32_t totalSamplesProcessed = 0;

  while (inputFile.available() && totalSamplesProcessed < maxBlocks*blockSizeFFT) {
    uint16_t count = 0;

    while (count < blockSizeFFT && inputFile.available()) {
      String line = inputFile.readStringUntil('\n');
      int commaIndex = line.indexOf(',');
      if (commaIndex < 0) continue;

      expected[count] = line.substring(0, commaIndex).trim().toFloat();
      Serial.print("Expected ");
      Serial.println(expected[count]);
      measured[count] = line.substring(commaIndex + 1).trim().toFloat();
      Serial.print("Measured ");
      Serial.println(measured[count]);
      count++;
    }

    // Handle partial block at end of file
    for (uint16_t i = 0; i < count; i++) {
      float impulse = measured[i] - expected[i];  // Adjust based on your definition
      Serial.print("Impulse ");
      Serial.println(impulse);
      outputFile.println(impulse, 6);
    }

    totalSamplesProcessed += count;

    if (count < blockSizeFFT) break;  // End of file
  }

  inputFile.close();
  outputFile.close();
  Serial.print("Finished. Total samples processed: ");
  Serial.println(totalSamplesProcessed);
}


/*
// Function to compute impulse response from CSV file
void computeImpulseResponseFromCSV(const char* filepath, IRMode mode) {
  File file = SD.open(filepath);
  if (!file) {
    Serial.println("Failed to open CSV file.");
    return;
  }

  Serial.println("Reading and parsing signal...");

  float* x = (float*)malloc(N * sizeof(float));
  float* y = (float*)malloc(N * sizeof(float));
  if (!x || !y) {
    Serial.println("Memory allocation failed.");
    file.close();
    return;
  }

  uint32_t sampleIndex = 0;
  char line[256];
  while (file.available() && sampleIndex < N) {
    file.readBytesUntil('\n', line, sizeof(line));
    float v1, v2;
    if (sscanf(line, "%f,%f", &v1, &v2) == 2) {
      x[sampleIndex] = v1;
      y[sampleIndex] = v2;
      sampleIndex++;
    }
  }
  file.close();
  Serial.printf("Read %lu samples.\n", sampleIndex);

  uint32_t Nfft = nextPowerOf2(sampleIndex);  // next power of 2
  arm_cfft_radix4_instance_f32 S;
  arm_cfft_radix4_init_f32(&S, Nfft, 0, 1);  // Forward FFT

  float* X = (float*)calloc(2 * Nfft, sizeof(float));
  float* Y = (float*)calloc(2 * Nfft, sizeof(float));
  float* H = (float*)calloc(2 * Nfft, sizeof(float));
  float* h = (float*)calloc(2 * Nfft, sizeof(float));

  if (!X || !Y || !H || !h) {
    Serial.println("FFT buffer allocation failed.");
    free(x);
    free(y);
    free(X);
    free(Y);
    free(H);
    free(h);
    return;
  }

  // Interleaved real/imag input for FFT
  for (uint32_t i = 0; i < sampleIndex; i++) {
    X[2 * i] = x[i];
    Y[2 * i] = y[i];
  }

  // Compute FFTs
  arm_cfft_radix4_f32(&S, X);
  arm_cfft_radix4_f32(&S, Y);

  // Deconvolution H = Y / (X + epsilon)
  for (uint32_t i = 0; i < Nfft; i++) {
    float Xre = X[2 * i];
    float Xim = X[2 * i + 1];
    float Yre = Y[2 * i];
    float Yim = Y[2 * i + 1];

    float denom = Xre * Xre + Xim * Xim + epsilon;
    H[2 * i] = (Yre * Xre + Yim * Xim) / denom;
    H[2 * i + 1] = (Yim * Xre - Yre * Xim) / denom;
  }

  // Inverse FFT
  arm_cfft_radix4_init_f32(&S, Nfft, 1, 1);  // Inverse FFT
  memcpy(h, H, 2 * Nfft * sizeof(float));
  arm_cfft_radix4_f32(&S, h);

  // Normalize and extract real part
  float norm = 1.0f / Nfft;
  for (uint32_t i = 0; i < sampleIndex; i++) {
    h[2 * i] *= norm;  // Only real part
  }

  // Output
  if (mode == IR_TIME) {
    Serial.println("Impulse Response (Time Domain):");
    for (uint32_t i = 0; i < sampleIndex; i++) {
      removeIfExists("IRTimeSweep.csv");                             // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
      impulseResponseFile = SD.open("IRTimeSweep.csv", FILE_WRITE);  // Create file on SD card
      impulseResponseFile.println(h[2 * i], 6);
      impulseResponseFile.close();  // Closes the file on SD
      Serial.println(h[2 * i], 6);
    }
  } else if (mode == IR_FREQ) {
    Serial.println("Impulse Response (Frequency Magnitude in dB):");
    for (uint32_t i = 0; i < Nfft / 2; i++) {
      float re = H[2 * i];
      float im = H[2 * i + 1];
      float mag = sqrtf(re * re + im * im);
      removeIfExists("IRTimeSweep.csv");                             // Deletes previous versions of the file, so that a new file is created, ensuring data integrity
      impulseResponseFile = SD.open("IRTimeSweep.csv", FILE_WRITE);  // Create file on SD card
      impulseResponseFile.println(20.0f * log10f(mag + epsilon), 2);
      impulseResponseFile.close();  // Closes the file on SD
      Serial.println(20.0f * log10f(mag + epsilon), 2);
    }
  } else if (mode == IR_SAVE) {
    File outFile = SD.open("impulseResponse.csv", FILE_WRITE);
    if (!outFile) {
      Serial.println("Failed to open output file.");
    } else {
      for (uint32_t i = 0; i < sampleIndex; i++) {
        outFile.println(h[2 * i], 6);
      }
      outFile.close();
      Serial.println("Impulse response saved to SD card.");
    }
  }

  // Cleanup
  free(x);
  free(y);
  free(X);
  free(Y);
  free(H);
  free(h);
}
*/
// Helper function to compute next power of 2
uint32_t nextPowerOf2(uint32_t n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n++;
  return n;
}



#include <algorithm>

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------
const uint32_t N        = 441000;
const uint8_t  P        = 1 + 31 - __builtin_clz(2*N - 1);
const uint32_t Nfft     = 1u << P;      // 524288
const float    epsilon  = 1e-10f;


// Helpers
void removeIf(const char* fn){ if (SD.exists(fn)) SD.remove(fn); }
void readBin(File &f, uint32_t k, float &r, float &i){
  f.seek(2*k*sizeof(float));  
  f.read((uint8_t*)&r,sizeof(r));  
  f.read((uint8_t*)&i,sizeof(i));
}
void writeBin(File &f, uint32_t k, float r, float i){
  f.seek(2*k*sizeof(float));  
  f.write((uint8_t*)&r,sizeof(r));  
  f.write((uint8_t*)&i,sizeof(i));
}

// ----------------------------------------------------------------------------
// PASS 1: Hybrid FFT – fuse first 12 stages in-RAM on 4 096-point blocks
// ----------------------------------------------------------------------------
void pass1_hybrid_fft(const char* csvFile, const char* fftFile, bool micCol) {
  Serial.printf("PASS1 (%s) → %s\n", micCol?"mic":"mix", fftFile);

  // 1) Build time-domain complex file "td.bin"
  File in = SD.open(csvFile);
  removeIf("td.bin");
  File td = SD.open("td.bin", FILE_WRITE);
  uint32_t cnt=0;
  while(cnt<N && in.available()){
    String L=in.readStringUntil('\n');
    int c=L.indexOf(',');
    float v = micCol ? L.substring(c+1).toFloat() : L.substring(0,c).toFloat();
    td.write((uint8_t*)&v,sizeof(v));
    float z=0; td.write((uint8_t*)&z,sizeof(z));
    cnt++;
  }
  for(;cnt<Nfft;cnt++){
    float z=0; td.write((uint8_t*)&z,sizeof(z));  td.write((uint8_t*)&z,sizeof(z));
  }
  in.close(); td.close();

  // 2) First 12 stages via in-RAM 4096-point FFT
  const int FUSE = 12;                          // fuse log2(4096)
  const int BLOCK = 1<<(FUSE);                  // 4096
  float buf[2*BLOCK];
  arm_cfft_instance_f32 inst;
  switch(BLOCK){
    case 256:  inst=arm_cfft_sR_f32_len256;  break;
    case 512:  inst=arm_cfft_sR_f32_len512;  break;
    case 1024: inst=arm_cfft_sR_f32_len1024; break;
    case 2048: inst=arm_cfft_sR_f32_len2048; break;
    case 4096: inst=arm_cfft_sR_f32_len4096; break;
  }

  for(uint32_t off=0; off<Nfft; off+=BLOCK){
    File f=SD.open("td.bin", FILE_WRITE);
    // read BLOCK bins
    f.seek(off*2*sizeof(float));
    f.read((uint8_t*)buf,sizeof(buf));
    // 12 in-RAM stages
    arm_cfft_f32(&inst, buf, 0, 0);  
    // write back
    f.seek(off*2*sizeof(float));
    f.write((uint8_t*)buf,sizeof(buf));
    f.close();
  }

  // 3) Remaining P-FUSE stages out-of-core
  for(int s=FUSE; s<P; s++){
    uint32_t stride=1u<<(s+1), half=stride>>1, twStep=Nfft/stride;
    File f=SD.open("td.bin", FILE_WRITE);
    for(uint32_t off=0; off<Nfft; off+=stride){
      std::vector<float> b(2*stride);
      f.seek(off*2*sizeof(float));
      f.read((uint8_t*)b.data(),2*stride*sizeof(float));
      for(uint32_t j=0;j<half;j++){
        float ar=b[2*j], ai=b[2*j+1],
              br=b[2*(j+half)], bi=b[2*(j+half)+1];
        float phi = -2*PI*float(j*twStep)/float(Nfft);
        float wr=cosf(phi), wi=sinf(phi);
        float tr=br*wr - bi*wi, ti=br*wi + bi*wr;
        b[2*j]       = ar+tr;    b[2*j+1]       = ai+ti;
        b[2*(j+half)]= ar-tr;    b[2*(j+half)+1]= ai-ti;
      }
      f.seek(off*2*sizeof(float));
      f.write((uint8_t*)b.data(),2*stride*sizeof(float));
    }
    f.close();
  }

  // rename to fftFile
  removeIf(fftFile);
  SD.rename("td.bin", fftFile);
  Serial.println("PASS1 done.");
}

// ----------------------------------------------------------------------------
// PASS 2: H[k] = Y[k]/(X[k]+ε)
// ----------------------------------------------------------------------------
void pass2_H(){
  Serial.println("PASS2 generate H");
  File fx=SD.open(X_FFT_FILE,FILE_READ),
       fy=SD.open(Y_FFT_FILE,FILE_READ);
  removeIf(H_FFT_FILE);
  File fh=SD.open(H_FFT_FILE,FILE_WRITE);
  const uint32_t C=2048;
  float Xr,Xi,Yr,Yi;
  for(uint32_t k=0;k<Nfft;){
    uint32_t end=min<uint32_t>(k+C,Nfft);
    for(uint32_t i=k;i<end;i++){
      readBin(fx,i,Xr,Xi);
      readBin(fy,i,Yr,Yi);
      float d=Xr*Xr+Xi*Xi+epsilon;
      float Hr=(Yr*Xr+Yi*Xi)/d,
            Hi=(Yi*Xr-Yr*Xi)/d;
      writeBin(fh,i,Hr,Hi);
    }
    k=end;
  }
  fx.close(); fy.close(); fh.close();
  Serial.println("PASS2 done.");
}

// ----------------------------------------------------------------------------
// PASS 3: inverse FFT + trim
// ----------------------------------------------------------------------------
void pass3_ifft(){
  Serial.println("PASS3 IFFT+trim");
  int stages=P;
  float invN=1.0f/float(Nfft);
  for(int s=stages-1;s>=0;s--){
    uint32_t stride=1u<<(s+1), half=stride>>1, tw=Nfft/stride;
    File f=SD.open(H_FFT_FILE,FILE_WRITE);
    for(uint32_t off=0;off<Nfft;off+=stride){
      std::vector<float> b(2*stride);
      f.seek(off*2*sizeof(float));
      f.read((uint8_t*)b.data(),2*stride*sizeof(float));
      for(uint32_t j=0;j<half;j++){
        float ar=b[2*j], ai=b[2*j+1],
              br=b[2*(j+half)], bi=b[2*(j+half)+1];
        float phi=2*PI*float(j*tw)/float(Nfft);
        float wr=cosf(phi), wi=sinf(phi);
        float tr=br*wr - bi*wi, ti=br*wi + bi*wr;
        b[2*j]       =ar+tr;    b[2*j+1]       =ai+ti;
        b[2*(j+half)]=ar-tr;    b[2*(j+half)+1]=ai-ti;
      }
      f.seek(off*2*sizeof(float));
      f.write((uint8_t*)b.data(),2*stride*sizeof(float));
    }
    f.close();
  }

  File in=SD.open(H_FFT_FILE,FILE_READ);
  removeIf(OUT_FILE);
  File out=SD.open(OUT_FILE,FILE_WRITE);
  for(uint32_t i=0;i<N;i++){
    float r,c; in.read((uint8_t*)&r,sizeof(r));
             in.read((uint8_t*)&c,sizeof(c));
    out.println(r*invN,6);
  }
  in.close(); out.close();
  Serial.println("PASS3 done.");
}