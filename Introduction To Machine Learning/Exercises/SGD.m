clear
close all
clc
rng(42)
% Generate synthetic data
N = 50;                      % Number of data points
x_data = linspace(0, 10, N); % Input (x) values
true_theta0 = 1;             % True intercept
true_theta1 = 2;             % True slope
true_theta = [true_theta0; true_theta1];
y_data = true_theta1 * x_data + true_theta0 + randn(1, N); % Add noise

% Plot data
figure;
scatter(x_data, y_data, 50, 'b', 'filled');
hold on;
xlabel('x');
ylabel('y');
title('Data Points and Fitted Line');
grid on;

% Hyperparameters
eta = 0.01;            % Learning rate
num_epochs = 100;      % Number of epochs (previously max_iter)
batch_size = 1;        % Mini-batch size
num_batches = N / batch_size; % Ensure N is divisible by batch_size
epsilon = 1e-6;

% Initialize parameters (no shuffle)
theta0_no_shuffle = 0; 
theta1_no_shuffle = 0;
theta_history_no_shuffle = [theta0_no_shuffle; theta1_no_shuffle];

% SGD Without Shuffling (with mini-batches)
for epoch = 1:num_epochs
    for batch = 1:num_batches
        % Get mini-batch
        start_idx = (batch-1)*batch_size + 1;
        end_idx = batch*batch_size;
        x_batch = x_data(start_idx:end_idx);
        y_batch = y_data(start_idx:end_idx);
        
        % Compute batch gradients
        grad0 = 0; grad1 = 0;
        for k = 1:batch_size
            y_pred = theta1_no_shuffle * x_batch(k) + theta0_no_shuffle;
            error = y_batch(k) - y_pred;
            grad0 = grad0 - error;
            grad1 = grad1 - error * x_batch(k);
        end
        
        % Update parameters with average gradient
        theta0_no_shuffle = theta0_no_shuffle - eta*(grad0/batch_size);
        theta1_no_shuffle = theta1_no_shuffle - eta*(grad1/batch_size);
        
        % Store history
        theta_history_no_shuffle(:, end+1) = [theta0_no_shuffle; theta1_no_shuffle];
    end
end

% Initialize parameters (with shuffle)
theta0_shuffle = 0; 
theta1_shuffle = 0;
theta_history_shuffle = [theta0_shuffle; theta1_shuffle];

% SGD With Shuffling (with mini-batches)
for epoch = 1:num_epochs
    % Shuffle data each epoch
    idx = randperm(N);
    x_shuffled = x_data(idx);
    y_shuffled = y_data(idx);
    
    for batch = 1:num_batches
        % Get shuffled mini-batch
        start_idx = (batch-1)*batch_size + 1;
        end_idx = batch*batch_size;
        x_batch = x_shuffled(start_idx:end_idx);
        y_batch = y_shuffled(start_idx:end_idx);
        
        % Compute batch gradients
        grad0 = 0; grad1 = 0;
        for k = 1:batch_size
            y_pred = theta1_shuffle * x_batch(k) + theta0_shuffle;
            error = y_batch(k) - y_pred;
            grad0 = grad0 - error;
            grad1 = grad1 - error * x_batch(k);
        end
        
        % Update parameters
        theta0_shuffle = theta0_shuffle - eta*(grad0/batch_size);
        theta1_shuffle = theta1_shuffle - eta*(grad1/batch_size);
        
        % Store history
        theta_history_shuffle(:, end+1) = [theta0_shuffle; theta1_shuffle];
    end
end


theta_no_shuffle=[theta0_no_shuffle; theta1_no_shuffle];

ens=norm(true_theta-theta_no_shuffle);


theta_shuffle=[theta0_shuffle; theta1_shuffle];

es=norm(true_theta-theta_shuffle);

 
fprintf('Error with no shuffling %.4f\n', ens);
fprintf('Error with shuffling %.4f\n', es);

% Plot the fitted lines

y_fit_no_shuffle = theta1_no_shuffle * x_data + theta0_no_shuffle;

y_fit_shuffle = theta1_shuffle * x_data + theta0_shuffle;

 

plot(x_data, y_fit_no_shuffle, 'r-', 'LineWidth', 2); % No shuffle

plot(x_data, y_fit_shuffle, 'g--', 'LineWidth', 2); % Shuffle

legend('Data Points', 'No Shuffle Fit', 'Shuffle Fit', 'Location', 'Best', 'FontSize', 20);

 

% Plot parameter trajectories

figure;

subplot(1, 2, 1);

plot(theta_history_no_shuffle(1, :), theta_history_no_shuffle(2, :), 'o-', 'LineWidth', 1.5);

xlabel('\theta_0 (Intercept)');

ylabel('\theta_1 (Slope)');

title('Parameter Updates (No Shuffling)','FontSize', 20);

grid on;

 

subplot(1, 2, 2);

plot(theta_history_shuffle(1, :), theta_history_shuffle(2, :), 'o-', 'LineWidth', 1.5);

xlabel('\theta_0 (Intercept)');

ylabel('\theta_1 (Slope)');

title('Parameter Updates (With Shuffling)','FontSize', 20);

grid on;