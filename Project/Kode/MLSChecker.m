check_mls_from_file('C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\MLSGeneratedByTeensy\24BitMLS.txt',44100,'N');

function check_mls_from_file(filename, Fs, save_outputs)
    % CHECK_MLS_FROM_FILE(filename, Fs, save_outputs)
    % Verifies if a binary sequence from a .txt file is a valid MLS.
    % Fs = sampling frequency in Hz (for FFT plot in Hz).
    % save_outputs = 'Y' or 'N' (save plots and CSV or not).
    %
    % Example: check_mls_from_file('mymls.txt', 48000, 'Y');

    if nargin < 3
        error('You must specify filename, Fs, and save_outputs (Y or N).');
    end

    if ~(save_outputs == 'Y' || save_outputs == 'N')
        error('save_outputs must be ''Y'' or ''N''.');
    end

    % --- Read Sequence ---
    try
        fid = fopen(filename, 'r');
        raw_line = fgetl(fid);
        fclose(fid);
    catch
        error('Could not open or read file.');
    end

    if ~ischar(raw_line) && ~isstring(raw_line)
        error('File must contain a single line of characters.');
    end

    seq = raw_line(:)'; % Ensure row vector
    if any(seq ~= '0' & seq ~= '1')
        error('File must contain only 0 and 1 characters.');
    end
    seq = seq - '0'; % Convert '0'/'1' -> numeric 0/1

    % --- Basic Checks ---
    L = length(seq);
    n = log2(L + 1);

    if abs(round(n) - n) > eps
        error('Length must be 2^n - 1 for some integer n between 2 and 32.');
    end

    n = round(n);

    if n < 2 || n > 32
        error('n must be between 2 and 32.');
    end

    fprintf('Sequence length: %d samples\n', L);
    fprintf('Detected register size: %d bits\n', n);

    % --- Map to {-1, +1} domain ---
    seq_mapped = 2*seq - 1;

    % --- Check all cyclic shifts ---
    cyclic_pass = false;
    shift_amount = 0;
    for k = 0:L-1
        shifted_seq = circshift(seq_mapped, k);
        acorr = ifft(abs(fft(shifted_seq)).^2);
        acorr = real(acorr);

        tolerance = 1e-6;
        peak_ok = abs(acorr(1) - L) < tolerance;
        sidelobes_ok = all(abs(acorr(2:end) + 1) < tolerance);

        if peak_ok && sidelobes_ok
            cyclic_pass = true;
            shift_amount = k;
            break;
        end
    end

    % --- Report results ---
    if cyclic_pass
        disp('✅ Sequence is a VALID MLS (after cyclic shift detection).');
        fprintf('Detected cyclic shift of %d samples.\n', shift_amount);
    else
        disp('❌ Sequence FAILED MLS check even after trying all cyclic shifts.');
    end

    % --- Prepare Summary Values ---
    peak_value = acorr(1);
    max_sidelobe_deviation = max(abs(acorr(2:end) + 1));

    % --- Print Summary Table ---
    fprintf('\nSummary:\n');
    fprintf('----------------------------------------------------\n');
    fprintf('Length                : %d samples\n', L);
    fprintf('Register size (n)      : %d bits\n', n);
    fprintf('Detected cyclic shift : %d samples\n', shift_amount);
    fprintf('Autocorrelation peak  : %.6f (ideal = %d)\n', peak_value, L);
    fprintf('Max sidelobe deviation: %.6f\n', max_sidelobe_deviation);
    fprintf('----------------------------------------------------\n\n');

    % --- Prepare for Saving ---
    [filepath, name_only, ~] = fileparts(filename);

    if save_outputs == 'Y'
        % --- Export summary to CSV ---
        summary_table = table(...
            string(filename), L, n, shift_amount, peak_value, max_sidelobe_deviation, ...
            'VariableNames', {'Filename','Length','RegisterSize','CyclicShift','PeakValue','MaxSidelobeDeviation'} ...
        );
        csv_filename = fullfile(filepath, sprintf('%s_summary.csv', name_only));
        writetable(summary_table, csv_filename);
        fprintf('Summary exported to CSV file: %s\n', csv_filename);
    else
        fprintf('CSV not saved (save_outputs = ''N'').\n');
    end

    % --- Plotting ---
    fig = figure('Name','MLS Analysis','Position',[100 100 1400 800]);

    % 1. MLS Sequence
    subplot(2,2,1);
    stem(seq, 'Marker','none');
    title('MLS Sequence (Time Domain)');
    xlabel('Sample');
    ylabel('Amplitude');
    grid on;
    xlim([0 L]);

    % 2. Autocorrelation
    subplot(2,2,2);
    lags = 0:L-1;
    stem(lags, acorr, 'Marker','none');
    title('Autocorrelation');
    xlabel('Lag');
    ylabel('Autocorr');
    grid on;
    xlim([0 L-1]);

    % 3. FFT Magnitude Spectrum
    subplot(2,2,3);
    Nfft = 2^nextpow2(L);
    f_Hz = (0:Nfft-1)*(Fs/Nfft); % Frequency axis in Hz
    spectrum = abs(fft(seq_mapped, Nfft));
    plot(f_Hz(1:Nfft/2), spectrum(1:Nfft/2));
    title('Magnitude Spectrum');
    xlabel('Frequency (Hz)');
    ylabel('Magnitude');
    grid on;
    xlim([0 Fs/2]);
    set(gca, 'XTickMode', 'auto', 'XMinorTick', 'on');
    ax = gca;
    ax.XAxis.Exponent = 0; % Disable scientific notation

    % 4. Power Spectral Density
    subplot(2,2,4);
    [Pxx,F] = pwelch(seq_mapped,[],[],[],Fs,'twosided');
    plot(F,10*log10(Pxx));
    title('Power Spectral Density (PSD)');
    xlabel('Frequency (Hz)');
    ylabel('Power/Frequency (dB/Hz)');
    grid on;
    xlim([0 Fs/2]);
    set(gca, 'XTickMode', 'auto', 'XMinorTick', 'on');
    ax = gca;
    ax.XAxis.Exponent = 0; % Disable scientific notation

    % --- Save plots if requested ---
    if save_outputs == 'Y'
        png_filename = fullfile(filepath, sprintf('%s_plot.png', name_only));
        pdf_filename = fullfile(filepath, sprintf('%s_plot.pdf', name_only));
        saveas(fig, png_filename);
        saveas(fig, pdf_filename);
        fprintf('Plots saved as: %s and %s\n\n', png_filename, pdf_filename);
    else
        fprintf('Plots not saved (save_outputs = ''N'').\n\n');
    end
end
