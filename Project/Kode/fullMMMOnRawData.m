clear; clc; close all;

csvFile = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\eggShellFoamTest1\sweepEggShellFoamCH12'; % Change to your local path if needed!
%filename = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\emptyTube\sweepEmptyTubeCH12.csv';
fs = 44100;              % Sampling rate [Hz]
c0 = 343;                % Speed of sound [m/s]
rho0 = 1.21;             % Air density [kg/m^3]
x1 = 0.07071;            % Mic 1 from sample [m]
x2 = 0.19722;            % Mic 2 from sample [m]
% Square tube - hydraulic diameter [m] (100x100mm)
tube_d = (4*(0.1*0.1))/(2*0.1+2*0.1);

% 1/3-octave bands for output
f_center = [12.5,16,20,25,31.5,40,50,63,80,100,125,160,200,250,315,400,500,630,800,1000,1250,1600];

% --- Load Data ---
T = readmatrix(csvFile);
mixer = T(:,1);
mic1  = T(:,2);
mic2  = T(:,3);

N = length(mixer);
t = (0:N-1)/fs;

figure;
plot(t, mixer, 'k', t, mic1, 'b', t, mic2, 'r');
xlabel('Time [s]'); ylabel('Amplitude');
legend('Mixer','Mic1','Mic2'); title('Raw time signals');
set(gcf, 'Position', [200, 200, 1600, 600]);

% --- A. BANDPASS FILTERING (remove out-of-band noise) ---
f_low = 20; f_high = 1220;
[b, a] = butter(4, [f_low, f_high]/(fs/2), 'bandpass');
mixer_f = filtfilt(b, a, mixer);
mic1_f  = filtfilt(b, a, mic1);
mic2_f  = filtfilt(b, a, mic2);

% --- B. AUTOMATIC TIME-DOMAIN GATING (detect sweep region) ---
% Calculate moving RMS energy of mixer to locate sweep
winlen = round(0.05*fs); % 50 ms window
env = sqrt(movmean(mixer_f.^2, winlen));
[~, idx_max] = max(env);   % Find sweep peak (should be mid-sweep)
thresh = max(env) * 0.15;  % 15% of max energy (tune if needed)
valid_idx = find(env > thresh);
i1 = max(1, valid_idx(1) - round(0.05*fs)); % Pad 50 ms before/after
i2 = min(N, valid_idx(end) + round(0.05*fs));
fprintf('Using window: %.3f - %.3f s (samples %d-%d)\n', i1/fs, i2/fs, i1, i2);

% --- Apply time window ---
mixer_win = mixer_f(i1:i2);
mic1_win  = mic1_f(i1:i2);
mic2_win  = mic2_f(i1:i2);

% --- OPTIONAL: Amplify signals for processing (uncomment to use) ---
amp_factor = 100; % e.g. 2 or 3
 mixer_win = mixer_win * amp_factor;
 mic1_win  = mic1_win  * amp_factor;
 mic2_win  = mic2_win  * amp_factor;

% --- Show windowed signals ---
figure;
plot((0:length(mixer_win)-1)/fs, mixer_win, 'k', ...
     (0:length(mic1_win)-1)/fs, mic1_win, 'b', ...
     (0:length(mic2_win)-1)/fs, mic2_win, 'r');
xlabel('Time [s]'); ylabel('Amplitude');
legend('Mixer','Mic1','Mic2'); title('Time-gated, bandpass filtered');

% --- C. SPECTRAL AVERAGING (Welch method for SNR) ---
nfft = 2^nextpow2(length(mixer_win));
win  = hanning(round(length(mixer_win)/4));
noverlap = round(length(win)/2);

% Use FFT of whole window for transfer function (sweep is single event)
X = fft(mixer_win, nfft);
Y1 = fft(mic1_win, nfft);
Y2 = fft(mic2_win, nfft);

f_axis = fs*(0:(nfft/2))/nfft;
H12 = Y2(1:nfft/2+1) ./ (Y1(1:nfft/2+1) + 1e-12);

% --- D. MMM TRANSFER FUNCTION AND REFLECTION COEFFICIENT ---
f = f_axis(:);

k0 = 2*pi*f/c0;
s = abs(x2 - x1);
HI = exp(-1j*k0*s);
HR = exp( 1j*k0*s);

r = (H12 - HI) ./ (HR - H12) .* exp(2j*k0*x1);
alpha = 1 - abs(r).^2;

% --- E. 1/3-OCTAVE BAND AVERAGING ---
alpha_band = nan(size(f_center));
r_band = nan(size(f_center));
for i=1:length(f_center)
    fl = f_center(i)/2^(1/6);
    fu = f_center(i)*2^(1/6);
    idx = find(f>=fl & f<=fu);
    alpha_band(i) = mean(alpha(idx),'omitnan');
    r_band(i) = mean(abs(r(idx)),'omitnan');
end

% --- F. Output CSV ---
results_table = table(f_center(:), alpha_band(:), r_band(:), ...
    'VariableNames',{'Freq_Hz','Absorption_1_3oct','Reflection_1_3oct'});
writetable(results_table, 'MMM_1_3oct_Results.csv');

% --- G. Plot Results ---
figure;
semilogx(f,alpha,'-', f_center,alpha_band,'o-','LineWidth',1.5);
xlabel('Frequency [Hz]'); ylabel('Absorption \alpha');
grid on; legend('Raw \alpha','1/3-oct avg \alpha');
title('MMM Absorption Coefficient (SNR boosted)');

figure;
semilogx(f,abs(r),'-', f_center,r_band,'o-','LineWidth',1.5);
xlabel('Frequency [Hz]'); ylabel('Reflection coefficient |r|');
grid on; legend('Raw |r|','1/3-oct avg |r|');
title('MMM Reflection Coefficient (SNR boosted)');

fprintf('Results saved to MMM_1_3oct_Results.csv\n');