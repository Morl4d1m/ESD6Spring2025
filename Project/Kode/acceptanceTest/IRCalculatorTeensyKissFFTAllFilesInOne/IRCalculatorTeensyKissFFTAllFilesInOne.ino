#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include "kiss_fft.h"

const int chipSelect = BUILTIN_SDCARD;
const int fs = 44100;
const float epsilon = 1e-10f;

kiss_fft_cfg kiss_fft_alloc_psram(int nfft, int inverse_fft) {
  size_t len_needed = 0;
  kiss_fft_alloc(nfft, inverse_fft, nullptr, &len_needed);
  void* buffer = extmem_malloc(len_needed);
  if (!buffer) return nullptr;
  return kiss_fft_alloc(nfft, inverse_fft, buffer, &len_needed);
}

__attribute__((section(".psram")))
void kiss_fft_psram(kiss_fft_cfg cfg, const kiss_fft_cpx* in, kiss_fft_cpx* out) {
  kiss_fft(cfg, in, out);
}

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

void computeImpulseResponse(const char* inputFilename) {
  Serial.printf("\n--- Processing %s ---\n", inputFilename);
  File file = SD.open(inputFilename);
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
  kiss_fft_cpx* X = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* Y = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* H_copy = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  kiss_fft_cpx* h_time = (kiss_fft_cpx*)extmem_malloc(Nfft * sizeof(kiss_fft_cpx));
  if (!X || !Y || !H || !H_copy || !h_time) {
    Serial.println("Memory allocation failed (FFT buffers)");
    return;
  }

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

  kiss_fft_psram(fwd_cfg, X, X);
  kiss_fft_psram(fwd_cfg, Y, Y);
  Serial.println("FFT done");

  for (size_t i = 0; i < Nfft; i++) {
    float xr = X[i].r, xi = X[i].i;
    float yr = Y[i].r, yi = Y[i].i;
    float denom = xr * xr + xi * xi + epsilon;
    H[i].r = (yr * xr + yi * xi) / denom;
    H[i].i = (yi * xr - yr * xi) / denom;
    H_copy[i] = H[i];
  }

  kiss_fft(inv_cfg, H, h_time);
  for (size_t i = 0; i < Nfft; i++) {
    h_time[i].r /= Nfft;
    h_time[i].i /= Nfft;
  }

  String outFilename = String(inputFilename);
  outFilename.remove(outFilename.length() - 4); // remove ".csv"
  outFilename += "FFTAndIR.csv";
  removeIfExists(outFilename.c_str());

  File outFile = SD.open(outFilename.c_str(), FILE_WRITE);
  if (!outFile) {
    Serial.print("Failed to open output file: ");
    Serial.println(outFilename);
    return;
  }

  outFile.println("Freq_Hz,X_real,X_imag,Y_real,Y_imag,H_real,H_imag,H_mag_dB,H_phase_rad,Impulse_response");
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

    outFile.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", freq, xr, xi, yr, yi, hr, hi, mag_dB, phase, h_val);
  }

  outFile.close();
  Serial.printf("Output written to %s\n", outFilename.c_str());

  extmem_free(fwd_cfg);
  extmem_free(inv_cfg);
  extmem_free(x); extmem_free(y);
  extmem_free(X); extmem_free(Y); extmem_free(H); extmem_free(H_copy); extmem_free(h_time);
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    while (1);
  }
  Serial.println("SD card initialized");

  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;

    String name = entry.name();
    entry.close();

    if (name.endsWith("CH12.csv") || name.endsWith("CH34.csv") || name.endsWith("CH56.csv")) {
      computeImpulseResponse(name.c_str());
    }
  }

  Serial.println("All selected files processed.");
}

void loop() {}
