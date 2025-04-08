cvx_begin
    variables x1 x2
    minimize( (x1 + 2)^2 + (x2 + 4)^2 ) % Minimize squared Euclidean distance
    subject to
        x1^2 + (x2 + 4)^2 <= 16  % Circle constraint
        x1 - x2 >= 6             % Half-plane constraint
        sqrt(x1^2 + (x2 + 6)^2) >= sqrt(2)   % New circular constraint (c3)
cvx_end

optimal_location = [x1, x2];
disp('Optimal Receiver Location (Max Signal Power):');
disp(optimal_location);

if strcmp(cvx_status, 'Infeasible')
    disp('No feasible solution exists with the added constraint c3(x).');
end

%cvx_begin
%    variables x1_worst x2_worst
%    maximize( sqrt((x1_worst - xt(1))^2 + (x2_worst - xt(2))^2) ) % Maximize distance
%    subject to
%        -x1_worst^2 - (x2_worst + 4)^2 + 16 <= 0;  % Circular constraint (c1)
%        x1_worst - x2_worst - 6 >= 0;              % Linear constraint (c2)
%cvx_end

%worst_location = [x1_worst, x2_worst];
%disp('Worst Receiver Location (Min Signal Power):');
%disp(worst_location);

% Plot the feasible region and optimal solution
figure(1);
hold on;
axis equal;

% Plot the circle
theta = linspace(0, 2*pi, 100);
circle_x = -2 + 4*cos(theta);
circle_y = -4 + 4*sin(theta);
plot(circle_x, circle_y, 'b', 'LineWidth', 2);

% Plot the half-plane boundary
x_vals = linspace(-6, 6, 100);
y_vals = x_vals - 6;
plot(x_vals, y_vals, 'r', 'LineWidth', 2);

% Plot the small circle
theta = linspace(0, 2*pi, 100);
circle_x_small =  2/sqrt(2)*cos(theta);
circle_y_small = -6 + 2/sqrt(2)*sin(theta);
plot(circle_x_small, circle_y_small, 'b', 'LineWidth', 2);

% Shade the feasible region (intersection of circle and half-plane)
[X, Y] = meshgrid(linspace(-6, 6, 200), linspace(-8, 4, 200));
feasible = (X.^2 + (Y+4).^2 <= 16) & (X - Y >= 6) & (X.^2 + (Y + 6).^2 - 2>=0);
hold on;
contourf(X, Y, feasible, [0.5 0.5], 'g', 'FaceAlpha', 0.3);

% Plot the transmitter location
plot(-2, -4, 'ro', 'MarkerFaceColor', 'r', 'MarkerSize', 8);
text(-2.3, -4.3, 'Transmitter', 'Color', 'r');

% Plot the optimal receiver location
plot(x1, x2, 'go', 'MarkerFaceColor', 'g', 'MarkerSize', 8);
text(x1 + 0.3, x2 + 0.3, 'Optimal Receiver', 'Color', 'g');

xlabel('x_1');
ylabel('x_2');
title('Optimal Receiver Placement');
legend('Circle Constraint', 'Half-Plane Constraint', 'Feasible Region', 'Location', 'Best');
grid on;
hold off;
