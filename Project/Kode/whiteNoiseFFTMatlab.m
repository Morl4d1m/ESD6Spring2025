% Parameters for the MLS signal
N = 1024;        % Length of the MLS signal (should be a power of 2)
M = 10;          % Order of the MLS (i.e., length of the shift register = 2^M - 1)
fs = 1000;       % Sampling frequency in Hz

% Generate the MLS signal
mls_signal = mls(N, M);

% Remove the DC offset by subtracting the mean
mls_signal = mls_signal - mean(mls_signal);

% Plot the MLS signal
figure;
subplot(2,1,1);
plot((0:N-1)/fs, mls_signal);
title('MLS Signal (DC Offset Removed)');
xlabel('Time (s)');
ylabel('Amplitude');

% Compute the FFT of the MLS signal
fft_signal = fft(mls_signal);
f = (0:N-1)*(fs/N);   % Frequency vector

% Plot the magnitude of the FFT
subplot(2,1,2);
plot(f, abs(fft_signal));
title('Magnitude of the FFT of MLS Signal (DC Offset Removed)');
xlabel('Frequency (Hz)');
ylabel('Magnitude');
xlim([0 fs/2]);       % Limit frequency to Nyquist frequency

% Function to generate MLS signal
function mls_signal = mls(N, M)
    % Generate a random initial seed for the shift register
    x = 2*round(rand(1, M)) - 1;  % Initial state: random 1's and -1's
    mls_signal = zeros(1, N);
    
    for n = 1:N
        % Output of the current shift register state (XOR of taps)
        mls_signal(n) = x(end);
        
        % Feedback function (XOR of the feedback taps)
        feedback = mod(sum(x([1, M])), 2);  % Feedback using taps 1 and M (based on the polynomial x^M + x + 1)
        
        % Shift the register
        x = [feedback, x(1:end-1)];
    end
end
