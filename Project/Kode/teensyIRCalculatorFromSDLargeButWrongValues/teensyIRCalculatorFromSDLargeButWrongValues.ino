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

  const size_t Nfft = next_power_of_2(BLOCK_SIZE + OVERLAP_SIZE);
  float* x_block = (float*)malloc(BLOCK_SIZE * sizeof(float));
  float* y_block = (float*)malloc(BLOCK_SIZE * sizeof(float));
  float* overlap_x = (float*)malloc(OVERLAP_SIZE * sizeof(float));
  float* overlap_y = (float*)malloc(OVERLAP_SIZE * sizeof(float));

  float* X = (float*)calloc(2 * Nfft, sizeof(float));
  float* Y = (float*)calloc(2 * Nfft, sizeof(float));
  float* Sxy = (float*)calloc(2 * Nfft, sizeof(float));
  float* Sxx = (float*)calloc(2 * Nfft, sizeof(float));
  float* H = (float*)calloc(2 * Nfft, sizeof(float));
  float* h = (float*)malloc(Nfft * sizeof(float));

  if (!x_block || !y_block || !overlap_x || !overlap_y || !X || !Y || !Sxy || !Sxx || !H || !h) {
    Serial.println("Memory allocation failed!");
    return;
  }

  Serial.println("Memory allocation successful");

  arm_cfft_radix4_instance_f32 S;
  arm_cfft_radix4_init_f32(&S, Nfft, 0, 1);

  size_t blockCount = 0;
  size_t lastIndex = 0;

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

    if (N > 0) {
      blockCount++;

      memset(X, 0, 2 * Nfft * sizeof(float));
      memset(Y, 0, 2 * Nfft * sizeof(float));

      for (size_t i = 0; i < N; ++i) {
        X[2 * i] = x_block[i];
        Y[2 * i] = y_block[i];
        X[2 * i + 1] = 0.0f;
        Y[2 * i + 1] = 0.0f;
      }

      // Add overlap to the current block
      for (size_t i = 0; i < OVERLAP_SIZE; ++i) {
        X[2 * i] += overlap_x[i];
        Y[2 * i] += overlap_y[i];
      }

      // Perform FFT for both X and Y
      arm_cfft_radix4_f32(&S, X);
      arm_cfft_radix4_f32(&S, Y);

      // Normalize FFT results
      for (size_t i = 0; i < Nfft; i++) {
        X[2 * i] /= (float)Nfft;
        X[2 * i + 1] /= (float)Nfft;
        Y[2 * i] /= (float)Nfft;
        Y[2 * i + 1] /= (float)Nfft;
      }

      // Compute cross-spectral and auto-spectral densities
      for (size_t i = 0; i < Nfft; ++i) {
        float xr = X[2 * i], xi = X[2 * i + 1];
        float yr = Y[2 * i], yi = Y[2 * i + 1];
        
        // Cross Power Spectrum
        Sxy[2 * i] = xr * yr + xi * yi;
        Sxy[2 * i + 1] = xi * yr - xr * yi;
        
        // Auto Power Spectrum of X
        Sxx[2 * i] = xr * xr + xi * xi;
        Sxx[2 * i + 1] = 0;
      }

      // Compute H as the transfer function (Sxy / Sxx)
      for (size_t i = 0; i < Nfft; ++i) {
        float denom = Sxx[2 * i] * Sxx[2 * i] + Sxx[2 * i + 1] * Sxx[2 * i + 1] + epsilon;
        H[2 * i] = (Sxy[2 * i] * Sxx[2 * i] + Sxy[2 * i + 1] * Sxx[2 * i + 1]) / denom;
        H[2 * i + 1] = (Sxy[2 * i] * Sxx[2 * i + 1] - Sxy[2 * i + 1] * Sxx[2 * i]) / denom;
      }

      // Inverse FFT to get the impulse response
      arm_cfft_radix4_init_f32(&S, Nfft, 1, 1);
      arm_cfft_radix4_f32(&S, H);

      // Normalize impulse response
      for (size_t i = 0; i < Nfft; i++) {
        H[2 * i] /= (float)Nfft;
        H[2 * i + 1] /= (float)Nfft;
      }

      // Save the results
      saveResults(blockCount, X, Y, H, h, Nfft);

      // Copy overlap for the next block
      memcpy(overlap_x, &x_block[BLOCK_SIZE - OVERLAP_SIZE], OVERLAP_SIZE * sizeof(float));
      memcpy(overlap_y, &y_block[BLOCK_SIZE - OVERLAP_SIZE], OVERLAP_SIZE * sizeof(float));
    }
  }

  file.close();
  Serial.println("Processing complete.");
}


void saveResults(size_t blockCount, float* X, float* Y, float* H, float* h, size_t Nfft) {
  // Implement logic to save results incrementally to the SD card after processing each block
  File combinedOut = SD.open("fft_and_impulse.csv", FILE_WRITE);
  if (!combinedOut) {
    Serial.println("Failed to open fft_and_impulse.csv!");
    return;
  }

  // For the first block, add headers
  if (blockCount == 1) {
    combinedOut.println("Freq_Hz,X_real,X_imag,Y_real,Y_imag,H_real,H_imag,H_mag_dB,H_phase_rad,Impulse_response");
  }

  float freqRes = (float)fs / Nfft;
  size_t N_unique = Nfft / 2 + 1;

  for (size_t i = 0; i < N_unique; ++i) {
    float freq = i * freqRes;
    float xr = X[2 * i], xi = X[2 * i + 1];
    float yr = Y[2 * i], yi = Y[2 * i + 1];
    float hr = H[2 * i], hi = H[2 * i + 1];

    float mag_dB = 20.0f * log10f(sqrtf(xr * xr + xi * xi) / (sqrtf(yr * yr + yi * yi) + epsilon));
    float phase = atan2f(hi, hr);
    float h_val = (i < Nfft) ? h[i] : 0.0f;

    combinedOut.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", freq, xr, xi, yr, yi, hr, hi, mag_dB, phase, h_val);
  }

  combinedOut.close();
}
