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
% Instructions for Students:
%   For this function to work, you will need to complete some missing
%   parts:
%       a) The direction for SD and NF.
%       b) Write the tolerance condition to terminate the optimization.

    % Check for missing optional inputs and set defaults if needed.
    if nargin < 8, stepsize_rule = 3; end  % Default to constant step size.
    if nargin < 7, method = 'SD'; end       % Default to Steepest Descent.

    % Initialization
    n = length(x0);
    x = zeros(n, max_iter+1);
    x(:,1) = x0;
    alpha = zeros(1, max_iter);   % To store step sizes
    d = zeros(n, max_iter);       % To store search directions

    % Main Optimization Loop
    for k = 1:max_iter
        % --- Compute the Search Direction ---
        if strcmpi(method, 'SD')
            % Steepest Descent direction: negative gradient
            d(:,k) = %fill the missing part here.;
        elseif strcmpi(method, 'NR')
            % Newton-Raphson direction: 
            d(:,k) = %fill the missing part here.
        else
            error('Unknown method. Choose ''SD'' (Steepest Descent) or ''NR'' (Newton-Raphson).');
        end

        % --- Determine the Step Size ---
        switch stepsize_rule
            case 1  % Golden Section Search (GSS)

                a_gss=x(:,k); b_gss=x(:,k)+1*d(:,k); tol_gss=1e-6; max_iter_gss = 100;  
                [xmin_gss, fmin_gss, iter_gss]=GSS(f, a_gss, b_gss, tol_gss, max_iter_gss);
                alpha(k)=norm(xmin_gss-x(:,k))/norm(d(:,k));
            case 2  % Backtracking Line Search (BLS)

                x_bls=x(:,k); max_iter_bls=100; alpha_bls=.2; beta_bls=.8; d_bls=d(:,k);
                alpha(k)=BLS(f, grad_f, x_bls, max_iter_bls, alpha_bls, beta_bls, d_bls);
                
            case 3  % Constant step size
                alpha(k) = 0.05;
                
            otherwise
                error('Invalid step size method selected. Choose an integer between 1 and 5.');
        end

        % --- Update the Current Point ---
        x(:, k+1) = % fill the missing part!

        % --- Check Convergence ---

        if %Fill the missing part (what should the tolerance condition be? check your slides
            fprintf('Convergence reached at iteration %d.\n', k);
            break;
        end
    end

    % Set outputs
    xmin = x(:, k+1);
    fmin = f(xmin);
    iter = k;
    
    % Optionally, you can display the results:
    fprintf('Minimum found at x = [');
    fprintf('%.4f ', xmin);
    fprintf('] with f(x) = %.4f\n', fmin);
    fprintf('Total iterations: %d\n', iter);
end