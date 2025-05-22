#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include "kiss_fft.h"

// === PSRAM allocator for KissFFT ===
kiss_fft_cfg kiss_fft_alloc_psram(int nfft, int inverse_fft) {
  size_t len_needed = 0;
  // Query memory requirement
  kiss_fft_alloc(nfft, inverse_fft, nullptr, &len_needed);
  Serial.printf("Allocating %zu bytes for KissFFT config\n", len_needed);

  void* buffer = extmem_malloc(len_needed);
  if (!buffer) {
    Serial.println("[ERROR] PSRAM allocation for KissFFT failed!");
    return nullptr;
  }
  kiss_fft_cfg cfg = kiss_fft_alloc(nfft, inverse_fft, buffer, &len_needed);
  if (!cfg) {
    Serial.println("[ERROR] kiss_fft_alloc returned null even after buffer allocation");
    extmem_free(buffer);
  }
  return cfg;
}

// Wrapper to force PSRAM usage
__attribute__((section(".psram")))
void kiss_fft_psram(kiss_fft_cfg cfg, const kiss_fft_cpx* in, kiss_fft_cpx* out) {
  kiss_fft(cfg, in, out);
}

const int chipSelect = BUILTIN_SDCARD;
const int fs = 44100;
const float epsilon = 1e-10f;

size_t next_power_of_2(size_t n) {
  size_t p = 1;
  while (p < n) p <<= 1;
  return p;
}

void removeIfExists(const char* filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
    Serial.printf("Deleted previous version of: %s\n", filename);
  }
}

// Compute and save impulse responses for mic1 & mic2 from a given CSV
void computeImpulseResponse(const char* inputFilename) {
  Serial.printf("\n--- Processing %s ---\n", inputFilename);
  File infile = SD.open(inputFilename);
  if (!infile) {
    Serial.println("Failed to open input file!");
    return;
  }

  const size_t max_samples = 32768;
  float* x  = (float*)extmem_malloc(max_samples * sizeof(float)); // mixer
  float* y1 = (float*)extmem_malloc(max_samples * sizeof(float)); // mic1
  float* y2 = (float*)extmem_malloc(max_samples * sizeof(float)); // mic2
  if (!x || !y1 || !y2) {
    Serial.println("Memory allocation failed for input arrays");
    return;
  }

  // Read CSV: mixer, mic1, mic2
  size_t N = 0;
  while (infile.available() && N < max_samples) {
    String line = infile.readStringUntil('\n');
    int idx1 = line.indexOf(',');
    int idx2 = line.indexOf(',', idx1 + 1);
    if (idx1 > 0 && idx2 > idx1) {
      x [N] = line.substring(0, idx1).toFloat();
      y1[N] = line.substring(idx1+1, idx2).toFloat();
      y2[N] = line.substring(idx2+1).toFloat();
      N++;
    }
  }
  infile.close();
  Serial.printf("Loaded %zu samples from %s\n", N, inputFilename);

  // FFT length = next power of two of 2*N
  size_t Nfft = next_power_of_2(2 * N);
  Serial.printf("FFT length: %zu\n", Nfft);

  // Allocate FFT buffers
  kiss_fft_cpx *X  = (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *Y1 = (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *Y2 = (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *H1 = (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *H2 = (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *H1c= (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *H2c= (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *h1t= (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  kiss_fft_cpx *h2t= (kiss_fft_cpx*)extmem_malloc(Nfft*sizeof(kiss_fft_cpx));
  if (!X||!Y1||!Y2||!H1||!H2||!H1c||!H2c||!h1t||!h2t) {
    Serial.println("Memory allocation failed for FFT buffers");
    return;
  }

  // Zero-pad inputs
  for (size_t i = 0; i < Nfft; i++) {
    X [i].r = (i < N ? x [i] : 0.0f); X [i].i = 0.0f;
    Y1[i].r = (i < N ? y1[i] : 0.0f); Y1[i].i = 0.0f;
    Y2[i].r = (i < N ? y2[i] : 0.0f); Y2[i].i = 0.0f;
  }

  // Create FFT configs
  kiss_fft_cfg fwd = kiss_fft_alloc_psram(Nfft, 0);
  kiss_fft_cfg inv = kiss_fft_alloc_psram(Nfft, 1);
  if (!fwd||!inv) { Serial.println("KissFFT config alloc failed"); return; }

  // Forward FFTs
  kiss_fft_psram(fwd, X,  X);
  kiss_fft_psram(fwd, Y1, Y1);
  kiss_fft_psram(fwd, Y2, Y2);
  Serial.println("FFTs Done");

  // Frequency response: H1 = Y1/X, H2 = Y2/X
  for (size_t i=0; i<Nfft; i++) {
    float xr=X[i].r, xi=X[i].i;
    float denom = xr*xr + xi*xi + epsilon;
    // mic1
    {
      float yr=Y1[i].r, yi=Y1[i].i;
      H1[i].r = (yr*xr + yi*xi)/denom;
      H1[i].i = (yi*xr - yr*xi)/denom;
    }
    // mic2
    {
      float yr=Y2[i].r, yi=Y2[i].i;
      H2[i].r = (yr*xr + yi*xi)/denom;
      H2[i].i = (yi*xr - yr*xi)/denom;
    }
  }

  // Copy for output
  memcpy(H1c, H1, Nfft*sizeof(kiss_fft_cpx));
  memcpy(H2c, H2, Nfft*sizeof(kiss_fft_cpx));

  // Inverse FFT to get impulse responses
  kiss_fft(inv, H1, h1t);
  kiss_fft(inv, H2, h2t);
  // Normalize
  for (size_t i=0; i<Nfft; i++) {
    h1t[i].r /= (float)Nfft;
    h2t[i].r /= (float)Nfft;
  }
  Serial.println("IFFTs Done");

  // Write CSV
  String outName = String(inputFilename);
  outName.replace(".csv", "IRAndFFT.csv");
  removeIfExists(outName.c_str());
  File out = SD.open(outName.c_str(), FILE_WRITE);
  out.println("FreqHz,Xreal,Ximag,Y1real,Y1imag,H1real,H1imag,H1magdB,H1phaserad,h1Impulse,Y2real,Y2imag,H2real,H2imag,H2magdB,H2phaserad,h2Impulse");

  float freqRes = (float)fs/Nfft;
  size_t Nu = Nfft/2 + 1;
  for (size_t i=0; i<Nu; i++) {
    float freq = i*freqRes;
    float xr=X[i].r, xi=X[i].i;
    // mic1
    float y1r=Y1[i].r, y1i=Y1[i].i;
    float h1r=H1c[i].r, h1i=H1c[i].i;
    float mag1 = abs(sqrtf(h1r*h1r + h1i*h1i));
    float dB1  = 20.0f*log10f(mag1 + epsilon);
    float ph1  = atan2f(h1i, h1r);
    float imp1 = h1t[i].r;
    // mic2
    float y2r=Y2[i].r, y2i=Y2[i].i;
    float h2r=H2c[i].r, h2i=H2c[i].i;
    float mag2 = abs(sqrtf(h2r*h2r + h2i*h2i));
    float dB2  = 20.0f*log10f(mag2 + epsilon);
    float ph2  = atan2f(h2i, h2r);
    float imp2 = h2t[i].r;

    out.printf("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
      freq, xr, xi,
      y1r,y1i,h1r,h1i,dB1,ph1,imp1,
      y2r,y2i,h2r,h2i,dB2,ph2,imp2
    );
  }
  out.close();
  Serial.printf("Output written to %s\n", outName.c_str());

  // Cleanup
  extmem_free(x);  extmem_free(y1); extmem_free(y2);
  extmem_free(X);  extmem_free(Y1); extmem_free(Y2);
  extmem_free(H1); extmem_free(H2); extmem_free(H1c); extmem_free(H2c);
  extmem_free(h1t); extmem_free(h2t); extmem_free(fwd); extmem_free(inv);
  Serial.println("Cleanup Done!");
}

void setup() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);

  if (!SD.begin(chipSelect)) {
    Serial.println("SD init failed!");
    while (1);
  }
  Serial.println("SD initialized");

  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String name = entry.name();
    entry.close();
    // Process only CSVs with two mics
    if (name.endsWith("CH12.csv") || name.endsWith("CH34.csv") || name.endsWith("CH56.csv")) {
      computeImpulseResponse(name.c_str());
    }
  }

  Serial.println("All files processed.");
}

void loop() {
  // Nothing to do here
}
