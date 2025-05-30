% Definér matricen A
A = [6 2 -2;
     2 5  0;
    -2 0  7];

% Beregn egenværdier og egenvektorer
[V, D] = eig(A);

% Udskriv egenværdier
disp('Egenværdier:');
disp(diag(D));

% Udskriv egenvektorer (hver søjle i V er en egenvektor)
disp('Egenvektorer (hver søjle er en egenvektor):');
disp(V);

% (Ekstra) Hvis du vil vise dem parvis
for i = 1:size(V,2)
    fprintf('Egenvektor til egenværdi %.4f:\n', D(i,i));
    disp(V(:,i));
end