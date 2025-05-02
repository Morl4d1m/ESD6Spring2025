% analyze_sweep.m
clear; close all;

% Parameters
fs = 44100;                % sampling rate
filename = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\teensyGeneratedToneSweep\sweep.csv';

% Load data (as a single column of floats)
data = csvread(filename);

% Time vector
N = length(data);
t = (0:N-1)'/fs;

% Plot time-domain signal
figure;
plot(t, data);
xlabel('Time (s)');
ylabel('Amplitude');
xlim([0.01 0.05])
title('Time-Domain Sine Sweep (1–50 Hz over 10 s)');
grid on;

% Compute FFT
X = fft(data);
f = (0:N-1)*(fs/N);
mag = abs(X)/N;

% Plot single-sided amplitude spectrum
half = 1:floor(N/2);
figure;
plot(f(half), mag(half));
xlabel('Frequency (Hz)');
ylabel('Magnitude');
title('Amplitude Spectrum of the Sweep');
grid on;

% Estimate and plot Power Spectral Density using Welch’s method
window = hamming(1024);
noverlap = 512;
nfft = 2048;
[pxx, f_welch] = pwelch(data, window, noverlap, nfft, fs);

figure;
plot(f_welch, 10*log10(pxx));
xlabel('Frequency (Hz)');
ylabel('PSD (dB/Hz)');
title('Power Spectral Density (Welch)');
grid on;
