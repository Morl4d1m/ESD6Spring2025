%clc;
%clear;
%close all;

% === User parameters ===
fs = 44100;  % Sampling rate (Hz)
output_dir = "C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\individualMicImpulseResponses\";

% === Signal file paths ===
file_paths = {
    'sweepCombinedMic1.csv', ...
    'sine520CombinedMic1.csv', ...
    'whiteNoiseCombinedMic1.csv', ...
    'sineShifted180CombinedMic1.csv' ...
    'sweepCombinedMic2.csv', ...
    'sine520CombinedMic2.csv', ...
    'whiteNoiseCombinedMic2.csv', ...
    'sineShifted180CombinedMic2.csv' ...
    'sweepCombinedMic3.csv', ...
    'sine520CombinedMic3.csv', ...
    'whiteNoiseCombinedMic3.csv', ...
    'sineShifted180CombinedMic3.csv' ...
    'sweepCombinedMic4.csv', ...
    'sine520CombinedMic4.csv', ...
    'whiteNoiseCombinedMic4.csv', ...
    'sineShifted180CombinedMic4.csv' ...
    'sweepCombinedMic5.csv', ...
    'sine520CombinedMic5.csv', ...
    'whiteNoiseCombinedMic5.csv', ...
    'sineShifted180CombinedMic5.csv' ...
    'sweepCombinedMic6.csv', ...
    'sine520CombinedMic6.csv', ...
    'whiteNoiseCombinedMic6.csv', ...
    'sineShifted180CombinedMic6.csv' ...
    'sweepCombinedMic7.csv', ...
    'sine520CombinedMic7.csv', ...
    'whiteNoiseCombinedMic7.csv', ...
    'sineShifted180CombinedMic7.csv' ...
    'sweepCombinedMic8.csv', ...
    'sine520CombinedMic8.csv', ...
    'whiteNoiseCombinedMic8.csv', ...
    'sineShifted180CombinedMic8.csv'
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
    N = min(length(x), length(y));
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

    %subplot(2, 1, 2);
    plot(f, H_dB, 'b');
    xlabel('Frequency (Hz)');
    ylabel('Magnitude (dB)');
    title(sprintf('Frequency Response (Magnitude) - %s', file_paths{i}));
    xlim([0 1250]);
    xticks([0:20:1250]);
    grid on;
    
set(findall(gcf,'-property','FontSize'),'FontSize',24);

    % === Save plots ===
    %saveas(gcf, fullfile(output_dir, sprintf('%s_impulse_response.png', file_paths{i}(1:end-4))));
    %close(gcf);  % Close the figure after saving
end
