%clc;
%clear;
%close all;

% === User parameters ===
fs = 44100;                   % Sampling rate (Hz)
output_dir = "E:\";           % Output folder

% === Signal file paths ===
file_paths = {
    'sweepEggShellFoamCH12.csv', ...
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
};

% === Loop over each signal type ===
for i = 1:length(file_paths)
    % Construct full path & load
    filename = fullfile(output_dir, file_paths{i});
    data = readmatrix(filename);      % [mixer, mic1, mic2]
    x = data(:,1);                    % played sweep
    y = data(:,2);                    % mic1 recording
    z = data(:,3);                    % mic2 recording

    % Trim to common length
    N = min([length(x), length(y), length(z)]);
    x = x(1:N);  y = y(1:N);  z = z(1:N);

    % FFT size (zero‑pad to next power of 2)
    Nfft = 2^nextpow2(2*N);

    % FFT of each
    X = fft(x, Nfft);
    Y = fft(y, Nfft);
    Z = fft(z, Nfft);

    % Inverse filtering (with epsilon)
    eps = 1e-10;
    H1 = Y ./ (X + eps);
    H2 = Z ./ (X + eps);

    % Impulse responses (time domain)
    h1 = real(ifft(H1));   
    h2 = real(ifft(H2));
    h1 = h1(1:N);
    h2 = h2(1:N);

    % Time & freq axes
    t = (0:N-1)/fs;
    f = (0:Nfft/2-1)*fs/Nfft;

    % Magnitude (dB) of freq responses
    H1dB = -20*log10(abs(H1(1:Nfft/2)));
    H2dB = -20*log10(abs(H2(1:Nfft/2)));

    % === Plot Impulse Responses ===
    figure;
    plot(t, h1, 'b', t, h2, 'r', 'LineWidth', 1.5);
    xlabel('Time (s)');
    ylabel('Amplitude');
    title(sprintf('Impulse Responses - %s', file_paths{i}));
    legend('Mic 1','Mic 2');
    grid on;
    set(findall(gcf,'-property','FontSize'),'FontSize',24);
    %saveas(gcf, fullfile(output_dir, sprintf('%s_impulse_response.png', file_paths{i}(1:end-4))));
    %close(gcf);

    % === Plot Frequency Responses (Magnitude) ===
    figure;
    plot(f, H1dB, 'b', f, H2dB, 'r', 'LineWidth', 1.5);
    xlabel('Frequency (Hz)');
    ylabel('Magnitude (dB)');
    title(sprintf('Frequency Responses - %s', file_paths{i}));
    legend('Mic 1','Mic 2');
    xlim([0 1250]);
    xticks(0:20:1250);
    grid on;
    set(findall(gcf,'-property','FontSize'),'FontSize',24);
    %saveas(gcf, fullfile(output_dir, sprintf('%s_freq_response.png', file_paths{i}(1:end-4))));
    %close(gcf);
end
