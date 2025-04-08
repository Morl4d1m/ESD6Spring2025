clf;
% Room dimensions (modify as needed)
x = 10; % Length in meters
y = 8;  % Width in meters
z = 5;  % Height in meters

% Number of nodal planes in each direction
Nx = 3; % Number of planes along X-axis
Ny = 3; % Number of planes along Y-axis
Nz = 3; % Number of planes along Z-axis

figure(1); hold on;
xlabel('X (m)'); ylabel('Y (m)'); zlabel('Z (m)');
axis([0 x 0 y 0 z]);
grid on;
view(3);

% Plot nodal planes along X-axis
for i = 1:Nx
    x_plane = (i / (Nx + 1)) * x;
    plane = patch([x_plane x_plane x_plane x_plane], [0 y y 0], [0 0 z z], 'r');
    set(plane, 'FaceAlpha', 0.3, 'EdgeColor', 'none');
end

% Plot nodal planes along Y-axis
for i = 1:Ny
    y_plane = (i / (Ny + 1)) * y;
    plane = patch([0 x x 0], [y_plane y_plane y_plane y_plane], [0 0 z z], 'g');
    set(plane, 'FaceAlpha', 0.3, 'EdgeColor', 'none');
end

% Plot nodal planes along Z-axis
for i = 1:Nz
    z_plane = (i / (Nz + 1)) * z;
    plane = patch([0 x x 0], [0 0 y y], [z_plane z_plane z_plane z_plane], 'b');
    set(plane, 'FaceAlpha', 0.3, 'EdgeColor', 'none');
end

legend({'X-planes', 'Y-planes', 'Z-planes'}, 'Location', 'best');
title('3D Rendering of Nodal Planes in a Room');
hold off;

% Room dimensions (modify as needed)
x = 10; % Length in meters
y = 8;  % Width in meters
z = 5;  % Height in meters

% Wave parameters
Nx = 3; % Number of wave nodes in X direction
Ny = 2; % Number of wave nodes in Y direction

[X, Y] = meshgrid(linspace(0, x, 50), linspace(0, y, 50));
kx = Nx * pi / x; ky = Ny * pi / y;
wave = cos(kx * X) .* cos(ky * Y); % Using cosine to start at maxima/minima along walls

% Create figure
figure(2); hold on;
colormap(jet);

% Plot 3D wave surface
surf(X, Y, wave, 'EdgeColor', 'none', 'FaceAlpha', 0.8);

% Plot contour projection on the XY plane
contour3(X, Y, wave, 16, 'k', 'LineWidth', 1);

% Labels and aesthetics
xlabel('x (m)'); ylabel('y (m)'); zlabel('\psi(x,y)');
title('3D Nodal Planes and Wave Visualization');
axis tight;
grid on;
view(3);
colorbar;

hold off;


% Inputs: Desired frequencies and room dimensions
fx = 50; % Frequency along x-axis in Hz
fy = 40; % Frequency along y-axis in Hz
x = 10; % Room length in meters
y = 8;  % Room width in meters
z = 5;  % Room height in meters

% Physical constants
c = 343; % Speed of sound in air (m/s)
lambda_x = c / fx; % Wavelength along x
lambda_y = c / fy; % Wavelength along y

% Wave number calculations
kx = 2 * pi / lambda_x;
ky = 2 * pi / lambda_y;

[X, Y] = meshgrid(linspace(0, x, 50), linspace(0, y, 50));
wave = cos(kx * X) .* cos(ky * Y); % Using cosine to start at maxima/minima along walls

% Create figure
figure(3); hold on;
colormap(jet);

% Plot 3D wave surface
surf(X, Y, wave, 'EdgeColor', 'none', 'FaceAlpha', 0.8);

% Plot contour projection on the XY plane
contour3(X, Y, wave, 16, 'k', 'LineWidth', 1);

% Labels and aesthetics
xlabel('x (m)'); ylabel('y (m)'); zlabel('\psi(x,y)');
title('3D Nodal Planes and Wave Visualization Based on Frequncy and Room Size');
axis tight;
grid on;
view(3);
colorbar;

hold off;


% Room dimensions
x = 100; % Room length in meters
y = 80;  % Room width in meters

% Physical constants
c = 343; % Speed of sound in air (m/s)

% Define sound sources (position and frequency)
sources = [
    30, 40, 50, true;  % x, y, frequency (Hz), on/off (1 = on, 0 = off)
    7, 2, 40, false;
    5, 6, 60, false
];

% Create grid
[X, Y] = meshgrid(linspace(0, x, 100), linspace(0, y, 100));
wave = zeros(size(X));

% Compute wave contribution from each active source
for i = 1:size(sources, 1)
    if sources(i, 4) % Check if source is active
        sx = sources(i, 1);
        sy = sources(i, 2);
        f = sources(i, 3);
        lambda = c / f;
        k = 2 * pi / lambda;
        r = sqrt((X - sx).^2 + (Y - sy).^2) + 1e-6; % Avoid division by zero
        wave = wave + cos(k * r); % Ensure maxima at source
    end
end

% Create figure
figure(4); hold on;
colormap(jet);

% Plot 3D wave surface
surf(X, Y, wave, 'EdgeColor', 'none', 'FaceAlpha', 0.8);

% Plot contour projection on the XY plane
contour3(X, Y, wave, 16, 'k', 'LineWidth', 1);

% Labels and aesthetics
xlabel('x (m)'); ylabel('y (m)'); zlabel('\psi(x,y)');
title('Wave Propagation from Multiple Sound Sources');
axis tight;
grid on;
view(3);
colorbar;

hold off;



% Room dimensions (modify as needed)
x = 10; % Length in meters
y = 8;  % Width in meters

% Sound source parameters
sources = [2, 3, 500, 1;  % [x, y, frequency (Hz), on/off]
           8, 5, 1500, 0;
           8, 3, 1500, 0;
           2, 5, 1500, 0];

c = 343; % Speed of sound in air (m/s)
SPL_source = 60; % Sound Pressure Level at source (dB)
alpha_air = 1.0002; % Attenuation coefficient in air (approx. dB/m)

% Reflection coefficients at walls (1 means full reflection)
R_left = 0; R_right = 0;
R_top = 0; R_bottom = 0;

% Grid setup
[X, Y] = meshgrid(linspace(0, x, 100), linspace(0, y, 100));
pressure_field = zeros(size(X));

for i = 1:size(sources, 1)
    if sources(i, 4) == 1 % Check if source is on
        xs = sources(i, 1);
        ys = sources(i, 2);
        freq = sources(i, 3);
        lambda = c / freq;
        k = 2 * pi / lambda;
        
        % Calculate distance from source
        dist = sqrt((X - xs).^2 + (Y - ys).^2);
        
        % Apply attenuation
        SPL = SPL_source - alpha_air * dist;
        amplitude = 10.^(SPL / 20);
        
        % Wave propagation with reflections
        wave = amplitude .* cos(k * dist);
        
        % Apply reflections at boundaries
        wave = wave + R_left * amplitude .* cos(k * abs(X));
        wave = wave + R_right * amplitude .* cos(k * abs(X - x));
        wave = wave + R_top * amplitude .* cos(k * abs(Y - y));
        wave = wave + R_bottom * amplitude .* cos(k * abs(Y));
        
        pressure_field = pressure_field + wave;
    end
end

% Visualization
figure(5);
surf(X, Y, pressure_field, 'EdgeColor', 'none', 'FaceAlpha', 0.8);
hold on;
contour3(X, Y, pressure_field, 16, 'k', 'LineWidth', 1);
%scatter3(sources(:,1), sources(:,2), max(pressure_field(:)) * ones(size(sources,1),1), 100, 'r', 'filled');

% Labels and aesthetics
xlabel('x (m)'); ylabel('y (m)'); zlabel('Pressure (Pa)');
title('Sound Propagation with Attenuation and Reflection');
axis tight;
grid on;
view(3);
colorbar;
hold off;



% Room dimensions (modify as needed)
x = 10; % Length in meters
y = 8;  % Width in meters

% Sound source parameters
sources = [5, 4, 200, 1;  % [x, y, frequency (Hz), on/off]
           7, 5, 300, 0;
           5, 7, 150, 0];

c = 343; % Speed of sound in air (m/s)
SPL_source = 60; % Sound Pressure Level at source (dB)
alpha_air = 0.0002; % Attenuation coefficient in air (approx. dB/m)

% Define walls as start and end coordinates with reflection, absorption, and transmission loss coefficients
walls = [1, 1, 9, 1, 1, 0.1, 100;  % [x1, y1, x2, y2, reflection coefficient, absorption coefficient, transmission loss (dB)]
         1, 7, 9, 7, 1, 0.2, 100;
         1, 1, 1, 7, 1, 0.2, 100;
         9, 1, 9, 7, 1, 0.15, 100];

% Grid setup
[X, Y] = meshgrid(linspace(0, x, 100), linspace(0, y, 100));
pressure_field = zeros(size(X));

for i = 1:size(sources, 1)
    if sources(i, 4) == 1 % Check if source is on
        xs = sources(i, 1);
        ys = sources(i, 2);
        freq = sources(i, 3);
        lambda = c / freq;
        k = 2 * pi / lambda;
        
        % Calculate distance from source
        dist = sqrt((X - xs).^2 + (Y - ys).^2);
        
        % Apply attenuation
        SPL = SPL_source - alpha_air * dist;
        amplitude = 10.^((SPL - 94) / 20); % Convert dB to linear scale (assuming reference 94 dB)
        
        % Wave propagation
        wave = amplitude .* cos(k * dist);
        
        % Apply reflections, absorption, and transmission loss at walls
        for j = 1:size(walls, 1)
            x1 = walls(j, 1); y1 = walls(j, 2);
            x2 = walls(j, 3); y2 = walls(j, 4);
            R = walls(j, 5);
            A = walls(j, 6); % Absorption coefficient
            TL = walls(j, 7); % Transmission loss in dB
            
            % Compute distances to the wall
            dx = x2 - x1; dy = y2 - y1;
            wall_length = sqrt(dx^2 + dy^2);
            
            % Normal vector to the wall
            nx = dy / wall_length;
            ny = -dx / wall_length;
            
            % Distance from each grid point to the closest point on the wall
            proj_length = ((X - x1) * dx + (Y - y1) * dy) / wall_length;
            proj_x = x1 + proj_length * dx / wall_length;
            proj_y = y1 + proj_length * dy / wall_length;
            
            % Mask out points that project outside the segment
            mask = (proj_length >= 0) & (proj_length <= wall_length);
            
            % Compute reflection distances
            reflection_dist = sqrt((X - proj_x).^2 + (Y - proj_y).^2);
            reflected_wave = (R * (1 - A)) * amplitude .* cos(k * reflection_dist);
            
            % Determine if a point is behind the wall relative to the source
            source_to_wall = sign((xs - x1) * nx + (ys - y1) * ny);
            point_to_wall = sign((X - x1) * nx + (Y - y1) * ny);
            behind_wall = (source_to_wall ~= point_to_wall);
            
            % Ensure walls correctly block sound at corners
            if any(behind_wall(:))
                wave(behind_wall) = wave(behind_wall) .* 10.^(-TL/20); % Apply transmission loss
            end
            
            % Apply reflections properly
            wave(mask & ~behind_wall) = wave(mask & ~behind_wall) + reflected_wave(mask & ~behind_wall);
        end
        
        pressure_field = pressure_field + wave;
    end
end

% Visualization
figure(6);
surf(X, Y, pressure_field, 'EdgeColor', 'none', 'FaceAlpha', 0.8);
hold on;
contour3(X, Y, pressure_field, 16, 'k', 'LineWidth', 1);
%scatter3(sources(:,1), sources(:,2), max(pressure_field(:)) * ones(size(sources,1),1), 100, 'r', 'filled');

% Plot walls
for j = 1:size(walls, 1)
    plot3([walls(j,1), walls(j,3)], [walls(j,2), walls(j,4)], [max(pressure_field(:)), max(pressure_field(:))], 'b', 'LineWidth', 3);
end

% Labels and aesthetics
xlabel('x (m)'); ylabel('y (m)'); zlabel('Pressure (Pa)');
title('Sound Propagation with Attenuation, Reflection, Absorption, and Transmission Loss');
axis tight;
grid on;
view(3);
colorbar;
hold off;
