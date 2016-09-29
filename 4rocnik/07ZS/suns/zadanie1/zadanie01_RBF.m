function [ output_args ] = zadanie2_( input_args )
%ZADANIE2_ Summary of this function goes here
%   Detailed explanation goes here
%   Funkcia, ktoru chceme aproximovat pomocou siete RBF: Funckia c.16, y=x^(1/3)

x = linspace(-10, 10, 100);
% 1. Funkcny predpis funkcie f(x)
y = nthroot(x, 3);
goal = [0.1, 0.01, 0.001, 0.0005, 0.0001]; % Mean squared error goal, default = 0.0.
xTest = linspace(-9.5, 9.5, 20);
% intersect(x, xTest) % kontrolujem prienik vektorov

% Obrazok do dokumentacie
%f = figure;
%plot(x, y, 'LineWidth', 2);
%axis([-10 10 -10 10]);
%grid on;
%xlabel('x');
%ylabel('x\^(1/3)');
%title('Priebeh funckie, ktora sa bude aproximovat');
%saveas(f, 'priebeh.png');

% 2. Vytvorim si RBF siet, nastavim strednu kvadraticku chybu (siet sa bude dovtedy trenovat kym nedosiahne tuto chybu)
% 3. a zaroven zistujem pocet neuronov po natrenovani
RBF_network = newrb(x, y, goal(5));

% 4. Testovanie siete na lubovolnej vzorke
result = RBF_network(xTest); % result = sim(RBF_network, xTest); % to iste

% 5. Vykreslenie grafu pre overenie spravnosti pouzitej funkcie
f = figure;
plot(x, y, 'k-', 'LineWidth', 2);
hold on;
plot(xTest, result, 'r*-');
title('Priebeh funkcie x\^(1/3)');
axis([-10, 10, -10, 10]);
grid on;
saveas(f, 'aproximacia_nastavenie6.png');


end