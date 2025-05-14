#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <arm_math.h>
#include <arm_const_structs.h>

// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
const char* filename = "sweepCombined.csv";
const int fs = 44100;  // Sampling frequency
const float epsilon = 1e-10f;

// Buffer size for each block
const size_t BLOCK_SIZE = 2048;  
// Overlap size for overlap-save
const size_t OVERLAP_SIZE = BLOCK_SIZE / 2; 

// Utility: Compute next power of 2
size_t next_power_of_2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}

void removeIfExists(const char* filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.print("Deleted previous version of: ");
    Serial.println(filename);
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // SD setup
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized");
  delay(1000);  // Ensure system stabilizes
  Serial.println("Setup done");
  removeIfExists("fft_and_impulse.csv");
  computeImpulseResponse();
}

void loop() {
  // put your main code here, to run repeatedly:
}

void computeImpulseResponse() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("SD initialization failed!");
    return;
  }

  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file!");
    return;
  }

  const size_t Nfft = next_power_of_2(BLOCK_SIZE + OVERLAP_SIZE);  // Next power of 2 for FFT length
  float* x_block = (float*)malloc(BLOCK_SIZE * sizeof(float));
  float* y_block = (float*)malloc(BLOCK_SIZE * sizeof(float));

  float* X = (float*)calloc(2 * Nfft, sizeof(float));
  float* Y = (float*)calloc(2 * Nfft, sizeof(float));
  float* Sxy = (float*)calloc(2 * Nfft, sizeof(float));
  float* Sxx = (float*)calloc(2 * Nfft, sizeof(float));

  float* H = (float*)calloc(2 * Nfft, sizeof(float));
  float* h = (float*)malloc(Nfft * sizeof(float));

  if (!x_block || !y_block || !X || !Y || !Sxy || !Sxx || !H || !h) {
    Serial.println("Memory allocation failed!");
    return;
  }

  Serial.println("Memory allocation successful");

  // Prepare FFT instance
  arm_cfft_radix4_instance_f32 S;
  arm_cfft_radix4_init_f32(&S, Nfft, 0, 1);  // FFT forward

  size_t blockCount = 0;
  size_t lastIndex = 0;
  
  // File reading and processing loop
  while (file.available()) {
    size_t N = 0;

    // Read one block of data
    while (file.available() && N < BLOCK_SIZE) {
      String line = file.readStringUntil('\n');
      int commaIndex = line.indexOf(',');
      if (commaIndex > 0) {
        float val1 = line.substring(0, commaIndex).toFloat();
        float val2 = line.substring(commaIndex + 1).toFloat();
        x_block[N] = val1;
        y_block[N] = val2;
        N++;
      }
    }

    // If block is fully read, process it
    if (N > 0) {
      blockCount++;

      // Zero out previous block's FFT data
      memset(X, 0, 2 * Nfft * sizeof(float));
      memset(Y, 0, 2 * Nfft * sizeof(float));

      // Interleave x_block, y_block into X[], Y[]
      for (size_t i = 0; i < N; ++i) {
        X[2 * i] = x_block[i];
        Y[2 * i] = y_block[i];
        X[2 * i + 1] = 0.0f;
        Y[2 * i + 1] = 0.0f;
      }

      // Perform FFT for both X and Y
      arm_cfft_radix4_f32(&S, X);
      arm_cfft_radix4_f32(&S, Y);

      // Accumulate cross-spectral and auto-spectral densities
      for (size_t i = 0; i < Nfft; ++i) {
        float xr = X[2 * i], xi = X[2 * i + 1];
        float yr = Y[2 * i], yi = Y[2 * i + 1];

        // Cross-spectral density Sxy
        Sxy[2 * i] += yr * xr + yi * xi;
        Sxy[2 * i + 1] += yi * xr - yr * xi;

        // Auto-spectral density Sxx (|X|^2)
        float mag_sq = xr * xr + xi * xi;
        Sxx[2 * i] += mag_sq;
      }

      // Compute the transfer function H(f) in frequency domain
      for (size_t i = 0; i < Nfft; ++i) {
        float reSxy = Sxy[2 * i];
        float imSxy = Sxy[2 * i + 1];
        float denom = Sxx[2 * i] + epsilon;

        H[2 * i] = reSxy / denom;
        H[2 * i + 1] = imSxy / denom;
      }

      // Perform IFFT to obtain impulse response
      arm_cfft_radix4_init_f32(&S, Nfft, 1, 1);  // FFT inverse
      arm_cfft_radix4_f32(&S, H);  // Inverse FFT

      // Normalize and extract the real part
      for (size_t i = 0; i < Nfft; ++i) {
        H[2 * i] /= (float)Nfft;
        H[2 * i + 1] /= (float)Nfft;
        h[i] = H[2 * i];  // Impulse response is real part
      }

      // Save the results incrementally to SD
      if (blockCount == 1) {
        File combinedOut = SD.open("fft_and_impulse.csv", FILE_WRITE);
        if (!combinedOut) {
          Serial.println("Failed to open fft_and_impulse.csv!");
          return;
        }
        // Header
        combinedOut.println("Freq_Hz,X_real,X_imag,Y_real,Y_imag,H_real,H_imag,H_mag_dB,H_phase_rad,Impulse_response");
        combinedOut.close();
      }

      // Write FFT data and impulse response to SD
      File combinedOut = SD.open("fft_and_impulse.csv", FILE_WRITE);
      if (!combinedOut) {
        Serial.println("Failed to open fft_and_impulse.csv for writing!");
        return;
      }

      float freqRes = (float)fs / Nfft;
      size_t N_unique = Nfft / 2 + 1;

      for (size_t i = 0; i < N_unique; i++) {
        float freq = i * freqRes;
        float xr = X[2 * i];
        float xi = X[2 * i + 1];
        float yr = Y[2 * i];
        float yi = Y[2 * i + 1];
        float hr = H[2 * i];
        float hi = H[2 * i + 1];

        float x_mag = sqrtf(xr * xr + xi * xi);
        float y_mag = sqrtf(yr * yr + yi * yi);
        float h_mag = sqrtf(hr * hr + hi * hi);

        float mag_dB = abs(20.0f * log10f((y_mag + epsilon) / (x_mag + epsilon)));
        float phase = atan2f(hi, hr);
        float h_val = h[i];

        combinedOut.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", freq, xr, xi, yr, yi, hr, hi, mag_dB, phase, h_val);
      }
      combinedOut.close();
    }
  }

  free(x_block);
  free(y_block);
  free(X);
  free(Y);
  free(Sxy);
  free(Sxx);
  free(H);
  free(h);
  Serial.println("Cleanup done");
}
