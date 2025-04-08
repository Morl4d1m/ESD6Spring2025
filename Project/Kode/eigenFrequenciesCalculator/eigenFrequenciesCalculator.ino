//#include <Arduino.h>
#include <math.h>

#define MAX_MODES 6500 // Adjust based on ESP32 memory limits

// Room dimensions (meters)
const float Lx = 4.7;
const float Ly = 4.1;
const float Lz = 3.1;

// Speed of sound (m/s)
const float c = 343.0;

// Max frequency (Hz)
int f_max = 200.0;

// Struct to store mode information
struct Mode {
    float frequency;
    int nx, ny, nz;
};

// Array to store unique frequencies with modes
Mode eigenfrequencies[MAX_MODES];
int uniqueFreqCount = 0;

// Function to check if frequency already exists in the list
bool isUnique(float freq, int nx, int ny, int nz) {
    for (int i = 0; i < uniqueFreqCount; i++) {
        if (fabs(eigenfrequencies[i].frequency - freq) < 0.1) { // Small tolerance for float comparison
            return false;
        }
    }
    return true;
}

void setup() {
    Serial.begin(115200);

    int validModes = 0;
    float threshold = pow((2 * f_max / c), 2);

    Serial.println("List of unique eigenfrequencies and their modes:");
    Serial.println("--------------------------------------------------");

    // Iterate through possible mode values
    for (int nx = 0; nx <= ceil(Lx * sqrt(threshold)); nx++) {
        for (int ny = 0; ny <= ceil(Ly * sqrt(threshold)); ny++) {
            for (int nz = 0; nz <= ceil(Lz * sqrt(threshold)); nz++) {

                // Compute modal ratio
                float ratio = pow(nx / Lx, 2) + pow(ny / Ly, 2) + pow(nz / Lz, 2);

                if (ratio <= threshold) {
                    validModes++;

                    // Compute eigenfrequency
                    float freq = (c / 2) * sqrt(ratio);

                    // Store unique frequency if not already present
                    if (isUnique(freq, nx, ny, nz) && uniqueFreqCount < MAX_MODES) {
                        eigenfrequencies[uniqueFreqCount].frequency = freq;
                        eigenfrequencies[uniqueFreqCount].nx = nx;
                        eigenfrequencies[uniqueFreqCount].ny = ny;
                        eigenfrequencies[uniqueFreqCount].nz = nz;
                        uniqueFreqCount++;
                        Serial.println(uniqueFreqCount);
                    }
                }
            }
        }
    }

    // Sort eigenfrequencies in ascending order
    for (int i = 0; i < uniqueFreqCount - 1; i++) {
        for (int j = i + 1; j < uniqueFreqCount; j++) {
            if (eigenfrequencies[i].frequency > eigenfrequencies[j].frequency) {
                Mode temp = eigenfrequencies[i];
                eigenfrequencies[i] = eigenfrequencies[j];
                eigenfrequencies[j] = temp;
            }
        }
    }

    // Print unique eigenfrequencies with mode composition
    for (int i = 0; i < uniqueFreqCount; i++) {
        Serial.printf("f = %.2f Hz  (nx=%d, ny=%d, nz=%d)\n", 
                      eigenfrequencies[i].frequency, 
                      eigenfrequencies[i].nx, 
                      eigenfrequencies[i].ny, 
                      eigenfrequencies[i].nz);
    }

    Serial.println("--------------------------------------------------");
    Serial.printf("Total number of valid modes: %d\n", validModes);
    Serial.printf("Total number of unique eigenfrequencies: %d\n", uniqueFreqCount);
}

void loop() {
    // Nothing to do in loop
}
