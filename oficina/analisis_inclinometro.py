import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import json
import re

# Leer archivo con datos
with open('datos_mpu6050.txt', 'r') as file:
    lines = file.readlines()

ax_list, ay_list, az_list = [], [], []
gx_list, gy_list, gz_list = [], [], []

# Extraer datos JSON
for line in lines:
    match = re.search(r'\{.*\}', line)
    if match:
        try:
            data = json.loads(match.group())
            ax_list.append(data["ax"])
            ay_list.append(data["ay"])
            az_list.append(data["az"])
            gx_list.append(data["gx"])
            gy_list.append(data["gy"])
            gz_list.append(data["gz"])
        except (json.JSONDecodeError, KeyError):
            continue

# Convertir a arrays
ax = np.array(ax_list)
ay = np.array(ay_list)
az = np.array(az_list)
gx = np.array(gx_list)
gy = np.array(gy_list)
gz = np.array(gz_list)

# Calcular magnitud total de la aceleración
acc_magnitude = np.sqrt(ax**2 + ay**2 + az**2)

# Normalizar vector giroscopio para proyectar sobre esfera
gyro_vectors = np.vstack((gx, gy, gz)).T
gyro_norms = np.linalg.norm(gyro_vectors, axis=1)
# Evitar división por cero
gyro_norms[gyro_norms == 0] = 1
gyro_unit_vectors = gyro_vectors / gyro_norms[:, None]

x_sphere = gyro_unit_vectors[:, 0]
y_sphere = gyro_unit_vectors[:, 1]
z_sphere = gyro_unit_vectors[:, 2]

# Crear figura y subplots
fig = plt.figure(figsize=(14, 6))

# Gráfica 3D: trayectoria en la superficie de la esfera
ax3d = fig.add_subplot(121, projection='3d')
ax3d.plot(x_sphere, y_sphere, z_sphere, label='Orientación normalizada')
ax3d.set_xlabel('X')
ax3d.set_ylabel('Y')
ax3d.set_zlabel('Z')
ax3d.set_title('Orientación del giroscopio en esfera unitaria')
ax3d.legend()
ax3d.grid(True)

# Gráfica 2D de magnitud de aceleración
ax2d = fig.add_subplot(122)
ax2d.plot(acc_magnitude, label='Magnitud total de aceleración', color='black')
ax2d.axhline(9.8, color='red', linestyle='--', label='Gravedad estándar (9.8 m/s²)')
ax2d.set_xlabel('Muestra')
ax2d.set_ylabel('Aceleración (m/s²)')
ax2d.set_title('Evolución de la magnitud total de aceleración')
ax2d.legend()
ax2d.grid(True)

plt.tight_layout()
plt.show()
