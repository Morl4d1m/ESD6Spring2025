% Backtracking line search algorithm
function t = BLS(f, grad_f, x, max_iter, alpha, beta, d)
    % Input:
    % f - cost function handle
    % grad_f - gradient of the cost function handle
    % x - initial point
    % max_iter - maximum number of iterations
    % alpha - Armijo constant (0, 0.5)
    % beta - step size reduction factor (0, 1)
    % d - search direction (vector)

    % Initialization
    t = 1; % initial step size
    iter = 0; % iteration count

    % Main loop
    while f(x + t * d) > f(x) + alpha * t * grad_f(x)' * d && iter < max_iter
        t = beta * t;
        iter = iter + 1;
    end

    % Output
    % t - step size

     % Display results
    fprintf('Backtracking line search Results:\n');
    fprintf('Iterations: %d\n', iter);
    fprintf('Minimum point: x = %.6f\n', x + t * d);
    fprintf('Function value at minimum: f(x) = %.6f\n', f(x + t * d));

end
