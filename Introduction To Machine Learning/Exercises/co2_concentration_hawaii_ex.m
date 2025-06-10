%clear all
%close all
%clc

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% CONFIG
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% SETUP
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% import data (The complete data set)
data = import_co2_concentration('co2_weekly_mlo.txt');
N = size(data,1); % Number of data points

% Fill missing values
int_thr = 1;
data.int_idx = data.co2_ppm<int_thr;
data.co2_ppm(data.int_idx) = nan;
data.co2_ppm = fillmissing(data.co2_ppm,'movmedian',10);
data.dt = day_counter2datetime(data);

% Split data
Q = floor(N/2);

% Data used for the fit
data_fit = data(1:Q,:);

% Data used for validation
data_val = data(Q+1:N,:);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% FILL IN CODE HERE
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Plot the CO2 concentration over time
figure(1);
plot(data.dt, data.co2_ppm, 'b');
hold on;
plot(data_fit.dt, data_fit.co2_ppm, 'r', 'LineWidth', 2); % Mark training data
xlabel('Year');
ylabel('CO2 concentration (ppm)');
title('CO2 Concentration in Hawaii (1974-2020)');
legend('Complete Data', 'Training Data');
grid on;

sampling_period_days = mean(days(diff(data.dt))); % Time difference between samples in days
sampling_period_weeks = sampling_period_days / 7;
fprintf('Average sampling period: %.2f weeks\n', sampling_period_weeks);

% Define time variable normalized to start at zero
t = years(data_fit.dt - data_fit.dt(1));

% Construct basis functions
X = [ones(size(t)), t, t.^2, cos(2*pi*t), sin(2*pi*t)];

% Solve for coefficients using least squares
c = X \ data_fit.co2_ppm;

% Predict values
t_val = years(data_val.dt - data_fit.dt(1));
X_val = [ones(size(t_val)), t_val, t_val.^2, cos(2*pi*t_val), sin(2*pi*t_val)];
y_pred = X_val * c;

% Plot the results
figure(2);
plot(data_fit.dt, data_fit.co2_ppm, 'r', 'LineWidth', 2); hold on;
plot(data_val.dt, data_val.co2_ppm, 'b', 'LineWidth', 2);
plot(data_val.dt, y_pred, 'k', 'LineWidth', 2);
xlabel('Year');
ylabel('CO2 concentration (ppm)');
title('Linear Regression Model Fit and Extrapolation');
legend('Training Data', 'Validation Data', 'Extrapolated Model');
grid on;


% Extract time and CO2 concentration from data
t_fit = datenum(data_fit.dt);  % Convert datetime to numeric format
y_fit = data_fit.co2_ppm;      % Measured CO2 concentration

% Choose basis functions (e.g., polynomial + seasonal)
A = [ones(size(t_fit)), t_fit, t_fit.^2, sin(2*pi*t_fit/365.25), cos(2*pi*t_fit/365.25)];

% Solve for coefficients using least squares
c = A \ y_fit;

% Compute fitted values
y_fit_pred = A * c;

% Plot
figure(3);
plot(data_fit.dt, y_fit, 'b', 'DisplayName', 'Measured CO2');
hold on;
plot(data_fit.dt, y_fit_pred, 'r', 'DisplayName', 'Fitted Model');
xlabel('Time');
ylabel('CO2 Concentration (ppm)');
title('Least Squares Fit to CO2 Data');
legend;
hold off;


t_val = datenum(data_val.dt);  % Future time points
A_val = [ones(size(t_val)), t_val, t_val.^2, sin(2*pi*t_val/365.25), cos(2*pi*t_val/365.25)];
y_val_pred = A_val * c;

% Plot extrapolation
figure(4);
plot(data.dt, data.co2_ppm, 'b', 'DisplayName', 'Full CO2 Data');
hold on;
plot(data_val.dt, y_val_pred, 'r--', 'DisplayName', 'Extrapolated Fit');
xlabel('Time');
ylabel('CO2 Concentration (ppm)');
title('Extrapolation into the Future');
legend;
hold off;

% Generate 100 basis functions
num_basis = 100;
A_large = ones(size(t_fit));  % Start with bias term
for i = 1:num_basis-1
    A_large(:, end+1) = t_fit.^i;  % Polynomial terms
end

% Solve
c_large = A_large \ y_fit;

% Predict and plot extrapolation
A_large_val = ones(size(t_val));
for i = 1:num_basis-1
    A_large_val(:, end+1) = t_val.^i;
end
y_large_pred = A_large_val * c_large;

% Plot overfitting result
figure(5);
plot(data.dt, data.co2_ppm, 'b', 'DisplayName', 'Full CO2 Data');
hold on;
plot(data_val.dt, y_large_pred, 'r', 'DisplayName', 'Overfit Extrapolation');
xlabel('Time');
ylabel('CO2 Concentration (ppm)');
title('Overfitting with 100 Basis Functions');
legend;
hold off;


%MM4 C+D
%% Load CO2 Data
data = import_co2_concentration('co2_weekly_mlo.txt');  
N = size(data,1); % Number of data points

% Fill missing values
int_thr = 1;
data.int_idx = data.co2_ppm < int_thr;
data.co2_ppm(data.int_idx) = nan;
data.co2_ppm = fillmissing(data.co2_ppm, 'movmedian', 10);
data.dt = day_counter2datetime(data);

% Split data into training (first half) and validation (second half)
Q = floor(N/2);
data_fit = data(1:Q, :);
data_val = data(Q+1:N, :);

%% Define Basis Functions for Regression
t = (1:Q)';  % Time index (assume uniform sampling)

% Polynomial basis functions (linear + quadratic trend)
X = [ones(Q,1), t, t.^2];  % Design matrix (constant, linear, quadratic)
y = data_fit.co2_ppm;       % Target variable (CO2 concentration)

%% Least Squares Solution (Optimal Reference)
c_ls = (X' * X) \ (X' * y);

%% Steepest Descent Implementation
max_iter = 100000;  
tol = 1e-6;
alpha = 1e-10;  % Step size (too small slows convergence, too large diverges)

c_sd = zeros(3,1);  % Initialize coefficients
for iter = 1:max_iter
    grad = -2 * X' * (y - X * c_sd);  % Gradient of cost function
    c_sd = c_sd - alpha * grad;       % Update coefficients
    
    % Convergence check
    if norm(grad) < tol
        break;
    end
end

fprintf('Steepest Descent converged in %d iterations\n', iter);

%% Newton-Raphson Implementation
c_nr = zeros(3,1);  % Initialize coefficients
for iter = 1:20  % Newton converges in a few iterations
    grad = -2 * X' * (y - X * c_nr);   % Gradient
    H = 2 * (X' * X);                  % Hessian (always positive definite)
    
    c_nr = c_nr - H \ grad;  % Newton update
    
    % Convergence check
    if norm(grad) < tol
        break;
    end
end

fprintf('Newton-Raphson converged in %d iterations\n', iter);

%% Plot Results
figure(6);
hold on;
plot(t, y, 'b.', 'DisplayName', 'CO2 Data');
plot(t, X * c_ls, 'k--', 'LineWidth', 2, 'DisplayName', 'Least Squares Fit');
plot(t, X * c_sd, 'r-.', 'LineWidth', 2, 'DisplayName', 'Steepest Descent');
plot(t, X * c_nr, 'g-', 'LineWidth', 2, 'DisplayName', 'Newton-Raphson');
legend;
xlabel('Time');
ylabel('CO2 Concentration (ppm)');
title('Regression Results');
grid on;
hold off;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% END
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Functions (Must be in end of file)
function dt = day_counter2datetime(data)
    dt =datetime(data.year, data.month, data.day);
end