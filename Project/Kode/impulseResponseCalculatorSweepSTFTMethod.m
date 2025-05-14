clc; clear; close all;

% === User parameters ===
fs        = 44100;
L         = 2048;            % block size
output_dir = "C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\1Mic\";

file      = 'sweepCombined.csv';
epsilon   = 1e-10;

% --- Load & trim ---
data = readmatrix(fullfile(output_dir, file));
x = data(:,1); y = data(:,2);
N = min(length(x), length(y));
x = x(1:N); y = y(1:N);

% --- Full deconvolution in one FFT ---
Nfft = 2^nextpow2(2*N);
X_full = fft(x, Nfft);
Y_full = fft(y, Nfft);
H_full = Y_full ./ (X_full + epsilon);
h_full = real(ifft(H_full));  % full IR

% --- Partition H_full into M blocks of length L in freq-domain ---
M = ceil(Nfft / L);
H_part = zeros(L, M);
for m = 1:M
    idx = (m-1)*L + (1:L);
    H_part(:,m) = H_full(idx);
end

% --- Overlap–save deconvolution streaming ---
h_stream = zeros(Nfft,1);
prev_tail = zeros(L-1,1);
for m = 1:M
    % Inverse-FFT this partition to get time-domain partition impulse g_m
    Gm = real(ifft(H_part(:,m), L));
    % Overlap-save add:
    %   take [prev_tail; zeros] convolved with Gm, append new block
    in_block = [prev_tail; zeros(L,1)];  % length = 2L-1 truncated
    y_block  = conv(in_block, Gm);
    y_valid  = y_block(L:end-1);         % length L
    % place in h_stream
    n0 = (m-1)*L + 1;
    h_stream(n0:n0+L-1) = y_valid;
    % update tail
    prev_tail = in_block(end-(L-2):end);
end
h_stream = h_stream(1:Nfft);

% --- Compare to full IR ---
max_error = max(abs(h_full - h_stream));
fprintf("Max sample-wise error: %.3e\n", max_error);

% --- Plot comparison ---
t = (0:Nfft-1)/fs;
figure;
subplot(2,1,1);
plot(t, h_full, 'k'); hold on;
plot(t, h_stream, 'r--');
title('Impulse Response: full FFT vs. partitioned inverse filtering');
legend('Full FFT','Partitioned');
xlabel('Time (s)'); ylabel('Amplitude');

f = (0:Nfft/2-1)*(fs/Nfft);
H_stream = fft(h_stream, Nfft);
subplot(2,1,2);
plot(f, -20*log10(abs(H_full(1:Nfft/2))), 'k'); hold on;
plot(f, -20*log10(abs(H_stream(1:Nfft/2))), 'r--');
title('Frequency Response Comparison');
xlabel('Hz'); ylabel('Mag (dB)'); legend('Full FFT','Partitioned');
xlim([0 1250]); grid on;
