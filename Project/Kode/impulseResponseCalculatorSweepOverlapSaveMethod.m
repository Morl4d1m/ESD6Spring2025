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

% === STFT Parameters ===
window_size = 2048;  % Window size in samples (2048)
overlap_factor = 0.5;  % Overlap factor (50% overlap for 1024 samples overlap)
hop_size = window_size * (1 - overlap_factor);  % Hop size in samples

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
    N = min(length(x), length(y));
    x = x(1:N);
    y = y(1:N);

    % === Ensure the signal length is at least as long as the window size ===
    if length(x) < window_size
        % Pad signal with zeros to meet window size
        x = [x; zeros(window_size - length(x), 1)];
        y = [y; zeros(window_size - length(y), 1)];
    end

    % === Compute STFT of both signals ===
    % Rectangular window (no smoothing, just a boxcar window)
    window = ones(window_size, 1);
    
    % Ensure the signal length is divisible by hop size for correct overlap-add operation
    while mod(length(x), hop_size) ~= 0
        x = [x; 0];  % Pad with zeros to make length divisible by hop_size
        y = [y; 0];  % Same padding for y
    end
    
    % Compute the STFT of the input signals (overlap-add method)
    [Sx, F, T] = spectrogram(x, window, window_size * overlap_factor, window_size, fs);
    [Sy, ~, ~] = spectrogram(y, window, window_size * overlap_factor, window_size, fs);
    
    % === Frequency response (H) ===
    % Frequency response H = Sy / Sx, avoid divide by zero with epsilon
    epsilon = 1e-10;
    H = Sy ./ (Sx + epsilon);

    % === Impulse response (h) ===
    % Compute the inverse STFT to get the impulse response in the time domain
    h = istft(H, fs, 'Window', window, 'OverlapLength', window_size * overlap_factor, 'FFTLength', window_size);

    % === Discard any imaginary components (for plotting) ===
    H = real(H);  % Discard imaginary parts of H
    h = real(h);  % Discard imaginary parts of h

    % === Time axis ===
    t = (0:length(h)-1) / fs;

    % === Frequency axis ===
    % F already contains the full positive frequencies
    % We are interested in plotting the range from 0 Hz to 1250 Hz (for sweep signal)
    f = F;  % Full frequency range

    % === Plot Time Domain (Impulse Response) ===
    figure;
    subplot(2, 1, 1);
    plot(t, h, 'k');
    xlabel('Time (s)');
    ylabel('Amplitude');
    title(sprintf('Impulse Response (STFT) - Time Domain - %s', file_paths{i}));
    grid on;

    % === Frequency Domain (Magnitude) ===
    % Plot the magnitude of the frequency response (only positive frequencies)
    % We now ensure that we are correctly plotting the expected frequency range
    H_dB = -20 * log10(abs(H));  % Magnitude in dB

    subplot(2, 1, 2);
    imagesc(T, f, H_dB);  % Plot magnitude in dB
    axis xy;
    xlabel('Time (s)');
    ylabel('Frequency (Hz)');
    title(sprintf('Frequency Response (Magnitude) - %s', file_paths{i}));
    xlim([T(1), T(end)]);
    ylim([20, 1250]);  % Adjust frequency axis to cover the full range (20 to 1250 Hz)
    colorbar;
    grid on;

    % === Save plots ===
    % saveas(gcf, fullfile(output_dir, sprintf('%s_impulse_response.png', file_paths{i}(1:end-4))));
    % close(gcf);  % Close the figure after saving
end
