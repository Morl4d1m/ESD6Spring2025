% Parameters
N = 16;            % Length of MLS (number of bits, this will produce a sequence of 2^N-1)
fs = 44100;        % Sampling frequency (samples per second)


file_path= 'C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Project\Kode\16bitMLS.txt'
fileID = fopen(file_path, 'r');
% Check if the file opened successfully
if fileID == -1
    error('Failed to open the file.');
end

% Read the file contents (assumed to be a sequence of '1' and '0' characters)
binaryString = fread(fileID, '*char')';  % Read the entire file as characters

% Close the file
fclose(fileID);

% Remove any unwanted characters (like spaces or newlines)
binaryString = strrep(binaryString, newline, '');  % Remove newlines
binaryString = strrep(binaryString, ' ', '');      % Remove spaces

% Convert the binary string to a numeric array (0 and 1)
binaryArray = binaryString == '1';  % Logical array (1 for '1', 0 for '0')

% Convert binary array to a float array suitable for sound scaled to -1 to
% 1
audioData = 2 * (binaryArray-0.5);

% Generate the MLS sequence (using MATLAB's built-in function)
mls = mls_sequence(N);

% Normalize to -1 to 1 for audio playback
mls_audio = 2 * (mls - 0.5);  % Convert 0/1 to -1/1 for audio

% Play the MLS signal
sound(mls, fs); %made by matlab
pause(2);
sound(audioData, fs); % made by Teensy

% Function to generate the MLS sequence (using feedback shift register)
function mls = mls_sequence(N)
    % N is the order of the sequence (number of bits)
    maxLength = 2^N - 1;
    mls = zeros(1, maxLength);
    register = ones(1, N);  % Initialize with all 1s
    taps = [16, 15, 13, 4];        % Feedback taps for the XOR operation (primitive polynomial)
    
    for i = 1:maxLength
        mls(i) = register(end);  % The output is the last bit of the register
        
        % Feedback and shift
        feedback = mod(sum(register(taps)), 2);  % XOR the selected bits
        register = [feedback register(1:end-1)];  % Shift the register
    end
end






