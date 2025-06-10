
function [xmin, fmin, iter] = GSS(f, a, b, tol, max_iter)
% GOLDEN_SECTION_SEARCH Performs optimization using the Golden Section Search method
% 
% Inputs:
%   f        - Cost function (function handle)
%   a        - Lower interval bound
%   b        - Upper interval bound
%   tol      - Tolerance for stopping criterion
%   max_iter - Maximum number of iterations (optional)
%
% Outputs:
%   xmin - Approximate minimum point
%   fmin - Function value at the minimum
%   iter - Number of iterations performed

    % Golden ratio and reciprocal
    phi = (1 + sqrt(5)) / 2;  % Golden ratio (ϕ ≈ 1.618)
    psi = 2 - phi;            % Reciprocal (ψ ≈ 0.382)

    % Initialization
    x1 = a + psi * (b - a);
    x2 = b - psi * (b - a);
    f1 = f(x1);
    f2 = f(x2);
    iter = 0;

    % Iterative process
    while norm(b - a) > tol && iter < max_iter
        if f1 < f2
            b = x2;
            x2 = x1;
            f2 = f1;
            x1 = a + psi * (b - a);
            f1 = f(x1);
        else
            a = x1;
            x1 = x2;
            f1 = f2;
            x2 = b - psi * (b - a);
            f2 = f(x2);
        end
        iter = iter + 1;
    end

    % Compute final minimum
    xmin = (a + b) / 2;
    fmin = f(xmin);

    % Display results
    fprintf('Golden Section Search Results:\n');
    fprintf('Iterations: %d\n', iter);
    fprintf('Minimum point: x = %.6f\n', xmin);
    fprintf('Function value at minimum: f(x) = %.6f\n', fmin);

end