# Python Example: 3-bit MLS using polynomial x^3 + x + 1
# The feedback is computed as XOR of the first and third bits.
# Sequence length for a 3-bit MLS is 2^3 - 1 = 7.

def generate_mls_3bit():
    # Initialize state with a nonzero 3-bit value
    state = [1, 0, 0]  # Example initial state: [s0, s1, s2]
    N = 7              # MLS length for 3 bits: 7
    bitstream = []     # List to hold the output bits

    print("Starting 3-bit MLS generation in Python:")
    print("Iteration | Current State | Output Bit | Bitstream")
    print("-----------------------------------------------------")
    
    for i in range(N):
        # For this example, we choose to output the last bit of the state.
        output_bit = state[2]
        bitstream.append(output_bit)
        
        # Print the current iteration, state, output bit and current bitstream
        print(f"{i+1:9} | {state} | {output_bit}         | {bitstream}")
        
        # Compute the feedback using XOR of the first and last bit (taps for x^3+x+1)
        feedback = state[0] ^ state[2]
        
        # Update the state by shifting left and inserting the new feedback bit at the end
        state = [state[1], state[2], feedback]
        
    print("\nFinal MLS Bitstream:", bitstream)

# Run the function to generate the MLS
generate_mls_3bit()
