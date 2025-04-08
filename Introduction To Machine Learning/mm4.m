% Define the function and its derivatives
alpha_values = [1, 100];  % Two cases for alpha
for i = 1:length(alpha_values)
    alpha = alpha_values(i);
    
    % Function definition
    f = @(x) alpha*x(1)^2 + x(2)^2;
    
    % Gradient of f
    grad_f = @(x) [2*alpha*x(1); 2*x(2)];
    
    % Hessian of f
    Hf = @(x) [2*alpha, 0; 0, 2];

    % Initial guess
    x0 = [-2; 2];

    % Optimization settings
    max_iter = 100;
    tol = 1e-6;
    method = 'SD';  % Newton-Raphson method
    stepsize_rule = 3;  % Constant step size

    % Run the optimization
    [xmin, fmin, x, iter] = unconstrained_opt(f, grad_f, Hf, x0, max_iter, tol, method, stepsize_rule);

    % Display results
    fprintf('Results for α = %d:\n', alpha);
    fprintf('xmin = [%.6f, %.6f]\n', xmin(1), xmin(2));
    fprintf('fmin = %.6f\n', fmin);
    fprintf('Iterations: %d\n\n', iter);

    % Plot convergence
    figure(i);
    plot(0:iter, vecnorm(x(:,1:iter+1)), 'o-');
    xlabel('Iteration');
    ylabel('Norm of x');
    title(sprintf('Convergence for \\alpha = %d', alpha));
    grid on;
end

% import data (The complete data set)
data = import_co2_concentration('C:\Users\Christian Lykke\Documents\Skole\Aalborg Universitet\ESD6\Introduction To Machine Learning\Ls(1)\exercise\co2_weekly_mlo.txt',[50, Inf]);
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


