clc; clear; close all;

% Define transmitter location
xt = [-2, -4];

% Find optimal receiver location (best signal)
cvx_begin
    variables x1 x2
    minimize( (x1 - xt(1))^2 + (x2 - xt(2))^2 ) % Minimize squared distance
    subject to
        x1^2 + (x2 + 4)^2 <= 16;   % Circular constraint (c1)
        x1 - x2 - 6 >= 0;           % Linear constraint (c2)
cvx_end

optimal_location = [x1, x2];
disp('Optimal Receiver Location (Max Signal Power):');
disp(optimal_location);

% Find worst receiver location (weakest signal)
%cvx_begin
%    variables x1_worst x2_worst d2_worst
%    maximize( d2_worst ) % Maximize squared distance
%    subject to
%        d2_worst <= (x1_worst - xt(1))^2 + (x2_worst - xt(2))^2; % Reformulated distance constraint
%        x1_worst^2 + (x2_worst + 4)^2 <= 16;   % Circular constraint (c1)
%        x1_worst - x2_worst - 6 >= 0;           % Linear constraint (c2)
%cvx_end

%worst_location = [x1_worst, x2_worst];
%disp('Worst Receiver Location (Min Signal Power):');
%disp(worst_location);

% Plot results
%figure; hold on; axis equal; grid on;
%xlabel('x_1'); ylabel('x_2'); title('Optimal and Worst Receiver Locations');

% Define grid for visualization
[x1_grid, x2_grid] = meshgrid(-6:0.01:6, -10:0.01:0);
c1 = x1_grid.^2 + (x2_grid + 4).^2 - 16;  % Circular constraint
c2 = -(x1_grid - x2_grid - 6);            % Linear constraint
feasible_region = (c1 <= 0) & (c2 <= 0);

% Plot feasible region
contourf(x1_grid, x2_grid, feasible_region, [0.5 0.5], 'g', 'LineWidth', 2);
colormap([0.8 1 0.8]); % Light green for feasible region

% Compute signal power contours (starting from transmitter)
d2 = (x1_grid - xt(1)).^2 + (x2_grid - xt(2)).^2;
P = 1 ./ d2; % Signal power inversely proportional to squared distance
contour(x1_grid, x2_grid, P, 20, 'k', 'LineWidth', 1.2); % Power contours

% Plot transmitter, optimal, and worst locations
plot(xt(1), xt(2), 'ro', 'MarkerSize', 10, 'MarkerFaceColor', 'r'); % Transmitter
plot(optimal_location(1), optimal_location(2), 'bo', 'MarkerSize', 10, 'MarkerFaceColor', 'b'); % Best
plot(worst_location(1), worst_location(2), 'mo', 'MarkerSize', 10, 'MarkerFaceColor', 'm'); % Worst

% Connect transmitter to receiver locations
plot([xt(1), optimal_location(1)], [xt(2), optimal_location(2)], 'b--', 'LineWidth', 2);
plot([xt(1), worst_location(1)], [xt(2), worst_location(2)], 'm-', 'LineWidth', 2);

% Add legend
legend({'Feasible Region', 'Transmitter', 'Best Location', 'Worst Location', ...
        'Best Path', 'Worst Path'}, 'Location', 'Best');
hold off;
c3 = x1.^2 + (x2 + 6).^2 - 2; % New constraint: Must be >= 0

% Objective functions
fun_min = @(x) (x(1) - xt(1))^2 + (x(2) - xt(2))^2; % Minimize distance
fun_max = @(x) -((x(1) - xt(1))^2 + (x(2) - xt(2))^2); % Maximize distance

% Solve for shortest and longest distance with new constraints
x_shortest_new = fmincon(fun_min, x0, [], [], [], [], [], [], @constraints_new, options);
x_longest_new = fmincon(fun_max, x0, [], [], [], [], [], [], @constraints_new, options);

% Plot new feasible region
figure; hold on; axis equal;
contourf(x1, x2, (c1 <= 0) & (c2 <= 0) & (c3 >= 0), [0.5 0.5], 'g', 'LineWidth', 2);
xlabel('x_1'); ylabel('x_2'); title('New Feasible Region with Constraint c3');
colormap([0.8 1 0.8]); % Light green for feasible region
grid on;

% Plot transmitter and receiver locations
plot(xt(1), xt(2), 'ro', 'MarkerSize', 10, 'MarkerFaceColor', 'r'); % Transmitter
plot(x_shortest_new(1), x_shortest_new(2), 'bo', 'MarkerSize', 10, 'MarkerFaceColor', 'b'); % Shortest Distance
plot(x_longest_new(1), x_longest_new(2), 'mo', 'MarkerSize', 10, 'MarkerFaceColor', 'm'); % Longest Distance

% Draw shortest and longest distance lines
plot([xt(1), x_shortest_new(1)], [xt(2), x_shortest_new(2)], 'k--', 'LineWidth', 2); % Shortest path
plot([xt(1), x_longest_new(1)], [xt(2), x_longest_new(2)], 'r-', 'LineWidth', 2); % Longest path

% Add legend
legend({'Feasible Region', 'Transmitter', 'Shortest Distance', 'Longest Distance', ...
        'Shortest Path', 'Longest Path'}, 'Location', 'Best');

figure(3);
x = linspace(pi,pi);
y = linspace(pi,pi);
[X,Y] = meshgrid(x,y);
Z = sin(X)+cos(Y);
contour(X,Y,Z)