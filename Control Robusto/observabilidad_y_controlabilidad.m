clear;
clc;
R =  3.4;
L = 6/1000;
Nr = 50;
Km = 10;
Tl = 1;
B = 5/10000;
Wm = 1; %vueltas por segundo
Iq = Tl/Km;
Id = 0;
Vd = 0;
Vq = Tl*R/Km;
J = 1;

  %%Matriz que calculé en plano de park
% A = [-R/L Nr*Wm Km*Iq 0;
%      -Nr*Wm -R/L (-Km/L)-Nr*Id 0;
%       0 Km/J -B/J 0;
%       0 0 1 0];
 %%Matriz del paper
% A = [-R/L 0 Km/L 0;
%      0 -R/L (-Km/L) 0;
%      -Km/J Km/J -B/J 0;
%       0 0 1 0];
% 
% B = [1/L 1/L 0 0];
% 
% C = [1 0 0 0;0 1 0 0];

  %%Lo mismo pero con 3 ecuaciones en vez de 4

  %%Matriz que calculé en plano de park
A = [-R/L Nr*Wm Km*Iq;
     -Nr*Wm -R/L (-Km/L)-Nr*Id;
      0 Km/J -B/J];

 %%Matriz del paper
% A = [-R/L 0 Km/L 0;
%      0 -R/L (-Km/L) 0;
%      -Km/J Km/J -B/J 0];


B = [1/L 1/L 0 ];

C = [1 0 0 ;0 1 0];








D = [0 0];

matriz_observavilidad = [C;C*A;C*A*A;C*A*A*A]
observavilidad = rank(matriz_observavilidad)
