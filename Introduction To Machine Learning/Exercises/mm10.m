close all
clear
% This code implements the logistic regression model in line with the lecture.
% Model: p(y=1|x,w) = sigma(w^T phi(x)) = 1/(1+exp(-w^T phi(x)))
% Compact form for data: sigma(Phi(x)w). 
% where phi(x) = [1; x_1; x_2; ...; x_d] includes the bias term.
% The weights w are learned via gradient descent on the cross-entropy loss.
% Inputs (assumed defined from previous code of the previous exercise):
%   X_train, y_train: Training features and binary labels (y in {0,1})
%   X_test, y_test:   Testing features and labels

% Prepare the Data
data = readtable('Breast_Cancer.csv');
rng(1234)
% Separate features and labels (Second coloumn containts the labels)
X = data{:, 3:end};  % feature matrix
y = data{:, 2};      % class labels

% If labels are not numeric, convert them.
if ~isnumeric(y)
    y = grp2idx(y)-1; % Convert categorical labels to numeric indices.
    % 0: "M" (Malignant)
    % 1: "B" (Benign)
end
cv = cvpartition(y, 'HoldOut', 0.3);
X_train = X(training(cv), :);
y_train = y(training(cv));
X_test  = X(test(cv), :);
y_test  = y(test(cv));


classes = unique(y_train);
numClasses = length(classes);
[numTrain, numFeatures] = size(X_train);


% Add the intercept (bias) term to our feature vectors:
Phi_train = [ones(size(X_train, 1), 1) X_train];  % phi(x) for training data
Phi_test  = [ones(size(X_test, 1), 1) X_test];    % phi(x) for test data


% Get dimensions: m training examples, d+1 features (including bias)
[m, d_plus1] = size(Phi_train);

% Option 1: Batch Gradient Descent (default)
w = zeros(d_plus1, 1);      % Initialize weights
eta = 0.1;                  % Learning rate
numIter = 100000;           % Maximum number of iterations
tol = 1e-6;                 % Convergence tolerance

% Preallocate vector to store loss history
loss_history = zeros(numIter, 1);

for iter = 1:numIter
    % Compute the linear model: 
    a = Phi_train * w;%FILL MISSING PART HERE

    % Apply the sigmoid function: sigma(a)
    sigma_a = 1 ./ (1 + exp(-a)); %FILL MISSING PART HERE

    % Compute the cross-entropy loss (negative log-likelihood)
    % Adding a small epsilon for numerical stability.
    eps_val = 1e-12;
    loss_history(iter) = -sum( y_train .* log(sigma_a + eps_val) + ...
                               (1-y_train) .* log(1 - sigma_a + eps_val) );

    % Compute the gradient of the -log-likelihood
    gradient = Phi_train' * (sigma_a - y_train);

    % Update weights
    w = w - eta * gradient;

    % Check convergence: if the norm of the update is smaller than tol, exit loop
    if norm(eta * gradient) < tol
        fprintf('Batch GD: Convergence reached at iteration %d.\n', iter);
        break;
    end
end

% Trim loss_history to the actual number of iterations performed
loss_history = loss_history(1:iter);

% Plot the Loss History for Batch Gradient Descent
figure;
plot(1:iter, loss_history, 'LineWidth', 2);
xlabel('Iteration');
ylabel('Cross-Entropy Loss');
title('Training Loss History (Batch Gradient Descent)');
grid on;

% Prediction on Test Set
a_test = Phi_test * w; % Compute the linear model for test data
sigma_test = 1 ./ (1 + exp(-a_test)); %FILL MISSING CODE HERE      % Apply the sigmoid

y_pred_logistic = sigma_test >= 0.5;%FILL MISSING CODE HERE % Predict. (you can use the prior too from the early exercise or just use the rule from the slides). 
fuck = double(y_pred_logistic);
% Evaluate the Logistic Regression Classifier
cmLogRegManual = confusionmat(y_test, fuck);
accuracyLogRegManual = sum(diag(cmLogRegManual)) / sum(cmLogRegManual(:));
fprintf('Logistic Regression Accuracy: %.2f%%\n', accuracyLogRegManual * 100);

% Display the confusion matrix as a heat map
figure;
confusionchart(cmLogRegManual);
title('Confusion Matrix - Logistic Regression');

% % Option 2: Stochastic Gradient Descent (SGD)
% % To experiment with SGD, comment out Option 1 above and uncomment the block below.
% 
% w = zeros(d_plus1, 1);      % Initialize weights
% eta = 0.001;                  % Learning rate
% numEpochs = 1000;            % Number of epochs
% loss_history_sgd = zeros(numEpochs, 1);
% 
% for epoch = 1:numEpochs
%     %Shuffle the training data each epoch
%     idx = randperm(m);
%     Phi_train_shuffled = Phi_train(idx, :);
%     y_train_shuffled = y_train(idx);
% 
%     for i = 1:m
%         %Compute the linear model for the i-th sample
%         a_i = #FILL MISSING PART HERE;
%         %Apply the sigmoid function
%         sigma_i = #FILL MISSING PART HERE
%         %Compute the gradient for the i-th sample
%         gradient_i = #FILL MISSING PART HERE
%         %Update weights immediately (stochastic update)
%         w = #FILL MISSING PART HERE
%     end
%     %Compute loss over the entire training set at the end of the epoch
%     a = #FILL MISSING PART HERE 
%     sigma_a = #FILL MISSING PART HERE
%     loss_history_sgd(epoch) = -sum( y_train .* log(sigma_a + eps_val) + ...
%                                    (1-y_train) .* log(1 - sigma_a + eps_val) );
%     %fprintf('Epoch %d, Loss: %.4f\n', epoch, loss_history_sgd(epoch));
% end