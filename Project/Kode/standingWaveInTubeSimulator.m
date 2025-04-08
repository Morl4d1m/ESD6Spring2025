clear;clf;

%% User Inputs
f = 1000;         % Frequency in Hz
L = 1.0;        % Tube length in meters
D = 0.10;       % Tube diameter in meters
R = 1;          % Reflection coefficient (1 = perfect reflection)
A_dB = 60;      % Amplitude in dB

%% Constants
c = 343; % Speed of sound in air (m/s)
A = A_dB;%10^(A_dB / 20); % Convert dB to linear scale
lambda = c / f; % Wavelength
k = 2 * pi / lambda; % Wave number
t = linspace(0, 1/f, 50); % Time vector (one period)
x = linspace(0, L, 500); % Spatial vector

%% Wave Components
omega = 2 * pi * f; % Angular frequency

[X, T] = meshgrid(x, t); % Create mesh for plotting

incident_wave = A * cos(k * X - omega * T); % Incident wave
reflected_wave = R * A * cos(-k * X - omega * T + pi); % Reflected wave (180° phase shift)
standing_wave = incident_wave + reflected_wave; % Superposition

%% Plot Standing Wave Over Time
figure(1);
surf(X, T, standing_wave, 'EdgeColor', 'none');
xlabel('Position along Tube (m)');
ylabel('Time (s)');
zlabel('Amplitude in dB');
title('Standing Wave in Tube');
colormap jet;
colorbar;
view(3);

%% Plot Standing Wave Profile at a Fixed Time
figure(2);
plot(x, standing_wave(round(end/2), :), 'k', 'LineWidth', 2);
hold on;
plot(x, incident_wave(round(end/2), :), '--b', 'LineWidth', 1);
plot(x, reflected_wave(round(end/2), :), '--r', 'LineWidth', 1);
hold off;
xlabel('Position along Tube (m)');
ylabel('Amplitude in dB');
title('Standing Wave Profile');
legend('Total Wave', 'Incident Wave', 'Reflected Wave');
grid on;

%% Multiple Harmonic Modes
figure(3);
modes = [1, 2, 3, 4]; % Different harmonic modes

for i = 1:length(modes)
    n = modes(i);
    fn = (2*n - 1) * c / (4 * L); % Odd harmonics for closed-open tube
    lambda_n = c / fn;
    k_n = 2 * pi / lambda_n;
    
    wave_n = A * cos(k_n * x); % Standing wave mode
    subplot(2, 2, i);
    plot(x, wave_n, 'k', 'LineWidth', 2);
    title(['Harmonic Mode ', num2str(n)]);
    xlabel('Position (m)');
    ylabel('Amplitude in dB');
    grid on;
end
sgtitle('Standing Wave Harmonics');



% Space and time setup
x = linspace(0, L, 1000); % 100 spatial points along the tube
t = linspace(0, 2/f, 2000); % 200 time steps for slider control

[X, T] = meshgrid(x, t); % Create a space-time grid

% Define the incident and reflected waves
incident_wave = A * cos(k * X - omega * T);              % Traveling towards x = L
reflected_wave = R * A * cos(k * X + omega * T + pi);    % Reflected with 180° phase shift

% Compute the total standing wave
standing_wave = incident_wave + reflected_wave;

% Create the figure
fig = figure(4);

% Store data inside fig for access in the callback
fig.UserData.incident_wave = incident_wave;
fig.UserData.reflected_wave = reflected_wave;
fig.UserData.standing_wave = standing_wave;
fig.UserData.D = D;  % Store tube diameter

% Create slider UI for time control
slider = uicontrol('Style', 'slider', 'Min', 1, 'Max', length(t), 'Value', 1, ...
    'Units', 'normalized', 'Position', [0.25 0.02 0.5 0.03], ...
    'Callback', @(src, event) updatePlot(round(get(src, 'Value')), fig));

% Axes setup
ax1 = subplot(3,1,1); hold on;
fig.UserData.incident_plot = plot(x, incident_wave(1, :), 'b', 'LineWidth', 2);
fig.UserData.reflected_plot = plot(x, reflected_wave(1, :), 'r', 'LineWidth', 2);
title('Incident and Reflected Waves');
legend('Incident', 'Reflected');
xlabel('Position (m)'); ylabel('Amplitude in dB');
ylim([-2*A 2*A]); grid on;
xticks([0:0.01:1]);

ax2 = subplot(3,1,2);
fig.UserData.standing_plot = plot(x, standing_wave(1, :), 'k', 'LineWidth', 2);
title('Resulting Standing Wave');
xlabel('Position (m)'); ylabel('Amplitude in dB');
ylim([-2*A 2*A]); grid on;
xticks([0:0.025:1]);

ax3 = subplot(3,1,3); hold on;
tube_x = [x fliplr(x)];
tube_y = [D/2 * ones(1, length(x)), -D/2 * ones(1, length(x))];
fill(tube_x, tube_y, 'c', 'FaceAlpha', 0.3, 'EdgeColor', 'k');
fig.UserData.standing_wave_tube = plot(x, standing_wave(1, :) / max(abs(standing_wave(:))) * (D/2), 'k', 'LineWidth', 2);
title('Tube Representation with Standing Wave');
xlabel('Position (m)'); ylabel('Tube Diameter');
ylim([-D D]); grid on;
xticks([0:0.025:1]);
hold off;

figure(5);
hold on;
plot(x, incident_wave(1, :), 'b', 'LineWidth', 2);
plot(x, reflected_wave(1, :), 'r', 'LineWidth', 2);
title('Incident and Reflected Waves');
legend('Incident', 'Reflected');
xlabel('Position (m)'); ylabel('Amplitude in dB');
ylim([0 A]); grid on;
hold off;

% Initialize plot at t = 1
updatePlot(1, fig);

% Function to update plots when slider is moved
function updatePlot(idx, fig)
    % Retrieve stored plots and variables
    incident_plot = fig.UserData.incident_plot;
    reflected_plot = fig.UserData.reflected_plot;
    standing_plot = fig.UserData.standing_plot;
    standing_wave_tube = fig.UserData.standing_wave_tube;
    
    % Retrieve wave data
    incident_wave = fig.UserData.incident_wave;
    reflected_wave = fig.UserData.reflected_wave;
    standing_wave = fig.UserData.standing_wave;
    D = fig.UserData.D;  % Retrieve tube diameter

    % Update plots
    set(incident_plot, 'YData', incident_wave(idx, :));
    set(reflected_plot, 'YData', reflected_wave(idx, :));
    set(standing_plot, 'YData', standing_wave(idx, :));
    set(standing_wave_tube, 'YData', standing_wave(idx, :) / max(abs(standing_wave(:))) * (D/2));
    drawnow;
end