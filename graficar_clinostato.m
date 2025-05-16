function graficar_clinostato(p1, p2, p3, r1, r2, r3)
    % Graficando el movimiento en 3D
    figure;
    hold on;
    grid on;
    axis equal;
    xlabel('X');
    ylabel('Y');
    zlabel('Z');
    title('Movimiento de los 3 puntos en 3D');
    
    % Graficar los puntos de las posiciones
    plot3(p1(1,:), p1(2,:), p1(3,:), 'r', 'LineWidth', 2); % Punto 1
    plot3(p2(1,:), p2(2,:), p2(3,:), 'g', 'LineWidth', 2); % Punto 2
    plot3(p3(1,:), p3(2,:), p3(3,:), 'b', 'LineWidth', 2); % Punto 3

    % Marcando puntos iniciales y finales
    scatter3(p1(1,1), p1(2,1), p1(3,1), 'ro', 'filled');
    scatter3(p2(1,1), p2(2,1), p2(3,1), 'go', 'filled');
    scatter3(p3(1,1), p3(2,1), p3(3,1), 'bo', 'filled');
    scatter3(p1(1,end), p1(2,end), p1(3,end), 'rx');
    scatter3(p2(1,end), p2(2,end), p2(3,end), 'gx');
    scatter3(p3(1,end), p3(2,end), p3(3,end), 'bx');

    % Dibujar los vectores de orientación (ejes Z)
    quiver3(p1(1,:), p1(2,:), p1(3,:), r1(1,:), r1(2,:), r1(3,:), 0.5, 'r', 'LineWidth', 2); % Eje Z en la articulación 1
    quiver3(p2(1,:), p2(2,:), p2(3,:), r2(1,:), r2(2,:), r2(3,:), 0.5, 'g', 'LineWidth', 2); % Eje Z en la articulación 2
    quiver3(p3(1,:), p3(2,:), p3(3,:), r3(1,:), r3(2,:), r3(3,:), 0.5, 'b', 'LineWidth', 2); % Eje Z en la articulación 3

    legend('Punto 1', 'Punto 2', 'Punto 3', 'Inicio 1', 'Inicio 2', 'Inicio 3', 'Final 1', 'Final 2', 'Final 3');
    view(3);
end
