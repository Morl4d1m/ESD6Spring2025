%Real robot postion
xr = [1;1];
%Beacons' positions:
b1 = [2;0];
b2 = [0;2];
b3 = [-2;-2];
%Noisy measured distances:
d1 = norm(xr-b1)+0.01*randn;
d2 = norm(xr-b2)+0.01*randn;
d3 = norm(xr-b3)+0.01*randn;

F = @(x) [norm(x-b1)^2-d1^2;norm(x-b2)^2-d2^2;norm(x-b3)^2-d3^2];

Jf = @(x) [
    2*(x(1)-b1(1)), 2*(x(2)-b1(2));
    2*(x(1)-b2(1)), 2*(x(2)-b2(2));
    2*(x(1)-b3(1)), 2*(x(2)-b3(2))
];

max_iter = 100;
tol = 1e-8;
stepsize_rule = 1;
x0 = [0;0];
[xmin, fmin, x, iter] = Gauss_Newton(F, Jf, x0, max_iter, tol, stepsize_rule);
error = norm(xmin-xr);
fprintf('Distance Error =  %.4f\n', error);