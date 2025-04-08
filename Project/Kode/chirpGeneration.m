% MATLAB Script to Display Linear and Exponential Chirp Signals and Their Spectrograms in Separate Figures

% Parameters
Fs = 20000;                % Sampling Frequency (Hz)
T = 5;                    % Duration of the signal (seconds)
t = linspace(0, T, T*Fs); % Time vector

% Linear Chirp Parameters
f0_linear = 1;           % Start frequency (Hz)
f1_linear = 10000;          % End frequency (Hz)

% Exponential Chirp Parameters
f0_exp = 1;              % Start frequency (Hz)
f1_exp = 10000;             % End frequency (Hz)

% Linear Chirp Signal
linear_chirp = chirp(t, f0_linear, T, f1_linear, 'linear');

% Exponential Chirp Signal
exponential_chirp = chirp(t, f0_exp, T, f1_exp, 'quadratic'); % Quadratic mode for exponential chirp

% --- Linear Chirp Time Domain Plot ---
figure(1); % Create a new figure for the linear chirp
plot(t, linear_chirp);
xlim([0 1]);
title('Linear Chirp');
xlabel('Time (s)');
ylabel('Amplitude');
grid on;

% --- Exponential Chirp Time Domain Plot ---
figure(2); % Create a new figure for the exponential chirp
plot(t, exponential_chirp);
xlim([0 1]);
title('Exponential Chirp');
xlabel('Time (s)');
ylabel('Amplitude');
grid on;

% --- Linear Chirp Spectrogram ---
figure(3); % Create a new figure for the spectrogram of the linear chirp
spectrogram(linear_chirp, 512, 200, 512, Fs, 'yaxis');
title('Spectrogram of Linear Chirp');
colorbar;
xlabel('Time (s)');
ylabel('Frequency (kHz)');
ylabel(colorbar, 'Power (dB)'); % Label the colorbar

% --- Exponential Chirp Spectrogram ---
figure(4); % Create a new figure for the spectrogram of the exponential chirp
spectrogram(exponential_chirp, 512, 200, 512, Fs, 'yaxis');
title('Spectrogram of Exponential Chirp');
colorbar;
xlabel('Time (s)');
ylabel('Frequency (kHz)');
ylabel(colorbar, 'Power (dB)'); % Label the colorbar
