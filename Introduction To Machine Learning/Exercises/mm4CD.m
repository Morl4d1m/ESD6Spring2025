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
max_iter = 1000;  
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
figure;
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
