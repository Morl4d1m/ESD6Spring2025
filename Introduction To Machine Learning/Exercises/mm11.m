% iris_mlp_nn.m
% Neural Network Classification of the Iris Dataset using MATLAB

% 1. Load and Preprocess Data
clear; clc; close all;

% Load the Iris dataset (Fisher's Iris)
load fisheriris % meas: 150x4, species: 150x1 cell array

X = meas; % Features (150x4)
Y = species; % Labels (cell array of strings)

% Convert string labels to categorical
Y_categorical = categorical(Y);

% Normalize features to zero mean, unit variance
X = (X - mean(X)) ./ std(X);

% Stratified split: 70% train, 30% test
cv = cvpartition(Y_categorical, 'HoldOut', 0.3);
XTrain = X(training(cv),:);
YTrain = Y_categorical(training(cv),:);
XTest  = X(test(cv),:);
YTest  = Y_categorical(test(cv),:);

% 2. Define Neural Network Architecture
inputSize = size(X,2);      % 4 features
numHidden = 10;             % 10 hidden units
numClasses = numel(categories(Y_categorical)); % 3 classes

layers = [
    featureInputLayer(inputSize, 'Name', 'input')
    fullyConnectedLayer(numHidden, 'Name', 'fc1')
    reluLayer('Name', 'relu1')
    fullyConnectedLayer(numClasses, 'Name', 'fc2')
    softmaxLayer('Name', 'softmax')
    classificationLayer('Name', 'output')
];

% 3. Set Training Options
options = trainingOptions('adam', ...
    'MaxEpochs',10000, ...
    'InitialLearnRate',0.01, ...
    'MiniBatchSize',16, ...
    'Shuffle','every-epoch', ...
    'L2Regularization', 0.01, ...
    'Plots','training-progress', ...
    'Verbose',false);

% 4. Train the Network
net = trainNetwork(XTrain, YTrain, layers, options);

% 5. Evaluate on Test Set
YPred = classify(net, XTest);

accuracy = mean(YPred == YTest) * 100;
fprintf('\nTest set accuracy: %.2f%%\n', accuracy);

% Confusion Matrix
figure;
confusionchart(YTest, YPred);
title('Confusion Matrix on Test Set');

% Optionally, display some misclassified examples (if any)
idx = find(YPred ~= YTest);
if ~isempty(idx)
    fprintf('\nMisclassified examples (showing up to 5):\n');
    for k = 1:min(5, numel(idx))
        fprintf('Sample %d: True = %s, Predicted = %s\n', ...
            idx(k), string(YTest(idx(k))), string(YPred(idx(k))));
    end
else
    fprintf('No misclassifications on the test set!\n');
end
