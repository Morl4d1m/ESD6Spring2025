# -*- coding: utf-8 -*-
"""
Created on Sun Mar  2 23:22:55 2025

@author: Christian Lykke
"""

import math

# Room dimensions (meters)
Lx = 4.7
Ly = 4.1
Lz = 3.1

# Speed of sound (m/s)
c = 343.0

# Max frequency (Hz)
f_max = 20000.0

# Maximum number of modes to store (for practicality in a real-world case)
MAX_MODES = 1635125000000

# Set to store unique frequencies (for fast lookup)
unique_frequencies = set()

# List to store frequencies with modes (for sorting and output)
eigenfrequencies = []

def main():
    valid_modes = 0
    threshold = (2 * f_max / c) ** 2

    print("List of unique eigenfrequencies and their modes:")
    print("--------------------------------------------------")

    # Mode counter for updates
    mode_counter = 0
    total_modes = (math.ceil(Lx * math.sqrt(threshold)) + 1) * (math.ceil(Ly * math.sqrt(threshold)) + 1) * (math.ceil(Lz * math.sqrt(threshold)) + 1)

    # Iterate through possible mode values
    for nx in range(0, math.ceil(Lx * math.sqrt(threshold)) + 1):
        for ny in range(0, math.ceil(Ly * math.sqrt(threshold)) + 1):
            for nz in range(0, math.ceil(Lz * math.sqrt(threshold)) + 1):
                # Compute modal ratio
                ratio = (nx / Lx) ** 2 + (ny / Ly) ** 2 + (nz / Lz) ** 2

                if ratio <= threshold:
                    valid_modes += 1

                    # Compute eigenfrequency
                    freq = (c / 2) * math.sqrt(ratio)

                    # Check if frequency is unique (using set for O(1) lookup)
                    if freq not in unique_frequencies and len(eigenfrequencies) < MAX_MODES:
                        unique_frequencies.add(freq)  # Add to set for fast lookup
                        eigenfrequencies.append({
                            'frequency': freq,
                            'nx': nx,
                            'ny': ny,
                            'nz': nz
                        })

                    # Increment mode counter and print progress every 1000 modes
                    mode_counter += 1
                    if mode_counter % 1000 == 0:
                        progress = (mode_counter / total_modes) * 100
                        print(f"Progress: {mode_counter}/{total_modes} modes processed ({progress:.2f}% complete)")

    # Sort eigenfrequencies in ascending order
    eigenfrequencies.sort(key=lambda mode: mode['frequency'])

    # Print unique eigenfrequencies with mode composition
    for mode in eigenfrequencies:
        print(f"f = {mode['frequency']:.2f} Hz  (nx={mode['nx']}, ny={mode['ny']}, nz={mode['nz']})")

    print("--------------------------------------------------")
    print(f"Total number of valid modes: {valid_modes}")
    print(f"Total number of unique eigenfrequencies: {len(eigenfrequencies)}")

if __name__ == "__main__":
    main()
