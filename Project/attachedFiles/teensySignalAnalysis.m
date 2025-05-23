close all;
% === Configuration ===
Fs = 44100; % Sampling rate
savePlots = questdlg('Do you want to save all plots as PNGs?', ...
    'Save Plots', 'Yes', 'No', 'No');
savePlots = strcmp(savePlots, 'Yes');

% === File paths ===
baseDir = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\';

sineFiles = fullfile(baseDir, 'teensyGeneratedSines', ...
    ["sine20.csv", "sine70.csv", "sine120.csv", "sine170.csv", "sine220.csv", ...
     "sine270.csv", "sine320.csv", "sine370.csv", "sine420.csv", "sine470.csv", ...
     "sine520.csv", "sine570.csv", "sine620.csv", "sine670.csv", "sine720.csv", ...
     "sine770.csv", "sine820.csv", "sine870.csv", "sine920.csv", "sine970.csv", ...
     "sine1020.csv", "sine1070.csv", "sine1120.csv", "sine1170.csv", ...
     "sine1220.csv"]);

phaseShiftedFiles = fullfile(baseDir, 'teensyGeneratedSinesShifted', ...
    ["sineShifted0.csv", "sineShifted90.csv", "sineShifted180.csv", ...
     "sineShifted270.csv", "sineShifted360.csv"]);

sweepFile = fullfile(baseDir, 'teensyGeneratedToneSweep', 'sweep.csv');
noiseFile = fullfile(baseDir, 'teensyGeneratedWhiteNoise', 'white.csv');

%% === Main Analysis ===
%processSignals(sineFiles, 'Sine', Fs, savePlots);
%processSignals(phaseShiftedFiles, 'PhaseShifted', Fs, savePlots);
%processSignals({sweepFile}, 'Sweep', Fs, savePlots);
processSignals({noiseFile}, 'WhiteNoise', Fs, savePlots);

% === Create Final Combined Plot ===
%createTimeDomainSubplots(sineFiles, Fs, savePlots);
%createFFTSubplots(sineFiles, Fs, savePlots);

%% === Processing and Plotting Function ===
function processSignals(fileList, label, Fs, savePlots)
    outDir = fullfile('outputPlots', label);
    if savePlots && ~exist(outDir, 'dir')
        mkdir(outDir);
    end

    for i = 1:length(fileList)
        try
            signal = readmatrix(fileList{i});
            signal = reshape(signal.', [], 1);  % Flatten block-style CSV to 1D column
            signal = signal(isfinite(signal));
        catch
            fprintf('Failed to read: %s\n', fileList{i});
            continue;
        end

        [~, name, ~] = fileparts(fileList{i});  % Filename only
        t = (0:length(signal)-1) / Fs;
        N = length(signal);


        %% === Time Domain Plot ===
        figure('Name', name + " - Time", 'NumberTitle', 'off');
        plot(t, signal);
        title(name);
        xlabel("Time [s]");
        ylabel("Amplitude");
        grid on;
        saveFigure(outDir, name + "_time", savePlots);

        %% === Sinusoid Zoom (First 5 ms) ===
        if contains(lower(label), 'sine') && ~contains(lower(label), 'shift')
            samplesToPlot = round(5e-3 * Fs);
            figure('Name', name + " - Zoom", 'NumberTitle', 'off');
            plot(t(1:samplesToPlot), signal(1:samplesToPlot));
            title(name + " (Zoom: First 5 ms)");
            xlabel("Time [s]");
            ylabel("Amplitude");
            grid on;
            saveFigure(outDir, name + "_zoom", savePlots);
elseif contains(lower(label), 'shift')
    % Apply the zoom for phase-shifted signals as well
    samplesToPlot = round(5e-3 * Fs);
    figure('Name', name + " - Zoom (Shifted)", 'NumberTitle', 'off');
    plot(t(1:samplesToPlot), signal(1:samplesToPlot));
    title(name + " (Zoom: First 5 ms, Phase Shifted)");
    xlabel("Time [s]");
    ylabel("Amplitude");
    grid on;
    saveFigure(outDir, name + "_zoom_shifted", savePlots);
        elseif contains(lower(label), 'sweep')
    desiredSamples = round(5 * Fs);  % Request first 5 seconds
    samplesToPlot = min(length(signal), desiredSamples);  % Don't exceed actual data

    figure('Name', name + " - Zoom (Sweep)", 'NumberTitle', 'off');
    plot(t(1:samplesToPlot), signal(1:samplesToPlot));
    title(name + " (Zoom: First 2.5 s, sine sweep)");
    xlabel("Time [s]");
    ylabel("Amplitude");
    xlim([0, 2.5]);  % Match x-axis to actual duration
    grid on;
    saveFigure(outDir, name + "_zoom_sweep", savePlots);

        end

        %% === FFT Plot ===
        Y = fft(signal);            
N = length(Y);              
f = (0:N-1) * (Fs / N);     
mag = abs(Y);               
mag = mag / max(mag);       % Normalize

figure('Name', name + " - FFT", 'NumberTitle', 'off');
plot(f(1:N/2), mag(1:N/2));
title(name + " (FFT)");
xlabel("Frequency [Hz]");
ylabel("Normalized Magnitude");
xlim([0 20000]);              % Tight frequency range for relevant content
xticks(0:1000:20000);
grid on;
saveFigure(outDir, name + "_fft", savePlots);

        %% === Spectrogram Plot ===
        figure('Name', name + " - Spectrogram", 'NumberTitle', 'off');
        spectrogram(signal, 256, 200, 1024, Fs, 'yaxis');
        ylim([0 1.3]);  % Max 1.3 kHz
        yticks(0:0.2:1.3);
        yticklabels(0:200:1300); % Convert to Hz
        title(name + " (Spectrogram)");
        ylabel("Frequency [Hz]");
        colorbar;
        colormap jet;
        saveFigure(outDir, name + "_spectrogram", savePlots);

        %% === Power Spectral Density Plot ===
        figure('Name', name + " - PSD", 'NumberTitle', 'off');
        [pxx, f_psd] = pwelch(signal, [], [], [], Fs);
        plot(f_psd, 10*log10(pxx));
        xlim([0 1300]);
        xticks(0:100:1300);
        title(name + " (Power Spectral Density)");
        xlabel("Frequency [Hz]");
        ylabel("Power/Frequency [dB/Hz]");
        grid on;
        saveFigure(outDir, name + "_psd", savePlots);

    end
end

%% === Save Figure Helper ===
function saveFigure(outDir, fileName, doSave)
    if doSave
        exportgraphics(gcf, fullfile(outDir, fileName + ".png"), 'Resolution', 300);
    end
end

%% === Create Time-Domain Subplots for All Sines (Zoomed In) ===
function createTimeDomainSubplots(sineFiles, Fs, savePlots)
    % Create a figure for time-domain subplots
    figure('Name', 'Sine Signals - Time Domain (Zoomed)', 'NumberTitle', 'off');
    numSines = length(sineFiles);
    rows = 4;  % Rows for subplot grid (4x5)
    cols = 5;  % Columns for subplot grid

    % Check if we need to adjust the number of rows/columns based on the signal count
    totalSubplots = rows * cols;
    if numSines > totalSubplots
        rows = ceil(numSines / cols); % Adjust number of rows if there are more than 20 signals
    end
    
    % Plot each sine signal in a subplot (zoomed version: first 5 ms)
    for i = 1:numSines
        signal = readmatrix(sineFiles{i});
        signal = reshape(signal.', [], 1);  % Flatten to 1D
        t = (0:length(signal)-1) / Fs;
        
        % Time duration for zooming (first 5 ms)
        samplesToPlot = round(5e-3 * Fs);  % First 5 ms
        tZoom = t(1:samplesToPlot);
        signalZoom = signal(1:samplesToPlot);
        
        % Create subplot for each sine signal
        subplot(rows, cols, i);
        plot(tZoom, signalZoom);
        title(['Sine ' num2str(i * 50-30) ' Hz']);
        xlabel('Time [s]');
        ylabel('Amplitude');
        grid on;
    end
    
    % Save the time-domain subplot figure
    saveFigure('outputPlots/TimeDomain', 'All_sine_time_subplots_zoomed', savePlots);
end

%% === Create FFT Subplots for All Sines ===
function createFFTSubplots(sineFiles, Fs, savePlots)
    % Create a figure for FFT subplots
    figure('Name', 'Sine Signals - FFT', 'NumberTitle', 'off');
    numSines = length(sineFiles);
    rows = 4;  % Rows for subplot grid (4x5)
    cols = 5;  % Columns for subplot grid
    
    % Check if we need to adjust the number of rows/columns based on the signal count
    totalSubplots = rows * cols;
    if numSines > totalSubplots
        rows = ceil(numSines / cols); % Adjust number of rows if there are more than 20 signals
    end
    
    % Plot the FFT of each sine signal in a subplot
    for i = 1:numSines
        signal = readmatrix(sineFiles{i});
        signal = reshape(signal.', [], 1);  % Flatten to 1D
        N = length(signal);
        f = (0:N-1) * (Fs / N);
        Y = fft(signal);
        mag = abs(Y) / max(abs(Y));  % Normalize magnitude
        
        % Create subplot for each FFT plot
        subplot(rows, cols, i);
        plot(f(1:N/2), mag(1:N/2));  % Plot only the positive frequencies
        title(['Sine ' num2str(i * 50-30) ' Hz - FFT']);
        xlabel('Frequency [Hz]');
        ylabel('Normalized Magnitude');
        xlim([0 1270]);  % Limit to relevant frequency range
        xticks([20:100:1270])
        grid on;
    end
    
    % Save the FFT subplot figure
    saveFigure('outputPlots/FFT', 'All_sine_fft_subplots', savePlots);
end