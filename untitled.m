clc;
clear;
close all;

tamanio = 1000;

% Inicializamos los ángulos para cada motor
theta1 = zeros(tamanio, 1);  % Ángulos del motor 1
theta2 = linspace(0, pi, tamanio);  % Ángulos del motor 2
theta3 = linspace(0, pi, tamanio);  % Ángulos del motor 3

% Inicializar matrices para almacenar las posiciones y orientaciones
p1 = zeros(3, tamanio);
p2 = zeros(3, tamanio);
p3 = zeros(3, tamanio);
r1 = zeros(3, tamanio);
r2 = zeros(3, tamanio);
r3 = zeros(3, tamanio);

% Ciclo for para calcular las posiciones y orientaciones del robot
for i = 1:tamanio
    % Llamamos a la función para calcular las posiciones y orientaciones del robot
    [p1(:,i), p2(:,i), p3(:,i), r1(:,i), r2(:,i), r3(:,i)] = mover_clinostato(theta1(i), theta2(i), theta3(i));
    graficar_clinostato(p1, p2, p3, r1, r2, r3);
end

% Llamamos a la función para graficar los resultados

