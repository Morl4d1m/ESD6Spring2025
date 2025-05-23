// mls_teensy.cpp
#include "mls_teensy.h"

MLS::MLS(int n_bits) {
    setBits(n_bits);
}

void MLS::setBits(int n_bits) {
    if (n_bits < 2)
        nbits = 2;
    else if (n_bits > MAX_NBITS)
        nbits = MAX_NBITS;
    else
        nbits = n_bits;
    initTaps();
}

int MLS::size() {
    return (1 << nbits) - 1; // 2^n - 1
}

void MLS::initTaps() {
    // Set taps according to nbits
    static const int tapsTable[33][4] = {
        {}, // 0
        {}, // 1
        {1, -1}, // 2
        {2, -1}, // 3
        {3, -1}, // 4
        {3, -1}, // 5
        {5, -1}, // 6
        {6, -1}, // 7
        {7,6,1,-1}, // 8
        {5, -1}, // 9
        {7, -1}, // 10
        {9, -1}, // 11
        {11,10,4,-1}, // 12
        {12,11,8,-1}, // 13
        {13,12,2,-1}, // 14
        {14, -1}, // 15
        {15,13,4,-1}, // 16
        {14, -1}, // 17
        {11, -1}, // 18
        {18,17,14,-1}, // 19
        {17, -1}, // 20
        {19, -1}, // 21
        {21, -1}, // 22
        {18, -1}, // 23
        {23,22,17,-1}, // 24
        {22, -1}, // 25
        {25,24,20,-1}, // 26
        {26,25,22,-1}, // 27
        {25, -1}, // 28
        {27, -1}, // 29
        {29,28,7,-1}, // 30
        {28, -1}, // 31
        {31,30,10,-1} // 32
    };
    taps = tapsTable[nbits];
    
    numTaps = 0;
    while (taps[numTaps] != -1 && numTaps < 4) {
        numTaps++;
    }
}

void MLS::getSeq(bool* seq_out) {
    bool state[MAX_NBITS] = {false};
    int length = (1 << nbits) - 1;
    bool feedback;
    int idx;

    do {
        uint32_t randomNum = random(0xFFFFFFFF);

        // Load initial random state
        for (int i = 0; i < nbits; i++) {
            state[i] = (randomNum >> i) & 1;
        }

        idx = 0;
        for (int i = 0; i < length; i++) {
            feedback = state[idx];
            seq_out[i] = feedback;
            for (int ti = 0; ti < numTaps; ti++) {
                feedback ^= state[(taps[ti] + idx) % nbits];
            }
            state[idx] = feedback;
            idx = (idx + 1) % nbits;
        }
    } while (!goodSeq(seq_out));
}

bool MLS::goodSeq(const bool* seq) {
    int length = (1 << nbits) - 1;
    int numZeros = 0;
    for (int i = 0; i < length; i++) {
        if (!seq[i]) numZeros++;
    }
    return (numZeros != length);
}
