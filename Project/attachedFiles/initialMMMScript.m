% Multi Microphone Method (MMM) Calculation Script (1/3 Octave Output)
% DS/ISO 10534-2:2023 based
clear; clc; close all;

%% User Inputs
input_csv = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\eggShellFoamTest2\sine1250EggShellFoamCH12IRAndFFT.csv'; 

% Physical parameters
c0 = 343;            % Speed of sound [m/s]
rho0 = 1.21;         % Air density [kg/m^3]
tube_width = 0.1;    % Width of tube [m]
tube_height = 0.1;   % Height of tube [m]

% Microphone positions
x1 = 70.71e-3;       % Closest mic to sample [m]
x2 = 197.99e-3;      % Second mic position [m]
s = abs(x1 - x2);    % Distance between microphones [m]

%% Load CSV Data
data = readtable(input_csv);

freq = data.FreqHz;
Hs1 = data.h1Impulse;
Hs2 = data.h2Impulse;

H1_complex = data.H1real + 1i*data.H1imag;
H2_complex = data.H2real + 1i*data.H2imag;

%% Calculate wave number k0
lambda0 = c0 ./ freq;
k0_prime = 2 * pi ./ lambda0;
d = (4 * (tube_width * tube_height)) / (2 * (tube_width + tube_height));
k0_doubleprime = 1.94e-2 .* sqrt(freq./c0) .* d;
k0 = k0_prime - 1i .* k0_doubleprime;

%% Calculate Transfer functions
H12_uncorrected = Hs2 ./ Hs1;
Hc = sqrt(H1_complex .* H2_complex);
H12 = H12_uncorrected;% ./ Hc;

HI = exp(-1i .* k0 .* s);
HR = exp(1i .* k0 .* s);

%% Reflection Coefficient
exp_factor = exp(2 .* 1i .* k0 .* x1);
r = ((H12 - HI) ./ (HR - H12)) .* exp_factor;

r_r = real(r);
r_i = imag(r);
abs_r = abs(r);

%% Absorption Coefficient
alpha = 1 - abs_r.^2;

%% Impedance and Admittance
Z_ratio = (1 + r) ./ (1 - r);
Z = Z_ratio .* (rho0 .* c0);
G_ratio = 1 ./ Z_ratio;
G = G_ratio ./ (rho0 .* c0);

%% Define standard 1/3 octave center frequencies
third_oct_freqs = [100,125,160,200,250,315,400,500,630,800,1000,...
    1250,1600,2000,2500,3150,4000,5000,6300,8000];

% Find indices closest to 1/3 octave frequencies
[~, idx_third_oct] = arrayfun(@(x) min(abs(freq - x)), third_oct_freqs);

%% Extract values at 1/3 octave frequencies
freq_oct = freq(idx_third_oct);
r_r_oct = r_r(idx_third_oct);
r_i_oct = r_i(idx_third_oct);
abs_r_oct = abs_r(idx_third_oct);
alpha_oct = alpha(idx_third_oct);
Z_real_oct = real(Z(idx_third_oct));
Z_imag_oct = imag(Z(idx_third_oct));
G_real_oct = real(G(idx_third_oct));
G_imag_oct = imag(G(idx_third_oct));

%% Output Results in Command Window
disp('--- MMM Results at 1/3 Octave Frequencies ---');
fprintf('%8s | %9s | %9s | %9s | %8s\n','FreqHz','Re(r)','Im(r)','|r|','Alpha');
fprintf('---------------------------------------------------------------\n');
for k = 1:length(freq_oct)
    fprintf('%8.1f | %9.4f | %9.4f | %9.4f | %8.4f\n',...
        freq_oct(k), r_r_oct(k), r_i_oct(k), abs_r_oct(k), alpha_oct(k));
end

%% Save Results to CSV
output_table = table(freq_oct, r_r_oct, r_i_oct, abs_r_oct, alpha_oct,...
    Z_real_oct, Z_imag_oct, G_real_oct, G_imag_oct,...
    'VariableNames', {'FreqHz','ReflectionReal','ReflectionImag','ReflectionAbs',...
    'AbsorptionCoeff','ImpedanceReal_Pa_s_per_m','ImpedanceImag_Pa_s_per_m',...
    'AdmittanceReal_m_per_Pa_s','AdmittanceImag_m_per_Pa_s'});

output_csv = 'MMM_results_output_ThirdOctave.csv';
writetable(output_table, output_csv);

disp(['Results saved in ', output_csv]);

%% Validation at ~1 kHz
[~, idx_1kHz] = min(abs(freq_oct - 1000));
fprintf('\n--- Validation at ~1 kHz ---\n');
fprintf('Freq = %.1f Hz\n', freq_oct(idx_1kHz));
fprintf('Reflection Coefficient: %.4f + %.4fi\n', r_r_oct(idx_1kHz), r_i_oct(idx_1kHz));
fprintf('Reflection Magnitude: %.4f\n', abs_r_oct(idx_1kHz));
fprintf('Absorption Coefficient: %.4f\n', alpha_oct(idx_1kHz));
fprintf('Expected Absorption (datasheet): ~0.48\n');

%% Plot Absorption Coefficient
figure;
semilogx(freq_oct, alpha_oct, '-o', 'LineWidth', 2);
xlabel('Frequency [Hz]');
ylabel('Absorption Coefficient');
title('Absorption Coefficient at 1/3 Octave Frequencies');
grid on;
xlim([min(freq_oct)*0.9, max(freq_oct)*1.1]);
ylim([0 1]);
xticks(freq_oct);
xtickangle(45);

%plot(freq, abs(H12), 'b', freq, abs(HI), 'g--', freq, abs(HR), 'r--');
%legend('H_{12}','H_I','H_R'); title('|Transfer Functions|');