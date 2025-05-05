%clc;
%clear;
%close all;
% === User parameters ===
fs = 44100;  % Sampling rate (Hz)
filename = "C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\sweepCombined.csv";

% === Load signals ===
data = readmatrix(filename);  % Each row: [mixer, mic]
x = data(:,1);  % Played sweep (mixer)
y = data(:,2);  % Measured response (mic)

% === Duration ===
durationx = length(x) / fs;
fprintf("Duration mixer: %.2f seconds\n", durationx);
durationy = length(y) / fs;
fprintf("Duration mic: %.2f seconds\n", durationy);

% === Ensure same length ===
N = min(length(x), length(y));
x = x(1:N);
y = y(1:N);

% === Deconvolution via inverse filtering ===
% Padding to next power of 2
Nfft = 2^nextpow2(2*N);

X = fft(x, Nfft);
Y = fft(y, Nfft);

% Regularization to avoid divide-by-zero
epsilon = 1e-10;
H = Y ./ (X + epsilon);   % Frequency response
h = real(ifft(H));        % Impulse response (discard imaginary part)

% Trim impulse response
h = h(1:N);

% === Time axis ===
t = (0:N-1) / fs;

% === Frequency axis ===
f = (0:Nfft/2-1) * fs / Nfft;

% === Plot: Impulse response ===
figure;
%subplot(2,1,1);
%plot(t, h, 'k');
%xlabel('Time (s)');
%ylabel('Amplitude');
%title('Impulse Response (Time Domain)');
%grid on;

% === Plot: Magnitude frequency response ===
H_dB = 20 * log10(abs(H(1:Nfft/2)));

%subplot(2,1,2);
plot(f, H_dB, 'b');
xlabel('Frequency (Hz)');
ylabel('Magnitude (dB)');
title('Frequency Response (Magnitude)');
xlim([0 1250]);
xticks([0:20:1250]);
grid on;
