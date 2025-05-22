% Diagnostic plots for MMM signals: FFT and Phase
clear; clc; close all;

%% USER INPUT
input_csv = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\eggShellFoamTest1\sine1000EggShellFoamCH12.csv';
fs = 44100;            % Sampling rate
Nfft = 2^16;           % FFT length

%% LOAD DATA
data = readmatrix(input_csv);
mixer = data(:,1) - mean(data(:,1));
mic1  = data(:,2) - mean(data(:,2));
mic2  = data(:,3) - mean(data(:,3));

% Pad
L = length(mixer);
if L < Nfft
    mixer(end+1:Nfft) = 0;
    mic1(end+1:Nfft)  = 0;
    mic2(end+1:Nfft)  = 0;
else
    mixer = mixer(1:Nfft);
    mic1  = mic1(1:Nfft);
    mic2  = mic2(1:Nfft);
end

%% FFT
f = (0:Nfft-1)' * fs / Nfft;
MixerF = fft(mixer, Nfft);
Mic1F  = fft(mic1,  Nfft);
Mic2F  = fft(mic2,  Nfft);

% Limit to meaningful range
f_lim = (f >= 20) & (f <= 1220);
f = f(f_lim);
MixerF = MixerF(f_lim);
Mic1F  = Mic1F(f_lim);
Mic2F  = Mic2F(f_lim);

%% PLOT 1: Magnitude Spectrum
figure;
plot(f, 20*log10(abs(MixerF)+eps), 'k-', 'LineWidth', 1.5); hold on;
plot(f, 20*log10(abs(Mic1F)+eps), 'b-', 'LineWidth', 1.5);
plot(f, 20*log10(abs(Mic2F)+eps), 'r-', 'LineWidth', 1.5);
xlabel('Frequency [Hz]');
ylabel('Magnitude [dB]');
title('FFT Magnitude Spectrum (Zoomed)');
legend('Mixer', 'Mic 1', 'Mic 2', 'Location', 'northeast');
grid on;
xlim([20 1220]); ylim auto;

%% PLOT 2: Phase Difference
phase_diff = unwrap(angle(Mic2F) - angle(Mic1F));
figure;
plot(f, phase_diff, 'm-', 'LineWidth', 1.5);
xlabel('Frequency [Hz]');
ylabel('\phi_{Mic2} - \phi_{Mic1} [rad]');
title('Unwrapped Phase Difference between Mic 2 and Mic 1');
grid on;
xlim([20 1220]);
