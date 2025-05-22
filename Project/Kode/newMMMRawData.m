% MMM calculation from raw impulse responses and sine sweep
clear; clc; close all;

%% USER PARAMETERS
input_csv = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\eggShellFoamTest1\whitenoiseEggShellFoamCH34.csv';
fs = 44100;              % Sample rate (Hz)
Nfft = 2^16;             % FFT length (zero-padding included)
ref_dB_threshold = -30;  % Truncate impulse response when it drops 30 dB below peak

% Tube geometry
c0 = 343;
rho0 = 1.21;
tube_width = 0.1;
tube_height = 0.1;
x1 = 70.71e-3;        % mic closest to sample [m]
x2 = 197.99e-3;       % second mic [m]
s = abs(x1 - x2);     % mic spacing [m]

%% LOAD CSV
data = readmatrix(input_csv);
mixer = data(:,1) - mean(data(:,1));
mic1  = data(:,2) - mean(data(:,2));
mic2  = data(:,3) - mean(data(:,3));

% Pad to Nfft
L = length(mixer);
if L < Nfft
    mixer(end+1:Nfft) = 0;
    mic1(end+1:Nfft)  = 0;
    mic2(end+1:Nfft)  = 0;
else
    mixer = mixer(1:Nfft);
    mic1  = mic1(1:Nfft);
    mic2  = mic2(1:Nfft);
end

%% STEP 1: Deconvolve to get impulse responses
MixerF = fft(mixer, Nfft);
Mic1F  = fft(mic1,  Nfft);
Mic2F  = fft(mic2,  Nfft);

eps_val = 1e-12; % prevent divide-by-zero
Hs1_imp = ifft(Mic1F ./ (MixerF + eps_val));
Hs2_imp = ifft(Mic2F ./ (MixerF + eps_val));

%% STEP 2: Auto-truncate Impulse Responses
[~, pk1] = max(abs(Hs1_imp));
[~, pk2] = max(abs(Hs2_imp));

% Find where energy falls below threshold
dB1 = 20*log10(abs(Hs1_imp)/max(abs(Hs1_imp)));
dB2 = 20*log10(abs(Hs2_imp)/max(abs(Hs2_imp)));
end1 = find(dB1(pk1:end) < ref_dB_threshold, 1, 'first') + pk1;
end2 = find(dB2(pk2:end) < ref_dB_threshold, 1, 'first') + pk2;

% Ensure bounds within signal
end1 = min(end1, length(Hs1_imp));
end2 = min(end2, length(Hs2_imp));

% Create Hann windows
len1 = end1 - pk1 + 1;
len2 = end2 - pk2 + 1;
win1 = hann(len1);
win2 = hann(len2);

Hs1_win = zeros(size(Hs1_imp));
Hs2_win = zeros(size(Hs2_imp));
Hs1_win(pk1:pk1+len1-1) = Hs1_imp(pk1:pk1+len1-1) .* win1;
Hs2_win(pk2:pk2+len2-1) = Hs2_imp(pk2:pk2+len2-1) .* win2;

%% STEP 3: FFT of windowed impulse responses
Hs1 = fft(Hs1_win, Nfft);
Hs2 = fft(Hs2_win, Nfft);
f = (0:Nfft-1)' * fs/Nfft;
keep = (f >= 20) & (f <= 1220);  % your measurement range

f = f(keep);
Hs1 = Hs1(keep);
Hs2 = Hs2(keep);

%% STEP 4: MMM Calculation
lambda0 = c0 ./ f;
k0_prime = 2 * pi ./ lambda0;
d = (4 * (tube_width * tube_height)) / (2 * (tube_width + tube_height));
k0_doubleprime = 1.94e-2 .* sqrt(f ./ c0) .* d;
k0 = k0_prime - 1i .* k0_doubleprime;

H12 = Hs2 ./ Hs1;
HI = exp(-1i .* k0 .* s);
HR = exp(1i .* k0 .* s);

% --- TOGGLE phase compensation ON or OFF ---
use_exp_factor = false; % <<<< TOGGLE THIS
if use_exp_factor
    exp_factor = exp(2 .* 1i .* k0 .* x1);
else
    exp_factor = 1;
end

r = ((H12 - HI) ./ (HR - H12)) .* exp_factor;
r_r = real(r);
r_i = imag(r);
abs_r = abs(r);
alpha = 1 - r_r.^2;

Z_ratio = (1 + r) ./ (1 - r);
Z = Z_ratio .* rho0 .* c0;
G = 1 ./ Z;

%% STEP 5: 1/3 Octave Averaging
third_oct_freqs = [100,125,160,200,250,315,400,500,630,800,1000,1220];
octave_data = [];

disp('--- MMM Results at 1/3 Octave Frequencies ---');
fprintf('%8s | %9s | %9s | %9s | %8s\n','FreqHz','Re(r)','Im(r)','|r|','Alpha');
fprintf('---------------------------------------------------------------\n');
for k = 1:length(third_oct_freqs)
    f_center = third_oct_freqs(k);
    f_low = f_center / 2^(1/6);
    f_high = f_center * 2^(1/6);
    idx = find(f >= f_low & f < f_high);
    if isempty(idx)
        continue;
    end
    r_r_oct = mean(r_r(idx));
    r_i_oct = mean(r_i(idx));
    abs_r_oct = mean(abs_r(idx));
    alpha_oct = mean(alpha(idx));
    Z_real_oct = mean(real(Z(idx)));
    Z_imag_oct = mean(imag(Z(idx)));
    G_real_oct = mean(real(G(idx)));
    G_imag_oct = mean(imag(G(idx)));

    fprintf('%8.1f | %9.4f | %9.4f | %9.4f | %8.4f\n', ...
        f_center, r_r_oct, r_i_oct, abs_r_oct, alpha_oct);

    octave_data = [octave_data; f_center, r_r_oct, r_i_oct, abs_r_oct, alpha_oct, ...
        Z_real_oct, Z_imag_oct, G_real_oct, G_imag_oct];
end

% Save results
output_table = array2table(octave_data, ...
    'VariableNames', {'FreqHz','ReflectionReal','ReflectionImag','ReflectionAbs',...
    'AbsorptionCoeff','ImpedanceReal_Pa_s_per_m','ImpedanceImag_Pa_s_per_m',...
    'AdmittanceReal_m_per_Pa_s','AdmittanceImag_m_per_Pa_s'});
output_csv = 'MMM_results_output_ThirdOctave_deconvolved.csv';
writetable(output_table, output_csv);
disp(['Results saved in ', output_csv]);

% Plot
figure;
semilogx(octave_data(:,1), octave_data(:,5), '-o','LineWidth',2);
xlabel('Frequency [Hz]');
ylabel('Absorption Coefficient');
title('Absorption Coefficient at 1/3 Octave Bands (MMM, Deconvolved)');
grid on;
xlim([80 1300]); ylim([0 1]);
