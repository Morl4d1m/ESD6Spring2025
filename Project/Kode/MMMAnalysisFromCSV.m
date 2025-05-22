% Multi‑Microphone Method analysis from CSV input
% CSV columns: FreqHz,Xreal,Ximag,Y1real,Y1imag,...
%              H1real,H1imag,H1magdB,H1phaserad,hs1,...
%              Y2real,Y2imag,H2real,H2imag,H2magdB,H2phaserad,hs2

% USER INPUTS: -------------------------------------------------------------
csvFile = 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\acceptanceTest\eggShellFoamTest1\IRAndFFT\sweepEggShellFoamCH56IRAndFFT';  % your CSV file

fs      = 44100;      % Hz
c0      = 343;        % m/s
rho0    = 1.21;       % kg/m3

% Experimental geometry: Mic 1 = closest to sample, Mic 2 = further from sample
x1 = 0.07071;   % [m] mic 1 (closest to sample, reference)
x2 = 0.19722;   % [m] mic 2 (further from sample, source side)
s  = x2 - x1;   % s = mic2 - mic1, should be positive

% Read raw time data
raw = readmatrix(csvFile);
N = size(raw,1);
mixer = raw(:,1);
mic1  = raw(:,2);  % P1 (close to sample)
mic2  = raw(:,3);  % P2 (farther from sample)

% FFT
Nfft = 2^nextpow2(N);
f = (0:Nfft/2)' * fs/Nfft;

% FFTs
P1 = fft(mic1,Nfft);   % Reference (close to sample)
P2 = fft(mic2,Nfft);   % Further from sample
% Only use positive freqs
P1 = P1(1:Nfft/2+1);
P2 = P2(1:Nfft/2+1);

% Remove DC and normalize for numerical stability
P1(1) = 1e-10; % avoid division by zero

% MMM Transfer function (ISO): T = P2/P1
H12 = P2 ./ P1;

% Wavenumber (NO loss for now, for debugging)
k0 = 2*pi*f/c0;

% MMM equations:
% Reference plane at sample front (x=0), mic1 at x1, mic2 at x2
% Incident: HI = exp(-1j*k0*s), Reflected: HR = exp(1j*k0*s)
HI = exp(-1j*k0*s);
HR = exp( 1j*k0*s);

% Reflection coefficient at the sample
r = ((H12 - HI)./(HR - H12)) .* exp(2j*k0*x1);
rmag = abs(r);

% Absorption
alpha = 1 - abs(r).^2;

% 1/3 octave bands (center freqs)
f_center = [12.5,16,20,25,31.5,40,50,63,80,100,125,160,200,250,315,400,500,630,800,1000,1250,1600];
alpha_band = zeros(size(f_center));
r_band = zeros(size(f_center));
for i = 1:length(f_center)
    fl = f_center(i)/2^(1/6);
    fu = f_center(i)*2^(1/6);
    idx = (f >= fl) & (f < fu) & (f > 10) & (f < 1700); % limit to good range
    if nnz(idx)
        alpha_band(i) = mean(alpha(idx),'omitnan');
        r_band(i) = mean(rmag(idx),'omitnan');
    else
        alpha_band(i) = NaN;
        r_band(i) = NaN;
    end
end

% Save results for easy comparison
results_table = table(f_center', alpha_band', r_band', ...
    'VariableNames',{'Freq_Hz','Alpha_avg','Reflection_avg'});
writetable(results_table,'MMM_1_3oct_Results.csv');

% Plot for check
figure;
semilogx(f,alpha,'-',f_center,alpha_band,'ro-','LineWidth',1.5);
xlabel('Frequency [Hz]'); ylabel('Absorption coefficient \alpha');
grid on; legend('Raw \alpha','1/3-octave mean \alpha');
title('MMM Absorption coefficient (corrected)');

fprintf('Results saved to MMM_1_3oct_Results.csv\n');