% MMM Batch averaging script for 1/3 Octave Output with plots and folder selection
clear; clc; close all;

% ----------- USER INPUT -----------
data_folder = 'E:\';  % <--- Change to your folder name
pattern = fullfile(data_folder, '*SAMPLENAMECH12IRAndFFT.csv');
CLAMP_VALUES = false;      % Clamp |r|, Re(r), Im(r) to [-1, 1]
ALPHA_NONNEG = true;      % Make alpha always >= 0 (set negative values to 0)
% ----------------------------------

% Physical parameters
c0 = 343;
rho0 = 1.21;
tube_width = 0.1;
tube_height = 0.1;
x1 = 70.71e-3;
x2 = 197.99e-3;
s = abs(x1 - x2);

third_oct_freqs = [10,12.5,16,20,25,31.5,40,50,63,80,100,125,160,200,250,315,400,500,630,800,1000,1250,1600];

files = dir(pattern);
nFiles = numel(files);

if nFiles == 0
    error('No matching files found in folder "%s"', data_folder);
end

% Preallocate
r_r_all    = zeros(nFiles, numel(third_oct_freqs));
r_i_all    = zeros(nFiles, numel(third_oct_freqs));
abs_r_all  = zeros(nFiles, numel(third_oct_freqs));
alpha_all  = zeros(nFiles, numel(third_oct_freqs));
freq_oct   = nan(1, numel(third_oct_freqs)); % Will be the same for all files

for fidx = 1:nFiles
    data = readtable(fullfile(files(fidx).folder, files(fidx).name));
    freq = data.FreqHz;
    Hs1 = data.h1Impulse;
    Hs2 = data.h2Impulse;
    H1_complex = data.H1real + 1i*data.H1imag;
    H2_complex = data.H2real + 1i*data.H2imag;

    % Calculate k0
    lambda0 = c0 ./ freq;
    k0_prime = 2 * pi ./ lambda0;
    d = (4 * (tube_width * tube_height)) / (2 * (tube_width + tube_height));
    k0_doubleprime = 1.94e-2 .* sqrt(freq./c0) .* d;
    k0 = k0_prime - 1i .* k0_doubleprime;

    % MMM calcs
    H12_uncorrected = Hs2 ./ Hs1;
    Hc = sqrt(H1_complex .* H2_complex);
    H12 = H12_uncorrected ./ Hc;

    HI = exp(-1i .* k0 .* s);
    HR = exp(1i .* k0 .* s);

    exp_factor = exp(2 .* 1i .* k0 .* x1);
    r = ((H12 - HI) ./ (HR - H12)) .* exp_factor;
    r_r = real(r);
    r_i = imag(r);
    abs_r = abs(r);
    alpha = 1 - abs_r.^2;

    % 1/3 octave selection
    [~, idx_third_oct] = arrayfun(@(x) min(abs(freq - x)), third_oct_freqs);
    freq_oct = freq(idx_third_oct); % only needs to be set once

    % --- Clamping and alpha correction based on user parameter ---
    if CLAMP_VALUES
        r_r_oct = max(-1, min(1, r_r(idx_third_oct)));
        r_i_oct = max(-1, min(1, r_i(idx_third_oct)));
        abs_r_oct = min(1, abs_r(idx_third_oct));
    else
        r_r_oct = r_r(idx_third_oct);
        r_i_oct = r_i(idx_third_oct);
        abs_r_oct = abs_r(idx_third_oct);
    end

    if ALPHA_NONNEG
        alpha_oct = min(1, abs(alpha(idx_third_oct))); % Use absolute value and clamp to 1
    else
        alpha_oct = alpha(idx_third_oct);
    end

    % Store for averaging
    r_r_all(fidx,:)   = r_r_oct;
    r_i_all(fidx,:)   = r_i_oct;
    abs_r_all(fidx,:) = abs_r_oct;
    alpha_all(fidx,:) = alpha_oct;
end

% Extra safety: clamp again before averaging
if CLAMP_VALUES
    r_r_all    = max(-1, min(1, r_r_all));
    r_i_all    = max(-1, min(1, r_i_all));
    abs_r_all  = min(1, abs_r_all);
end
if ALPHA_NONNEG
    alpha_all  = min(1, max(0, alpha_all));
end

% Compute averages and std devs
r_r_avg   = mean(r_r_all, 1);
r_i_avg   = mean(r_i_all, 1);
abs_r_avg = mean(abs_r_all, 1);
alpha_avg = mean(alpha_all, 1);

r_r_std   = std(r_r_all, 0, 1);
r_i_std   = std(r_i_all, 0, 1);
abs_r_std = std(abs_r_all, 0, 1);
alpha_std = std(alpha_all, 0, 1);

% Display averages (as your script does)
disp('--- MMM Averaged Results at 1/3 Octave Frequencies ---');
fprintf('%8s | %9s | %9s | %9s | %8s\n','FreqHz','Re(r)','Im(r)','|r|','Alpha');
fprintf('---------------------------------------------------------------\n');
for k = 1:length(freq_oct)
    fprintf('%8.1f | %9.4f | %9.4f | %9.4f | %8.4f\n',...
        freq_oct(k), r_r_avg(k), r_i_avg(k), abs_r_avg(k), alpha_avg(k));
end

%% Plot average Alpha as a curve
figure;
semilogx(freq_oct, alpha_avg, '-o', 'LineWidth', 2, 'DisplayName','Mean \alpha');
hold on;
errorbar(freq_oct, alpha_avg, alpha_std, 'k.', 'LineWidth', 1, 'DisplayName','Std Dev');
xlabel('Frequency [Hz]');
ylabel('Absorption Coefficient \alpha');
title('Average Absorption Coefficient (\alpha) at 1/3 Octave Frequencies');
grid on;
xlim([min(freq_oct)*0.9, max(freq_oct)*1.1]);
ylim([-0.05 1.05]);
xticks(freq_oct);
xtickangle(45);
legend;
hold off;

%% Plot std deviations for all quantities
figure;
errorbar(freq_oct, r_r_avg, r_r_std, '-o', 'LineWidth', 1.5, 'DisplayName','Re(r)');
hold on;
errorbar(freq_oct, r_i_avg, r_i_std, '-o', 'LineWidth', 1.5, 'DisplayName','Im(r)');
errorbar(freq_oct, abs_r_avg, abs_r_std, '-o', 'LineWidth', 1.5, 'DisplayName','|r|');
errorbar(freq_oct, alpha_avg, alpha_std, '-o', 'LineWidth', 1.5, 'DisplayName','Alpha');
xlabel('Frequency [Hz]');
ylabel('Mean value with Std Dev');
title('MMM: Average and Std Dev at 1/3 Octave Frequencies');
legend;
grid on;
set(gca, 'XScale', 'log');
xticks(freq_oct);
xtickangle(45);
hold off;