function [ output_args ] = zadanie2_( input_args )
%ZADANIE2_ Summary of this function goes here
%   Detailed explanation goes here
%   Funkcia, ktoru chceme aproximovat pomocou siete MLP: Funckia c.16, y=x^(1/3)

x = linspace(-10, 10, 200);
% 1. Funkcny predpis funkcie f(x)
y = nthroot(x, 3);
countOfHiddenNeurons = 2:5:30; %Pocet neuronovej siete. Ulohou je zistit idealny pocet neuronov v skrytej vrstve pre co  najlepsiu aproximaciu funkcie
trainRatio = 0.8; % 80% tvori trenovacia mnozina
valRatio = 0.1; % 10% tvori validacna mnozina
testRatio = 0.1; % 10% tvori testovacia mnozina

% Obrazok do dokumentacie
%f = figure;
%plot(x, y, 'LineWidth', 2);
%axis([-10 10 -10 10]);
%grid on;
%xlabel('x');
%ylabel('x\^(1/3)');
%title('Priebeh funckie, ktora sa bude aproximovat');
%saveas(f, 'priebeh.png');

% 2. Vytvorim si MLP siet, parametrom funckie fitnet je pocet neuronov v skrytej vrstve
% menim aktivacnu funkciu 'transferFcn'
MLP_network = fitnet(countOfHiddenNeurons(1), 'trainlm'); %default
% MLP_network = fitnet(countOfHiddenNeurons, 'trainbr');
% MLP_network = fitnet(countOfHiddenNeurons, 'trainscg');
% MLP_network = fitnet(countOfHiddenNeurons, 'trainrp');
% MLP_network = fitnet(countOfHiddenNeurons, 'logsig');

% http://www.mathworks.com/help/nnet/ug/divide-data-for-optimal-neural-network-training.html
% 3. Cross validacia - rozdelenie dat
MLP_network.divideFcn = 'dividerand'; % hodnoty vyberam pseudonahodnym sposobom do jednotlivych vzoriek
%MLP_network.divideFcn = 'divideint'; % test
MLP_network.divideParam.trainRatio = trainRatio;
MLP_network.divideParam.valRatio = valRatio; % Na validacnej mnozine sa pocas trenovania kontroluje, ci siet nie je "pretrenovana".
MLP_network.divideParam.testRatio = testRatio;

% 4. Trenovanie siete na vstupno/vystupnych datach
% natrenovana siet s upravenymi vahami
[MLP_network, train_record] = train(MLP_network, x, y);

% 5. Testovanie siete na neznamych udajoch
outputs = MLP_network(x(train_record.testInd)); %testovacia mnozina
%fprintf('%f', perform(MLP_network, y(train_record.testInd), outputs)); % stredna kvadraticka chyba siete
%view(MLP_network);

% 6. Vykreslenie grafu pre overenie spravnosti pouzitej funkcie
f2 = figure;
plot(x, y, 'k-', 'LineWidth', 2);
hold on;
plot(x(train_record.testInd), outputs, 'r*-'); % testovacie vstupy, vystupy z neuronovej siete
title('Priebeh funkcie x\^(1/3)');
axis([-10, 10, -10, 10]);
grid on;
%saveas(f2, 'aproximacia_nastavenie3.png');

% 7. Graf vyvoja chyby na trenovacej a validacnej mnozine pocas trenovania siete
f3 = figure
plotperform(train_record); % vykreslenie trenovacej, validacnej chyby, zastavenie
%saveas(f3, 'chyba_vyvoja_nastavenie3.png');

end