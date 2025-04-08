#include <stdio.h>
#include <stdint.h>

#define N 7   // Length of MLS for a 3-bit LFSR (2^3 - 1)

void print_state(uint8_t state[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%d ", state[i]);
    }
}

int main(void) {
    // Initialize the state of the LFSR (3 bits, nonzero)
    uint8_t state[3] = {1, 0, 0};  // [s0, s1, s2]
    uint8_t output_bit;
    uint8_t bitstream[N];
    
    printf("Starting 3-bit MLS generation in C:\n");
    printf("Iteration | Current State     | Output Bit | Bitstream\n");
    printf("---------------------------------------------------------\n");
    
    for (int i = 0; i < N; i++) {
        // Choose the last bit of the state as the output
        output_bit = state[2];
        bitstream[i] = output_bit;
        
        // Print current iteration, state, output bit and current bitstream
        printf("%9d | ", i + 1);
        print_state(state, 3);
        printf(" | %10d | ", output_bit);
        for (int j = 0; j <= i; j++) {
            printf("%d ", bitstream[j]);
        }
        printf("\n");
        
        // Compute the feedback using XOR of the first and third bits
        uint8_t feedback = state[0] ^ state[2];
        
        // Update the state: shift left and append the feedback
        state[0] = state[1];
        state[1] = state[2];
        state[2] = feedback;
    }
    
    printf("\nFinal MLS Bitstream: ");
    for (int i = 0; i < N; i++) {
        printf("%d ", bitstream[i]);
    }
    printf("\n");
    
    return 0;
}
