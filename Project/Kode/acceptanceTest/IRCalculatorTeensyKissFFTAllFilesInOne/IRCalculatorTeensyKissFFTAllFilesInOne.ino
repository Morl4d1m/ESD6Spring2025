#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include "kiss_fft.h"

kiss_fft_cfg kiss_fft_alloc_psram(int nfft, int inverse_fft) {
    size_t len_needed = 0;

    // First, ask how much memory is needed
    kiss_fft_cfg temp = kiss_fft_alloc(nfft, inverse_fft, nullptr, &len_needed);
    if (temp != nullptr) {
        Serial.println("[WARNING] kiss_fft_alloc unexpectedly returned non-null when querying memory size.");
    }

    Serial.printf("Allocating %zu bytes for KissFFT config\n", len_needed);

    // Now allocate from PSRAM
    void* buffer = extmem_malloc(len_needed);
    if (!buffer) {
        Serial.println("[ERROR] PSRAM allocation for KissFFT failed!");
        return nullptr;
    }

    // Now do the actual allocation using our buffer
    kiss_fft_cfg cfg = kiss_fft_alloc(nfft, inverse_fft, buffer, &len_needed);
    if (!cfg) {
        Serial.println("[ERROR] kiss_fft_alloc returned null even after buffer allocation");
        extmem_free(buffer);
    }

    return cfg;
}

kiss_fft_cfg fwd_cfg;
kiss_fft_cpx* X;
kiss_fft_cpx* Y;

// Forward declaration of the wrapper
void kiss_fft_psram(kiss_fft_cfg cfg, const kiss_fft_cpx* in, kiss_fft_cpx* out);

// Force wrapper into PSRAM
__attribute__((section(".psram"))) void kiss_fft_psram(kiss_fft_cfg cfg, const kiss_fft_cpx* in, kiss_fft_cpx* out) {
  kiss_fft(cfg, in, out);
}

const int chipSelect = BUILTIN_SDCARD;
const char* filename = "sweepCombined.csv";
const int fs = 44100;
const float epsilon = 1e-10f;

extern "C" char* __brkval;
extern "C" char* __sbrk(int incr);

void removeIfExists(const char* filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.print("Deleted previous version of: ");
    Serial.println(filename);
  }
}

size_t next_power_of_2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}

void printMemStatus() {
  char* heap_end = __brkval;
  Serial.printf("Heap usage: %lu bytes\n", (unsigned long)(heap_end - (char*)0x20200000));
}

void setup() {
  Serial.begin(115200);
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized");

  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_OFF);
  CCM_CBCMR = (CCM_CBCMR & ~(CCM_CBCMR_FLEXSPI2_PODF_MASK | CCM_CBCMR_FLEXSPI2_CLK_SEL_MASK))
              | CCM_CBCMR_FLEXSPI2_PODF(4) | CCM_CBCMR_FLEXSPI2_CLK_SEL(2);
  CCM_CCGR7 |= CCM_CCGR7_FLEXSPI2(CCM_CCGR_ON);
  delay(1000);

  Serial.println("Setup done");
  removeIfExists("fft_and_impulse.csv");
  computeImpulseResponse();
}

void loop() {}

__attribute__((noinline)) void computeImpulseResponse() {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Failed to open file!");
    return;
  }

  const size_t max_samples = 32768;
  float* x = (float*)extmem_malloc(max_samples * sizeof(float));
  float* y = (float*)extmem_malloc(max_samples * sizeof(float));

  if (!x || !y) {
    Serial.println("Memory allocation failed (x/y)");
    return;
  }

  size_t N = 0;
  while (file.available() && N < max_samples) {
    String line = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    if (commaIndex > 0) {
      x[N] = line.substring(0, commaIndex).toFloat();
      y[N] = line.substring(commaIndex + 1).toFloat();
      N++;
    }
  }
  file.close();
  Serial.printf("Loaded %zu samples\n", N);

  size_t Nfft = next_power_of_2(2 * N);
  Serial.printf("FFT length: %zu\n", Nfft);

  kiss_fft_cpx* X = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* Y = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H_copy = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* h_time = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));

  if (!X || !Y || !H || !H_copy || !h_time) {
    Serial.println("Memory allocation failed (FFT buffers)");
    return;
  }

  // Zero pad and prepare real input for FFT
  for (size_t i = 0; i < Nfft; i++) {
    X[i].r = (i < N) ? x[i] : 0.0f;
    X[i].i = 0.0f;
    Y[i].r = (i < N) ? y[i] : 0.0f;
    Y[i].i = 0.0f;
  }
  
  kiss_fft_cfg fwd_cfg = kiss_fft_alloc_psram(Nfft, 0);
  kiss_fft_cfg inv_cfg = kiss_fft_alloc_psram(Nfft, 1);
  if (!fwd_cfg || !inv_cfg) {
    Serial.println("KissFFT config alloc failed");
    return;
  }

  // Perform FFT
  kiss_fft_psram(fwd_cfg, X, X);
  kiss_fft_psram(fwd_cfg, Y, Y);
  Serial.println("FFT done");

  // Inverse filter: H = Y / X
  for (size_t i = 0; i < Nfft; i++) {
    float xr = X[i].r, xi = X[i].i;
    float yr = Y[i].r, yi = Y[i].i;
    float denom = xr * xr + xi * xi + epsilon;

    H[i].r = (yr * xr + yi * xi) / denom;
    H[i].i = (yi * xr - yr * xi) / denom;
  }

  // Copy H for frequency domain export
  for (size_t i = 0; i < Nfft; i++) {
    H_copy[i].r = H[i].r;
    H_copy[i].i = H[i].i;
  }

  // IFFT of H to get impulse response
  kiss_fft(inv_cfg, H, h_time);

  // Normalize IFFT output
  for (size_t i = 0; i < Nfft; i++) {
    h_time[i].r /= (float)Nfft;
    h_time[i].i /= (float)Nfft;
  }

  Serial.println("IFFT done");

  // Write results to CSV
  File combinedOut = SD.open("fft_and_impulse.csv", FILE_WRITE);
  if (!combinedOut) {
    Serial.println("Failed to open fft_and_impulse.csv!");
    return;
  }

  combinedOut.println("Freq_Hz,X_real,X_imag,Y_real,Y_imag,H_real,H_imag,H_mag_dB,H_phase_rad,Impulse_response");

  float freqRes = (float)fs / Nfft;
  size_t N_unique = Nfft / 2 + 1;

  for (size_t i = 0; i < N_unique; i++) {
    float freq = i * freqRes;
    float xr = X[i].r, xi = X[i].i;
    float yr = Y[i].r, yi = Y[i].i;
    float hr = H_copy[i].r, hi = H_copy[i].i;

    float x_mag = sqrtf(xr * xr + xi * xi);
    float y_mag = sqrtf(yr * yr + yi * yi);
    float h_mag = sqrtf(hr * hr + hi * hi);
    float mag_dB = abs(20.0f * log10f((y_mag + epsilon) / (x_mag + epsilon)));
    float phase = atan2f(hi, hr);
    float h_val = h_time[i].r;

    combinedOut.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", freq, xr, xi, yr, yi, hr, hi, mag_dB, phase, h_val);
  }

  combinedOut.close();
  Serial.println("All FFT data + impulse response written to fft_and_impulse.csv");

  // Cleanup
  extmem_free(fwd_cfg);
  extmem_free(inv_cfg);
  extmem_free(x); extmem_free(y);
  extmem_free(X); extmem_free(Y); extmem_free(H); extmem_free(H_copy); extmem_free(h_time);
  Serial.println("Cleanup done");
}