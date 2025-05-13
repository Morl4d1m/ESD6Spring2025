
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <arm_math.h>
#include <arm_const_structs.h>


// SD card stuff
const int chipSelect = BUILTIN_SDCARD;
File mixerFile;
File micFile;
File combinedFile;
const char* filename = "sweepCombined.csv";
const int fs = 44100;
const float epsilon = 1e-10f;

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
    while (1)
      ;
  }
  Serial.println("SD card initialized");
  delay(1000);  // Ensure system stabilizes
Serial.println("Setup done");
removeIfExists("impulseResponse.csv");
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

    const size_t max_samples = 2048;  // Safe for Teensy 4.1
    float* x = (float*)malloc(max_samples * sizeof(float));
    float* y = (float*)malloc(max_samples * sizeof(float));
    size_t N = 0;

    if (!x || !y) {
        Serial.println("Memory allocation failed (x/y)");
        return;
    }
    Serial.println("Memory allocation successful");

    while (file.available() && N < max_samples) {
        String line = file.readStringUntil('\n');
        int commaIndex = line.indexOf(',');
        if (commaIndex > 0) {
            float val1 = line.substring(0, commaIndex).toFloat();
            float val2 = line.substring(commaIndex + 1).toFloat();
            x[N] = val1;
            y[N] = val2;
            N++;
        }
    }
    file.close();
    Serial.printf("Loaded %zu samples\n", N);

    size_t Nfft = next_power_of_2(2 * N);
    Serial.printf("FFT length: %zu\n", Nfft);

    float* X = (float*)calloc(2 * Nfft, sizeof(float));
    float* Y = (float*)calloc(2 * Nfft, sizeof(float));
    float* H = (float*)calloc(2 * Nfft, sizeof(float));
    float* h = (float*)malloc(Nfft * sizeof(float));

    if (!X || !Y || !H || !h) {
        Serial.println("Memory allocation failed (FFT buffers)");
        return;
    }
    Serial.println("Memory allocation successful (FFT buffers)");

    for (size_t i = 0; i < Nfft; i++) {
        X[2 * i]     = (i < N) ? x[i] : 0.0f;
        X[2 * i + 1] = 0.0f;
        Y[2 * i]     = (i < N) ? y[i] : 0.0f;
        Y[2 * i + 1] = 0.0f;
    }
    Serial.println("Interleaving complete");

    // === FFT (Radix-4) ===
    arm_cfft_radix4_instance_f32 S;
    arm_cfft_radix4_init_f32(&S, Nfft, 0, 1);
    Serial.println("FFT init done");

    arm_cfft_radix4_f32(&S, X);
    Serial.println("FFT X done");
    arm_cfft_radix4_f32(&S, Y);
    Serial.println("FFT Y done");

    for (size_t i = 0; i < Nfft; i++) {
        float xr = X[2 * i];
        float xi = X[2 * i + 1];
        float yr = Y[2 * i];
        float yi = Y[2 * i + 1];

        float denom = xr * xr + xi * xi + epsilon;
        H[2 * i]     = (yr * xr + yi * xi) / denom;
        H[2 * i + 1] = (yi * xr - yr * xi) / denom;
    }
    Serial.println("Deconvolution done");

    arm_cfft_radix4_init_f32(&S, Nfft, 1, 1);
    arm_cfft_radix4_f32(&S, H);
    Serial.println("IFFT done");

    for (size_t i = 0; i < N; i++) {
        h[i] = H[2 * i];
    }
    Serial.println("Extract real part done");

    File out = SD.open("impulseResponse.csv", FILE_WRITE);
    if (!out) {
        Serial.println("Failed to open output file!");
    } else {
        for (size_t i = 0; i < N; i++) {
            out.printf("%f\n", h[i]);
        }
        out.close();
        Serial.println("Impulse response written to impulseResponse.csv");
    }

    free(x); free(y);
    free(X); free(Y); free(H); free(h);
    Serial.println("Cleanup done");
}

