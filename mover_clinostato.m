function [p1, p2, p3, r1, r2, r3, euler1, euler2, euler3] = mover_clinostato(theta1, theta2)
    % Parámetros D-H
    a1 = 0;       % Longitud del primer enlace (m)
    a2 = 5;       % Longitud del segundo enlace (m)
    a3 = -5;      % Longitud del tercer enlace (m)
    d1 = 0;       % Desplazamiento en z del primer enlace (m)
    d2 = 5;       % Desplazamiento en z del segundo enlace (m)
    d3 = 0;       % Desplazamiento en z del tercer enlace (m)
    alpha1 = 0;   % Ángulo de inclinación entre z0 y z1
    alpha2 = -90; % Ángulo de inclinación entre z1 y z2
    alpha3 = 90;  % Ángulo de inclinación entre z2 y z3

    % Calculando las matrices de transformación en cadena
    T1 = Matriz_TH(0, alpha1, d1, a1); % Transformación de la primera articulación
    T2 = T1 * Matriz_TH(theta1, alpha2, d2, a2); % Segunda articulación basada en la primera
    T3 = T2 * Matriz_TH(theta2, alpha3, d3, a3); % Tercera articulación basada en la segunda

    % Extrayendo posiciones
    p1 = T1(1:3, 4); % Posición de la articulación 1
    p2 = T2(1:3, 4); % Posición de la articulación 2
    p3 = T3(1:3, 4); % Posición de la articulación 3
    
    % Extrayendo los vectores de orientación (ejes Z)
    r1 = T1(1:3, 3); % Vector orientación en la articulación 1 (eje Z)
    r2 = T2(1:3, 3); % Vector orientación en la articulación 2 (eje Z)
    r3 = T3(1:3, 3); % Vector orientación en la articulación 3 (eje Z)
    
    % Calculando los ángulos de Euler (Roll, Pitch, Yaw) para cada matriz
    euler1 = rotm2eul(T1(1:3, 1:3), 'ZYX');
    euler2 = rotm2eul(T2(1:3, 1:3), 'ZYX');
    euler3 = rotm2eul(T3(1:3, 1:3), 'ZYX');
end
