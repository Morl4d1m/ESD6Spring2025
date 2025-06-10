% Script for part (b) of the exercise

alphas = [1, 100];
for aidx = 1:2
    alpha = alphas(aidx);
    fprintf('\n===== alpha = %d =====\n', alpha);

    % Define function, gradient, and Hessian
    f = @(x) alpha*x(1)^2 + x(2)^2;
    grad_f = @(x) [2*alpha*x(1); 2*x(2)];
    Hf = @(x) [2*alpha, 0; 0, 2];

    % Initial guess
    x0 = [5; 5];
    max_iter = 20000;
    tol = 1e-8;

    % Test Steepest Descent
    fprintf('--- Steepest Descent ---\n');
    [xmin_SD, fmin_SD, traj_SD, iter_SD] = unconstrained_opt(f, grad_f, Hf, x0, max_iter, tol, 'SD', 3);

    % Test Newton-Raphson
    fprintf('--- Newton-Raphson ---\n');
    [xmin_NR, fmin_NR, traj_NR, iter_NR] = unconstrained_opt(f, grad_f, Hf, x0, max_iter, tol, 'NR', 3);

    % Discussion (written in comments for your report):
    % When alpha = 1 (well-conditioned), both SD and NR converge quickly.
    % When alpha = 100 (ill-conditioned/elongated contours), SD is very slow (zig-zag behavior), NR remains fast.
end

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% EXERCISE (c) - Steepest Descent vs Newton-Raphson for CO2 Regression
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% -- Use the same design matrix X and target y as in your previous sections --
% X = [ones(Q,1), t, t.^2];
% y = data_fit.co2_ppm;

% --- Steepest Descent ---
max_iter_sd = 100000;
tol_sd = 1e-6;
alpha_sd = 1e-10; % You can experiment with this, but small is often needed!
c_sd = zeros(3,1); % initial guess

for iter_sd = 1:max_iter_sd
    grad_sd = -2 * X' * (y - X * c_sd);      % Gradient of least squares cost
    c_sd = c_sd - alpha_sd * grad_sd;        % Steepest Descent update
    if norm(grad_sd) < tol_sd
        break;
    end
end
fprintf('Steepest Descent converged in %d iterations\n', iter_sd);

% --- Newton-Raphson ---
max_iter_nr = 20000;
tol_nr = 1e-6;
c_nr = zeros(3,1); % initial guess

for iter_nr = 1:max_iter_nr
    grad_nr = -2 * X' * (y - X * c_nr);      % Gradient
    H_nr = 2 * (X' * X);                     % Hessian (constant for least squares)
    c_nr = c_nr - H_nr \ grad_nr;            % Newton-Raphson update
    if norm(grad_nr) < tol_nr
        break;
    end
end
fprintf('Newton-Raphson converged in %d iterations\n', iter_nr);

% --- Plot results for comparison ---
figure;
hold on;
plot(t, y, 'b.', 'DisplayName', 'CO2 Data');
plot(t, X * c_ls, 'k--', 'LineWidth', 2, 'DisplayName', 'Least Squares Fit');
plot(t, X * c_sd, 'r-.', 'LineWidth', 2, 'DisplayName', 'Steepest Descent');
plot(t, X * c_nr, 'g-', 'LineWidth', 2, 'DisplayName', 'Newton-Raphson');
legend;
xlabel('Time');
ylabel('CO2 Concentration (ppm)');
title('Regression Results: LS, SD, Newton for CO2 Data');
grid on;
hold off;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% EXERCISE (d) - Why Steepest Descent Fails for the CO2 Least Squares Problem
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Written explanation for report (copy to your report as needed):
%
% The CO2 regression problem involves minimizing a least-squares cost with a design
% matrix (X) that is typically ill-conditioned (columns nearly linearly dependent).
% For such problems, steepest descent moves orthogonally to level curves, but because
% the contours of the error surface are highly elongated, progress toward the minimum
% is very slow. The algorithm "zig-zags" down the valley, requiring many steps,
% especially with a small step size (alpha). A larger step size leads to divergence.
% Newton-Raphson, on the other hand, uses the Hessian (curvature information) to
% rescale the space, moving directly to the solution. For quadratic problems like
% least squares, Newton-Raphson converges in just a few iterations (often one).
% This is observed in the much faster convergence in the script above.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


function [xmin, fmin, x, iter] = unconstrained_opt(f, grad_f, Hf, x0, max_iter, tol, method, stepsize_rule)
% UNCONSTRAINED_OPT  Unconstrained optimization using Steepest Descent or Newton-Raphson.
%
%   [xmin, fmin,x, iter] = unconstrained_opt(f, grad_f, Hf, x0, max_iter, tol, method, stepsize_rule)
%
%   Input arguments:
%     f             - Handle to the cost function, e.g., @(x) 0.5*x'*A*x - b'*x.
%     grad_f        - Handle to the gradient of f.
%     Hf            - Handle to the Hessian of f.
%     x0            - Initial guess (column vector).
%     max_iter      - Maximum number of iterations.
%     tol           - Tolerance for stopping criterion.
%     method        - Optimization algorithm: 'SD' (Steepest Descent) or 'NR' (Newton-Raphson).
%     stepsize_rule - Integer (1 to 3) selecting the step size rule:
%                     1: Golden Section Search (requires separate function 'GSS')
%                     2: Backtracking Line Search (requires separate function 'BLS')
%                     3: Constant step size (default)
%
%   Output arguments:
%     xmin  - The computed minimizer (approximate solution).
%     fmin  - The cost function value at xmin.
%     x     - The trajectory of the optimization.
%     iter  - The number of iterations performed.
%
%  EXERCISE (a): Fill in the missing code to complete the search direction and stopping criteria.
%

    % Set defaults for missing input arguments.
    if nargin < 8, stepsize_rule = 3; end  % Default: constant step size
    if nargin < 7, method = 'SD'; end      % Default: Steepest Descent

    n = length(x0);
    x = zeros(n, max_iter+1); % Store iterates
    x(:,1) = x0;
    alpha = zeros(1, max_iter);
    d = zeros(n, max_iter);

    for k = 1:max_iter
        % EXERCISE (a): Compute the search direction.
        if strcmpi(method, 'SD')
            % Steepest Descent direction (negative gradient)
            d(:,k) = -grad_f(x(:,k));  % <-- SD direction (part a)
        elseif strcmpi(method, 'NR')
            % Newton-Raphson direction
            d(:,k) = -Hf(x(:,k)) \ grad_f(x(:,k)); % <-- NR direction (part a)
        else
            error('Unknown method. Use ''SD'' or ''NR''.');
        end

        % Step size determination (part a, and for use in part b and c)
        switch stepsize_rule
            case 1  % Golden Section Search
                % Example: a = x(:,k), b = x(:,k) + d(:,k)
                a_gss = x(:,k); b_gss = x(:,k) + 1*d(:,k); tol_gss = 1e-6; max_iter_gss = 100;  
                % GSS must minimize over alpha, so typically you write an anonymous function
                % of alpha as f_alpha = @(alpha) f(x(:,k) + alpha*d(:,k))
                f_alpha = @(alpha) f(x(:,k) + alpha*d(:,k));
                [alpha_gss, ~, ~] = GSS(f_alpha, 0, 1, tol_gss, max_iter_gss);
                alpha(k) = alpha_gss;
            case 2  % Backtracking Line Search
                x_bls = x(:,k); max_iter_bls = 100; alpha_bls = 0.2; beta_bls = 0.8; d_bls = d(:,k);
                alpha(k) = BLS(f, grad_f, x_bls, max_iter_bls, alpha_bls, beta_bls, d_bls);
            case 3  % Constant step size
                alpha(k) = 0.05;
            otherwise
                error('Invalid step size method. Choose 1, 2, or 3.');
        end

        % EXERCISE (a): Update the iterate.
        x(:,k+1) = x(:,k) + alpha(k)*d(:,k);  % (part a)

        % EXERCISE (a): Stopping criteria (check norm of step or gradient)
        if norm(alpha(k)*d(:,k)) < tol  % Typical stopping criterion (part a)
            fprintf('Convergence reached at iteration %d.\n', k);
            break;
        end
    end

    xmin = x(:,k+1);
    fmin = f(xmin);
    iter = k;

    fprintf('Minimum found at x = [');
    fprintf('%.4f ', xmin);
    fprintf('] with f(x) = %.4f\n', fmin);
    fprintf('Total iterations: %d\n', iter);
end
