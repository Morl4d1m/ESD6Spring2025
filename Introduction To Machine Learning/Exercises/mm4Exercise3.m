% --- Robot localization using Gauss-Newton ---

% 1. Problem setup
rng('default'); % For reproducibility
xr = [1;1]; % True position
b1 = [2;0];
b2 = [0;2];
b3 = [-2;-2];
d1 = norm(xr-b1)+0.01*randn;
d2 = norm(xr-b2)+0.01*randn;
d3 = norm(xr-b3)+0.01*randn;

F = @(x) [
    norm(x-b1)^2 - d1^2;
    norm(x-b2)^2 - d2^2;
    norm(x-b3)^2 - d3^2
];

Jf = @(x) [
    2*(x(1)-b1(1)), 2*(x(2)-b1(2));
    2*(x(1)-b2(1)), 2*(x(2)-b2(2));
    2*(x(1)-b3(1)), 2*(x(2)-b3(2))
];

% 2. Gauss-Newton implementation (inner function)
Gauss_Newton = @(F, Jf, x0, max_iter, tol, stepsize_rule) ...
    local_Gauss_Newton(F, Jf, x0, max_iter, tol, stepsize_rule);



% 3. Run Gauss-Newton
max_iter = 100;
tol = 1e-8;
stepsize_rule = 3;
x0 = [0;0];
[xmin, fmin, x, iter] = Gauss_Newton(F, Jf, x0, max_iter, tol, stepsize_rule);
error = norm(xmin-xr);
fprintf('Distance Error =  %.4f\n', error);

% 4. Plot
figure;
plot(b1(1), b1(2), 'ro', 'MarkerSize', 10, 'LineWidth', 2); hold on;
plot(b2(1), b2(2), 'go', 'MarkerSize', 10, 'LineWidth', 2);
plot(b3(1), b3(2), 'bo', 'MarkerSize', 10, 'LineWidth', 2);
plot(xr(1), xr(2), 'kx', 'MarkerSize', 12, 'LineWidth', 3);
plot(x(1,1:iter+1), x(2,1:iter+1), 'm.-', 'LineWidth', 2, 'MarkerSize', 15);
plot(xmin(1), xmin(2), 'cs', 'MarkerSize', 10, 'LineWidth', 2);
legend('Beacon 1', 'Beacon 2', 'Beacon 3', 'True Position', 'Trajectory', 'Estimate', 'Location', 'Best');
xlabel('x'); ylabel('y'); axis equal; grid on;
title('Robot Localization with Gauss-Newton');


function [xmin, fmin, x, iter] = local_Gauss_Newton(F, Jf, x0, max_iter, tol, stepsize_rule)
    if nargin < 6, stepsize_rule = 3; end
    n = length(x0);
    x = zeros(n, max_iter+1);
    x(:,1) = x0;
    alpha = zeros(1, max_iter);
    d = zeros(n, max_iter);
    f = @(x) sum(F(x).^2);
    grad_f = @(x) 2*Jf(x)'*F(x);
    for k = 1:max_iter
        d(:,k) = - (Jf(x(:,k))' * Jf(x(:,k))) \ (Jf(x(:,k))' * F(x(:,k)));
        alpha(k) = 1; % Use constant step size for simplicity
        x(:, k+1) = x(:,k) + alpha(k) * d(:,k);
        if norm(d(:,k)) < tol
            fprintf('Convergence reached at iteration %d.\n', k);
            break;
        end
    end
    xmin = x(:, k+1);
    fmin = f(xmin);
    iter = k;
    fprintf('Minimum found at x = [');
    fprintf('%.4f ', xmin);
    fprintf('] with f(x) = %.4f\n', fmin);
    fprintf('Total iterations: %d\n', iter);
end