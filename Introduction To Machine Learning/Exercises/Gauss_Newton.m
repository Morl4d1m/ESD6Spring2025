function [xmin, fmin, x, iter] = Gauss_Newton(F, Jf, x0, max_iter, tol, stepsize_rule)
% Gauss-Newton  
%
%   [xmin, fmin, x, iter] = Gauss_Newton(F, J, x0, max_iter, tol, stepsize_rule)
%
%   Input arguments:
%     F             - Handle to F in F(x)=0 (the equations we want to find
%     their zeros).
%     Jf            - The jacobian to f (remember the gradient and hessian
%     can be computed in this case from F and Jf (check the slides).
%     x0            - Initial guess (column vector).
%     max_iter      - Maximum number of iterations.
%     tol           - Tolerance for stopping criterion.
%     stepsize_rule - Integer (1 to 3) selecting the step size rule:
%                     1: Golden Section Search (requires separate function 'GSS')
%                     2: Backtracking Line Search (requires separate function 'BLS')
%                     3: Constant step size (default)
%
%   Output arguments:
%     xmin  - The computed minimizer (approximate solution).
%     fmin  - The cost function value at xmin. The cost can also be found
%     from F (check the slides).
%     x     - The trajectory of the optimization.
%     iter  - The number of iterations performed.
% Instructions for Students:
%   For this function to work, you will need to complete some missing
%   parts:
%       a) The gradient, and hessian computation from Jf.
%       b) The direction d(:,k);
%       c) The tolerance condition to terminate the optimization

    % Check for missing optional inputs and set defaults if needed.
    if nargin < 6, stepsize_rule = 3; end  % Default to constant step size.

    % Initialization
    n = length(x0);
    x = zeros(n, max_iter+1);
    x(:,1) = x0;
    alpha = zeros(1, max_iter);   % To store step sizes
    d = zeros(n, max_iter);       % To store search directions

    %Defin the cost function, gradient, and hessian:
    f = @(x) sum(F(x).^2); %Sum of squares. 

    grad_f = @(x) 2*Jf(x)'*F(x);

    Hf = @(x) 2*Jf(x)'*Jf(x);

    % Main Optimization Loop
    for k = 1:max_iter
            d(:,k) = - (Jf(x(:,k))' * Jf(x(:,k))) \ (Jf(x(:,k))' * F(x(:,k)));
            % ... step size as given ...
            x(:, k+1) = x(:,k) + alpha(k) * d(:,k);
        switch stepsize_rule
            case 1  % Golden Section Search (GSS)
                a_gss=x(:,k); b_gss=x(:,k)+1*d(:,k); tol_gss=1e-6; max_iter_gss = 100;  
                [xmin_gss, fmin_gss, iter_gss]=GSS(f, a_gss, b_gss, tol_gss, max_iter_gss);
                alpha(k)=norm(xmin_gss-x(:,k))/norm(d(:,k));
            case 2  % Backtracking Line Search (BLS)
                x_bls=x(:,k); max_iter_bls=100; alpha_bls=.2; beta_bls=.8; d_bls=d(:,k);
                alpha(k)=BLS(f, grad_f, x_bls, max_iter_bls, alpha_bls, beta_bls, d_bls);
            case 3  % Constant step size
                alpha(k) = 0.01;
                
            otherwise
                error('Invalid step size method selected. Choose an integer between 1 and 5.');
        end

        % --- Update the Current Point ---
        x(:, k+1) = x(:,k) + alpha(k) * d(:,k);

       % Convergence check (choose one)
    if norm(d(:,k)) < tol % or norm(grad_f(x(:,k))) < tol
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