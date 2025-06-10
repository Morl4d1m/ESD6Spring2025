clear all
close all
clc

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% CONFIG
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
filename = 'co2_weekly_mlo.txt';
num_poly_basis = 5;         % Number of polynomial (trend) basis functions
num_basis_large = 100;      % For overfitting demo
lambdas = [0, 0.01, 0.1, 1, 10, 100]; % Regularization values

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% SETUP
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Import data
data = import_co2_concentration(filename);
if istable(data)
    data = table2struct(data,'ToScalar',true);
end

N = length(data.co2_ppm);

% Handle missing values
int_thr = 1;
bad_idx = data.co2_ppm<int_thr | isnan(data.co2_ppm);
data.co2_ppm(bad_idx) = nan;
data.co2_ppm = fillmissing(data.co2_ppm,'movmedian',10);
data.dt = day_counter2datetime(data);

% Split data
Q = floor(N/2);
data_fit.dt = data.dt(1:Q);
data_fit.co2_ppm = data.co2_ppm(1:Q);
data_val.dt = data.dt(Q+1:end);
data_val.co2_ppm = data.co2_ppm(Q+1:end);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% EXERCISE: Linear regression, Ridge, Lasso, Overfitting, Extrapolation
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%% Plot the CO2 concentration over time
figure(1); clf
plot(data.dt, data.co2_ppm, 'b', 'DisplayName','Full Data'); hold on;
plot(data_fit.dt, data_fit.co2_ppm, 'r', 'LineWidth', 2, 'DisplayName','Training Data');
xlabel('Year'); ylabel('CO2 concentration (ppm)');
title('CO2 Concentration in Hawaii (1974-2020)');
legend; grid on;

sampling_period_days = mean(days(diff(data.dt)));
sampling_period_weeks = sampling_period_days / 7;
fprintf('Average sampling period: %.2f weeks\n', sampling_period_weeks);

%% Define time variables (normalized)
t_fit = years(data_fit.dt - data_fit.dt(1));
t_val = years(data_val.dt - data_fit.dt(1));
y_fit = data_fit.co2_ppm;
y_val = data_val.co2_ppm;

%% --- Basis construction (polynomial + optional seasonal) ---
% You can include Fourier/seasonal terms if desired:
add_seasonal = true;
if add_seasonal
    X_fit = [ones(size(t_fit)), t_fit, t_fit.^2, cos(2*pi*t_fit), sin(2*pi*t_fit)];
    X_val = [ones(size(t_val)), t_val, t_val.^2, cos(2*pi*t_val), sin(2*pi*t_val)];
else
    X_fit = ones(length(t_fit),1);
    X_val = ones(length(t_val),1);
    for d = 1:num_poly_basis-1
        X_fit = [X_fit t_fit.^d];
        X_val = [X_val t_val.^d];
    end
end

%% Ordinary Least Squares (No regularization)
c_ls = X_fit \ y_fit;
y_pred_fit_ls = X_fit * c_ls;
y_pred_val_ls = X_val * c_ls;

figure(2); clf
plot(data_fit.dt, y_fit, 'r', 'LineWidth', 2, 'DisplayName','Training Data'); hold on;
plot(data_val.dt, y_val, 'b', 'LineWidth', 2, 'DisplayName','Validation Data');
plot(data_val.dt, y_pred_val_ls, 'k', 'LineWidth', 2, 'DisplayName','OLS Extrapolated');
xlabel('Year'); ylabel('CO2 concentration (ppm)');
title('OLS Fit and Extrapolation');
legend; grid on;

%% Ridge Regression (L2)
figure(3); clf
plot(data_fit.dt, y_fit, 'b.', 'DisplayName','Train Data'); hold on;
plot(data_val.dt, y_val, 'k.', 'DisplayName','Validation Data');
colors = lines(length(lambdas));
for i = 1:length(lambdas)
    lambda = lambdas(i);
    c_ridge = (X_fit' * X_fit + lambda * eye(size(X_fit,2))) \ (X_fit' * y_fit);
    y_pred_val = X_val * c_ridge;
    plot(data_val.dt, y_pred_val, '-', 'Color', colors(i,:), 'LineWidth', 2, ...
        'DisplayName', sprintf('Ridge \\lambda=%.2g', lambda));
end
xlabel('Year'); ylabel('CO2 (ppm)');
title('Ridge Regression: Extrapolation');
legend('show'); grid on;

%% Lasso Regression (L1)
figure(4); clf
plot(data_fit.dt, y_fit, 'b.', 'DisplayName','Train Data'); hold on;
plot(data_val.dt, y_val, 'k.', 'DisplayName','Validation Data');
for i = 1:length(lambdas)
    if lambdas(i)==0, continue; end
    [c_lasso, ~] = lasso(X_fit, y_fit, 'Lambda', lambdas(i), 'Standardize', false);
    y_pred_val = X_val * c_lasso;
    plot(data_val.dt, y_pred_val, '-', 'Color', colors(i,:), 'LineWidth', 2, ...
        'DisplayName', sprintf('Lasso \\lambda=%.2g', lambdas(i)));
end
xlabel('Year'); ylabel('CO2 (ppm)');
title('Lasso Regression: Extrapolation');
legend('show'); grid on;

%% Overfitting demo: 100 basis functions
t_fit_num = years(data_fit.dt - data_fit.dt(1));
t_val_num = years(data_val.dt - data_fit.dt(1));
A_large = ones(length(t_fit_num),1); A_large_val = ones(length(t_val_num),1);
for i = 1:num_basis_large-1
    A_large(:, end+1) = t_fit_num.^i;
    A_large_val(:, end+1) = t_val_num.^i;
end

% OLS
c_large = A_large \ y_fit;
y_large_pred = A_large_val * c_large;

% Ridge (regularize heavily to control overfit)
lambda_large = 1000;
c_ridge_large = (A_large' * A_large + lambda_large * eye(num_basis_large)) \ (A_large' * y_fit);
y_pred_ridge_large = A_large_val * c_ridge_large;

% Lasso (regularize heavily, could be slow)
lambda_lasso_large = 1;
[c_lasso_large,~] = lasso(A_large, y_fit, 'Lambda', lambda_lasso_large, 'Standardize', false);
y_pred_lasso_large = A_large_val * c_lasso_large;

figure(5); clf
plot(data.dt, data.co2_ppm, 'k.', 'DisplayName','Full Data'); hold on;
plot(data_val.dt, y_large_pred, 'r-', 'LineWidth', 2, 'DisplayName','100-Basis OLS');
plot(data_val.dt, y_pred_ridge_large, 'b-', 'LineWidth', 2, 'DisplayName','100-Basis Ridge');
plot(data_val.dt, y_pred_lasso_large, 'm-', 'LineWidth', 2, 'DisplayName','100-Basis Lasso');
xlabel('Year'); ylabel('CO2 (ppm)');
title('Overfitting and Regularization');
legend('show'); grid on;

%% Optimization: Steepest Descent and Newton-Raphson
% For simple poly2: X = [ones(Q,1), t, t.^2]
t_simple = (1:Q)';
X_simple = [ones(Q,1), t_simple, t_simple.^2];
y_simple = data_fit.co2_ppm;

c_ls_simple = (X_simple' * X_simple) \ (X_simple' * y_simple);

max_iter = 100000; tol = 1e-6; alpha = 1e-10;
c_sd = zeros(3,1);
for iter = 1:max_iter
    grad = -2 * X_simple' * (y_simple - X_simple * c_sd);
    c_sd = c_sd - alpha * grad;
    if norm(grad) < tol, break; end
end
fprintf('Steepest Descent converged in %d iterations\n', iter);

c_nr = zeros(3,1);
for iter = 1:20
    grad = -2 * X_simple' * (y_simple - X_simple * c_nr);
    H = 2 * (X_simple' * X_simple);
    c_nr = c_nr - H \ grad;
    if norm(grad) < tol, break; end
end
fprintf('Newton-Raphson converged in %d iterations\n', iter);

figure(6); clf
hold on;
plot(t_simple, y_simple, 'b.', 'DisplayName', 'CO2 Data');
plot(t_simple, X_simple * c_ls_simple, 'k--', 'LineWidth', 2, 'DisplayName', 'Least Squares');
plot(t_simple, X_simple * c_sd, 'r-.', 'LineWidth', 2, 'DisplayName', 'Steepest Descent');
plot(t_simple, X_simple * c_nr, 'g-', 'LineWidth', 2, 'DisplayName', 'Newton-Raphson');
legend; xlabel('Time Index'); ylabel('CO2 (ppm)');
title('Regression (Poly2) via Different Optimization Methods');
grid on;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% END
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% Functions (Must be at end of file)
function dt = day_counter2datetime(data)
    dt = datetime(data.year, data.month, data.day);
end
