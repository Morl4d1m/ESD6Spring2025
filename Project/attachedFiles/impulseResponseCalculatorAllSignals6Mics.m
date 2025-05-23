clc;
clear;
close all;

% === User parameters ===
fs = 44100;  % Sampling rate (Hz)
output_dir = "E:\";

% === Signal file paths ===
file_paths = {
    'sweepEggShellFoamCH12.csv', ...
    %{
'sine500EggShellFoamCH12.csv', ...
    'whiteNoiseEggShellFoamCH12.csv', ...
    'sineShifted180EggShellFoamCH12.csv', ... 
    'sweepEggShellFoamCH34.csv', ...
    'sine500EggShellFoamCH34.csv', ...
    'whiteNoiseEggShellFoamCH34.csv', ...
    'sineShifted180EggShellFoamCH34.csv', ... 
    'sweepEggShellFoamCH56.csv', ...
    'sine500EggShellFoamCH56.csv', ...
    'whiteNoiseEggShellFoamCH56.csv', ...
    'sineShifted180EggShellFoamCH56.csv'
    %}
    
    %'fullSweepCombined2Mics.csv'
};

% === Loop over each signal type ===
for i = 1:length(file_paths)
    % Construct the full file path
    filename = fullfile(output_dir, file_paths{i});
    
    % === Load signals ===
    data = readmatrix(filename);  % Each row: [mixer, mic1, mic2]
    x = data(:, 1);  % Played signal (mixer)
    y = data(:, 2);  % Measured response (mic1)
    z = data(:, 3);  % Measured response (mic2)

    % === Duration ===
    durationx = length(x) / fs;
    fprintf("Signal: %s\n", file_paths{i});
    fprintf("Duration mixer: %.2f seconds\n", durationx);
    durationy = length(y) / fs;
    fprintf("Duration mic1: %.2f seconds\n", durationy);
    durationz = length(z) / fs;
    fprintf("Duration mic2: %.2f seconds\n", durationz);

    % === Ensure same length ===
    N = 32768;%min(min(length(x), length(y)), length(z));
    x = x(1:N);
    y = y(1:N);
    z = z(1:N);

    % === Deconvolution via inverse filtering ===
    % Padding to next power of 2
    Nfft = 2^nextpow2(2 * N);

    X = fft(x, Nfft);
    Y = fft(y, Nfft);
    Z = fft(z, Nfft);

    % Regularization to avoid divide-by-zero
    epsilon = 1e-10;
    H1 = Y ./ (X + epsilon);   % Frequency response
    h1 = real(ifft(H1));        % Impulse response (discard imaginary part)
    H2 = Z ./ (X + epsilon);   % Frequency response
    h2 = real(ifft(H2));        % Impulse response (discard imaginary part)

    % Trim impulse response
    h1 = h1(1:N);
    h2 = h2(1:N);

    % === Time axis ===
    t1 = (0:N-1) / fs;
    t2 = (0:N-1) / fs;

    % === Frequency axis ===
    f1 = (0:Nfft / 2 - 1) * fs / Nfft;
    f2 = (0:Nfft / 2 - 1) * fs / Nfft;
%{
    % === Plot Impulse Response === mic 1
    figure;
    plot(t1, h1, 'k');
    xlabel('Time (s)');
    ylabel('Amplitude');
    title(sprintf('Impulse Response (Time Domain) - %s', file_paths{i}));
    grid on;
    
set(findall(gcf,'-property','FontSize'),'FontSize',24);

    % === Plot Magnitude Frequency Response === mic 1
    
%}
H_dB1 = -20 * log10(abs(H1(1:Nfft / 2)));
%{


    figure;
    plot(f1, H_dB1, 'b');
    xlabel('Frequency (Hz)');
    ylabel('Magnitude (dB)');
    title(sprintf('Frequency Response (Magnitude) - %s', file_paths{i}));
    xlim([0 1250]);
    xticks([0:20:1250]);
    grid on;
    
set(findall(gcf,'-property','FontSize'),'FontSize',24);

    % === Plot Impulse Response === mic 2
    figure;
    plot(t2, h2, 'k');
    xlabel('Time (s)');
    ylabel('Amplitude');
    title(sprintf('Impulse Response (Time Domain) - %s', file_paths{i}));
    grid on;
    
set(findall(gcf,'-property','FontSize'),'FontSize',24);

    % === Plot Magnitude Frequency Response === mic 2
%}
    H_dB2 = -20 * log10(abs(H2(1:Nfft / 2)));
    %{

    figure;
    plot(f2, H_dB2, 'b');
    xlabel('Frequency (Hz)');
    ylabel('Magnitude (dB)');
    title(sprintf('Frequency Response (Magnitude) - %s', file_paths{i}));
    xlim([0 1250]);
    xticks([0:20:1250]);
    grid on;
    
set(findall(gcf,'-property','FontSize'),'FontSize',24);
%}
    % Impulse Response (both mics)
    figure;
    plot(t1, h1, 'b', t2, h2, 'r');
    xlabel('Time (s)');
    ylabel('Amplitude');
    title(sprintf('Impulse Responses - %s', file_paths{i}));
    legend('Mic 1', 'Mic 2');
    grid on;
    
set(findall(gcf,'-property','FontSize'),'FontSize',24);
    
    % Frequency Response (both mics)
    figure;
    plot(f1, H_dB1, 'b', f2, H_dB2, 'r');
    xlabel('Frequency (Hz)');
    ylabel('Magnitude (dB)');
    title(sprintf('Frequency Responses - %s', file_paths{i}));
    legend('Mic 1', 'Mic 2');
    xlim([0 1250]);
    xticks(0:20:1250);
    grid on;

set(findall(gcf,'-property','FontSize'),'FontSize',24);

    % === Save plots ===
    %saveas(gcf, fullfile(output_dir, sprintf('%simpulseresponseFirstWrong.png', file_paths{i}(1:end-4))));
    %close(gcf);  % Close the figure after saving
end
