clc;
clear;
close all;

% === User parameters ===
fs = 44100;  % Sampling rate (Hz)
output_dir = "C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\1Mic\";

% === Signal file paths ===
file_paths = {
    'sweepCombined.csv'%, ...
    %'sine1020Combined.csv', ...
    %'whiteNoiseCombined.csv', ...
    %'sineShifted180Combined.csv'
};

% === Loop over each signal type ===
for i = 1:length(file_paths)
    % Construct the full file path
    filename = fullfile(output_dir, file_paths{i});
    
    % === Load signals ===
    data = readmatrix(filename);  % Each row: [mixer, mic]
    x = data(:, 1);  % Played signal (mixer)
    y = data(:, 2);  % Measured response (mic)

    % === Duration ===
    durationx = length(x) / fs;
    fprintf("Signal: %s\n", file_paths{i});
    fprintf("Duration mixer: %.2f seconds\n", durationx);
    durationy = length(y) / fs;
    fprintf("Duration mic: %.2f seconds\n", durationy);

    % === Ensure same length ===
    N = 16384;%min(length(x), length(y));
    x = x(1:N);
    y = y(1:N);

    % === Deconvolution via inverse filtering ===
    % Padding to next power of 2
    Nfft = 2^nextpow2(2 * N);

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
    f = (0:Nfft / 2 - 1) * fs / Nfft;

    % === Plot Impulse Response ===
    figure;
    %subplot(2, 1, 1);
    %plot(t, h, 'k');
    %xlabel('Time (s)');
    %ylabel('Amplitude');
    %title(sprintf('Impulse Response (Time Domain) - %s', file_paths{i}));
    %grid on;

    % === Plot Magnitude Frequency Response ===
    H_dB = -20 * log10(abs(H(1:Nfft / 2)));
    H_half = H(1:Nfft/2 + 1);         % positive‐freq bins
    H_phase_rad = angle(H_half);    % phase in radians

    %subplot(2, 1, 2);
    plot(f, H_dB, 'b');
    xlabel('Frequency (Hz)');
    ylabel('Magnitude (dB)');
    title(sprintf('Frequency Response (Magnitude) - %s', file_paths{i}));
    xlim([0 1250]);
    xticks([0:20:1250]);
    grid on;

    % === Save plots ===
    %saveas(gcf, fullfile(output_dir, sprintf('%s_impulse_response.png', file_paths{i}(1:end-4))));
    %close(gcf);  % Close the figure after saving
end
