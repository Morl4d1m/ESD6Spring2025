% Room dimensions (meters)
Lx = 4.7;
Ly = 4.1;
Lz = 3.1;

% Speed of sound (m/s)
c = 343.0;

% Max frequency (Hz)
f_max = 20000.0;

% Threshold for valid modes
threshold = (2 * f_max / c)^2;

% Storage for unique frequencies
uniqueFrequencies = [];
modes = [];

validModes = 0;

% Iterate through possible mode values
for nx = 0:ceil(Lx * sqrt(threshold))
    for ny = 0:ceil(Ly * sqrt(threshold))
        for nz = 0:ceil(Lz * sqrt(threshold))
            
            % Compute modal ratio
            ratio = (nx / Lx)^2 + (ny / Ly)^2 + (nz / Lz)^2;
            
            if ratio <= threshold
                validModes = validModes + 1;
                
                % Compute eigenfrequency
                freq = (c / 2) * sqrt(ratio);
                
                % Check uniqueness (tolerance of 0.1 Hz)
                if isempty(uniqueFrequencies) || all(abs(uniqueFrequencies - freq) > 0.1)
                    uniqueFrequencies(end + 1) = freq; %#ok<SAGROW>
                    modes = [modes; nx, ny, nz]; %#ok<AGROW>
                end
            end
        end
    end
end

% Sort eigenfrequencies and modes
[uniqueFrequencies, sortIdx] = sort(uniqueFrequencies);
modes = modes(sortIdx, :);

% Display results
disp('List of unique eigenfrequencies and their modes:');
disp('--------------------------------------------------');
for i = 1:length(uniqueFrequencies)
    fprintf('f = %.2f Hz  (nx=%d, ny=%d, nz=%d)\n', uniqueFrequencies(i), modes(i,1), modes(i,2), modes(i,3));
end
disp('--------------------------------------------------');
fprintf('Total number of valid modes: %d\n', validModes);
fprintf('Total number of unique eigenfrequencies: %d\n', length(uniqueFrequencies));
