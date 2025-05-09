%clc;
%lear;
%close all;

% === User parameters ===
fs = 44100;  % Sampling rate (Hz)
output_dir = "C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\";  % Change to your directory

% === Signal file paths ===
% Assuming time-domain and frequency-domain data are stored in separate CSV files
timeCSVFilePath = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\IRTimeSweep.csv';   % Change to your time domain file path
freqCSVFilePath = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\impulseResponses\IRFrequencySweep.csv';   % Change to your frequency domain file path

% === Load signals ===
timeData = readmatrix(timeCSVFilePath);  % Time-domain data
freqData = readmatrix(freqCSVFilePath);  % Frequency-domain magnitude data

% === Duration ===
durationx = length(timeData) / fs;
fprintf("Duration time-domain signal: %.2f seconds\n", durationx);
durationy = length(freqData) / fs;
fprintf("Duration frequency-domain signal: %.2f seconds\n", durationy);

% === Time-domain plot ===
timeVec = (0:length(timeData)-1) / fs;  % Time axis

% Plot the time-domain impulse response
figure;
%subplot(2, 1, 1);
plot(timeVec, timeData, 'k', 'LineWidth', 2);
xlabel('Time (s)');
ylabel('Amplitude');
title('Impulse Response (Time Domain)');
%xlim([0 0.01]);
grid on;

% === Frequency-domain plot ===
% Convert frequency magnitude to dB for better visualization
H_dB = 20 * log10(abs(freqData) + eps);  % +eps to avoid log(0)

% Frequency axis for plotting
f = (0:length(freqData)-1) * fs / length(freqData);

% Plot the frequency-domain magnitude response
%subplot(2, 1, 2);
%figure;
%plot(f, H_dB, 'b', 'LineWidth', 2);
%xlabel('Frequency (Hz)');
%ylabel('Magnitude (dB)');
%title('Frequency Response (Magnitude) - dB');
%xlim([0 1250]);  % Adjust as needed
%xticks(0:200:1250);  % Adjust as needed
%grid on;

% === Save plots ===
% Save the figure as PNG
%saveas(gcf, fullfile(output_dir, 'impulse_response_plot.png'));
%close(gcf);  % Close the figure after saving
