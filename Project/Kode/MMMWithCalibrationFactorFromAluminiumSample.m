% MMM calculation from 1 kHz sine wave measurement with mic2 alignment
clear; clc; close all;

%% USER PARAMETERS
file = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\eggShellFoamTest1\sine1000EggShellFoamCH12.csv';
fs = 44100;         % Sampling rate [Hz]
Nfft = 2^18;        % High resolution FFT

% Tube geometry
c0 = 343;           % Speed of sound [m/s]
rho0 = 1.21;        % Air density [kg/m^3]
tube_width = 0.1;   % Tube width [m]
tube_height = 0.1;  % Tube height [m]
x1 = 70.71e-3;      % Mic1 (closer to sample)
x2 = 197.99e-3;     % Mic2
s = abs(x2 - x1);   % Distance between mics

%% LOAD AND PREPROCESS DATA
data = readmatrix(file);
mixer = data(:,1) - mean(data(:,1));
mic1  = data(:,2) - mean(data(:,2));
mic2  = data(:,3) - mean(data(:,3));

% Zero-pad
L = length(mic1);
mic1(end+1:Nfft) = 0;
mic2(end+1:Nfft) = 0;
mixer(end+1:Nfft) = 0;

% Get FFTs
F1 = fft(mic1, Nfft);
F2 = fft(mic2, Nfft);
f = (0:Nfft-1)' * fs / Nfft;

% Pick FFT bin closest to 1000 Hz
[~, idx1kHz] = min(abs(f - 1000));
phi1 = angle(F1(idx1kHz));
phi2 = angle(F2(idx1kHz));

% Compute phase difference and required shift
dphi = unwrap(phi2 - phi1);
f0 = f(idx1kHz);
time_delay = dphi / (2*pi*f0);              % in seconds
delay_samples = 176;     % in samples

% Shift mic2
mic2_aligned = circshift(mic2, -delay_samples);
fprintf('Forced delay applied to mic2: %d samples (%.3f ms)\n', ...
    delay_samples, delay_samples / fs * 1000);

% Plot time alignment
t = (0:length(mic1)-1) / fs;
figure;
plot(t, mic1, 'b', t, mic2, 'r', t, mic2_aligned, 'm');
title('Time-Domain Alignment of Microphones');
xlabel('Time [s]'); ylabel('Amplitude');
legend('Mic 1','Mic 2 (raw)','Mic 2 (aligned)'); grid on;

%% FFT AND FREQUENCY SETUP
f = (0:Nfft-1)' * fs / Nfft;
F1 = fft(mic1, Nfft);
F2 = fft(mic2_aligned, Nfft);

[~, idx] = min(abs(f - 1000));
f1kHz = f(idx);

%% TRANSFER FUNCTION
H12 = F2(idx) / F1(idx);

% Wave number k0 (complex)
lambda0 = c0 / f1kHz;
k0_prime = 2 * pi / lambda0;
d = (4 * (tube_width * tube_height)) / (2 * (tube_width + tube_height));
k0_doubleprime = 1.94e-2 * sqrt(f1kHz / c0) * d;
k0 = k0_prime - 1i * k0_doubleprime;

% Incident and reflected wave propagation from x1 to x2
HI = exp(-1i * k0 * (x2 - x1));
HR = exp( 1i * k0 * (x2 - x1));

% Reflection coefficient
r = (H12 - HI) / (HR - H12);

% Absorption coefficient
abs_r = abs(r);
alpha = 1 - abs_r^2;

% Impedance and Admittance
Z_ratio = (1 + r) / (1 - r);
Z = Z_ratio * rho0 * c0;
G = 1 / Z;

%% DISPLAY RESULTS
fprintf('\n--- MMM Results at %.1f Hz (with mic2 aligned) ---\n', f1kHz);
fprintf('H12       : %.4f + %.4fi\n', real(H12), imag(H12));
fprintf('r         : %.4f + %.4fi\n', real(r), imag(r));
fprintf('|r|       : %.4f\n', abs_r);
fprintf('alpha     : %.4f\n', alpha);
fprintf('Z         : %.2f + %.2fi Pa·s/m\n', real(Z), imag(Z));
fprintf('G         : %.4e + %.4ei m/(Pa·s)\n', real(G), imag(G));
fprintf('Delay applied to mic2: %d samples (%.3f ms)\n', delay_samples, delay_samples / fs * 1000);

%% PLOT FREQUENCY DOMAIN RESULTS
figure;
subplot(2,1,1);
plot(f, 20*log10(abs(F1)+eps), 'b'); hold on;
plot(f, 20*log10(abs(fft(mic2, Nfft))+eps), 'r');
plot(f, 20*log10(abs(F2)+eps), 'm');
xline(f1kHz, '--k');
legend('Mic 1','Mic 2 (raw)','Mic 2 (aligned)');
title('FFT Magnitude Spectrum');
xlabel('Frequency [Hz]'); ylabel('Magnitude [dB]');
xlim([800 1200]); grid on;

subplot(2,1,2);
plot(f, unwrap(angle(F2) - angle(F1)), 'm');
xline(f1kHz, '--k');
title('Phase Difference (Mic2 aligned - Mic1)');
xlabel('Frequency [Hz]'); ylabel('Phase [rad]');
xlim([800 1200]); grid on;
