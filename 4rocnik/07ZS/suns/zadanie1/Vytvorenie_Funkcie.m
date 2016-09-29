clc;
clear;
close all

% Vytvorenie 1. aproximacnej funkcie

% Prvy krok je vytvorenie priamky
x = 0:0.15:3;   % Na x-ovej osi je 1. priamka v rozmedzi <0 3> a obsahuje 20 vzoriek
a = 0.21667;    
b = 0.3;        % a, b su parametre priamky, ktore treba vypocitat

y = a*x+b;      % rovnica 1. priamky

% Druhy krok, vytvorenie 2. priamky
x1 = 3:0.142:4; % V rozsaju <3 4> mame 2. priamku, ktora obsahuje 8 vzoriek

a1 = -0.95;
b1 = -4*a1;       % Opat parametre priamky, ktore treba vypocitat

y1 = a1*x1+b1;     % Rovnica 2. Priamky

% Spojenie jednotlivych casti funkcie
yz = [y y1(2:end) zeros(1,12)];         % Spojena 1. a 2. priamka, za ktore 
                                        % sme este pripojili 12 nul
xz = [x x1(2:end) 4.154:0.154:5.85];    % Spojenie x-ovych osi
                                        % Dolezita je hodnota 5.85, kde
                                        % koncia vsetky funkcie
% Vykreslenie danej funkcie                                        
figure
plot(xz, yz, '-o')
xlabel 'x'
ylabel 'f(x)'