import tkinter as tk
from tkinter import ttk

def create_controls(tab, device_id, app):
    control_frame = tk.LabelFrame(tab, text="Control de Motores")
    control_frame.pack(fill=tk.X, pady=10)

    def send(topic_suffix, value):
        topic = f"esp32/{device_id}/{topic_suffix}"
        app.mqtt.client.publish(topic, str(value))

    speed_vars = {}
    dir_vars = {}

    # Añade esta variable para el estado del modo sincronizado
    mantener_relacion = tk.BooleanVar(value=False)

    def on_slider_x(val):
        if mantener_relacion.get():
            vel_motorX = speed_vars['X'].get()
            vel_ejeX = vel_motorX / xRatio
            vel_ejeY = vel_ejeX * (1.8 / 4.0)
            vel_motorY = int(vel_ejeY * yRatio)
            if speed_vars['Y'].get() != vel_motorY:
                speed_vars['Y'].set(vel_motorY)
                publish_speed('Y')  # <-- Publica la velocidad Y sincronizada
        publish_speed('X')        # <-- Publica la velocidad X siempre
        actualizar_vel_ejes()

    def on_slider_y(val):
        if mantener_relacion.get():
            vel_motorY = speed_vars['Y'].get()
            vel_ejeY = vel_motorY / yRatio
            vel_ejeX = vel_ejeY * (4.0 / 1.8)
            vel_motorX = int(vel_ejeX * xRatio)
            if speed_vars['X'].get() != vel_motorX:
                speed_vars['X'].set(vel_motorX)
                publish_speed('X')  # <-- Publica la velocidad X sincronizada
        publish_speed('Y')        # <-- Publica la velocidad Y siempre
        actualizar_vel_ejes()

    # Reemplaza la creación de sliders por esto:
    for axis in ['X', 'Y']:
        row = tk.Frame(control_frame)
        row.pack(fill=tk.X, padx=5, pady=2)
        tk.Label(row, text=f"Velocidad {axis}", width=15).pack(side=tk.LEFT)

        speed_vars[axis] = tk.IntVar(value=0)
        dir_vars[axis] = tk.IntVar(value=1)

        def publish_speed(a=axis):
            val = speed_vars[a].get()
            direction = dir_vars[a].get()
            speed = val if direction == 1 else -val
            send(f"motor/speed{a}", speed)

        if axis == 'X':
            speed_slider = tk.Scale(row, from_=0, to=5000, orient=tk.HORIZONTAL, length=200,
                                    variable=speed_vars[axis], command=on_slider_x)
        else:
            speed_slider = tk.Scale(row, from_=0, to=5000, orient=tk.HORIZONTAL, length=200,
                                    variable=speed_vars[axis], command=on_slider_y)
        speed_slider.pack(side=tk.LEFT, fill=tk.X, expand=True)

        dir_row = tk.Frame(control_frame)
        dir_row.pack(fill=tk.X, padx=5, pady=2)
        tk.Label(dir_row, text=f"Dirección {axis}", width=15).pack(side=tk.LEFT)
        tk.Radiobutton(dir_row, text="Horario", variable=dir_vars[axis], value=1,
                       command=lambda a=axis: publish_speed(a)).pack(side=tk.LEFT)
        tk.Radiobutton(dir_row, text="Antihorario", variable=dir_vars[axis], value=0,
                       command=lambda a=axis: publish_speed(a)).pack(side=tk.LEFT)

    # Botón para activar/desactivar el modo de relación fija
    tk.Checkbutton(control_frame, text="Mantener relación 4:1.8", variable=mantener_relacion).pack(fill=tk.X, padx=5, pady=5)

    sys_row = tk.Frame(control_frame)
    sys_row.pack(fill=tk.X, padx=5, pady=2)
    tk.Label(sys_row, text="Encendido Motor", width=15).pack(side=tk.LEFT)
    tk.Button(sys_row, text="Encender", command=lambda: send("motor/start", 1)).pack(side=tk.LEFT)
    tk.Button(sys_row, text="Detener", command=lambda: send("motor/start", 0)).pack(side=tk.LEFT)

    micro_row = tk.Frame(control_frame)
    micro_row.pack(fill=tk.X, padx=5, pady=2)
    tk.Label(micro_row, text="Microstepping", width=15).pack(side=tk.LEFT)
    micro_var = tk.StringVar()
    micro_dropdown = ttk.Combobox(micro_row, textvariable=micro_var, values=["0", "1", "2", "4"])
    micro_dropdown.pack(side=tk.LEFT, expand=True, fill=tk.X)
    tk.Button(micro_row, text="Enviar", command=lambda: send("motor/microstepping", micro_var.get())).pack(side=tk.RIGHT)

    sensor_row = tk.LabelFrame(tab, text="Sensor CO2")
    sensor_row.pack(fill=tk.X, pady=5)
    tk.Button(sensor_row, text="Medir ahora", command=lambda: send("sensor/measure", 1)).pack(side=tk.LEFT, padx=10)
    tk.Button(sensor_row, text="Detener medición", command=lambda: send("sensor/measure", 0)).pack(side=tk.LEFT, padx=10)

    # Botón Flag calibrar
    if device_id not in app._calibrar_flag_state:
        app._calibrar_flag_state[device_id] = False

    def enviar_flag_calibrar():
        import paho.mqtt.client as mqtt
        client = mqtt.Client()
        client.connect(app.sensor_state.BROKER, app.sensor_state.PORT, 60)
        topic = f"esp32/{device_id}/sensor/calibrar"
        app._calibrar_flag_state[device_id] = not app._calibrar_flag_state[device_id]
        client.publish(topic, str(int(app._calibrar_flag_state[device_id])))
        client.disconnect()

    tk.Button(sensor_row, text="Flag calibrar", command=enviar_flag_calibrar).pack(side=tk.LEFT, padx=10)

    # Parámetros de engranajes
    xMotTeeth = 16
    xPulleyTeeth = 50
    yMotTeeth = 16
    yTurnTeeth = 50
    yPulleyTeeth = 20

    # Relaciones de transmisión
    xRatio = xPulleyTeeth / xMotTeeth
    yRatio1 = yTurnTeeth / yMotTeeth
    yRatio2 = yPulleyTeeth / yTurnTeeth
    yCorrection = 1.5  # Ajusta según calibración real
    yRatio = yRatio1 * yRatio2 * yCorrection

    # Labels para mostrar la velocidad real de cada eje
    vel_ejeX_label = tk.Label(control_frame, text="Velocidad eje X: 0.00")
    vel_ejeX_label.pack(fill=tk.X, padx=5, pady=2)
    vel_ejeY_label = tk.Label(control_frame, text="Velocidad eje Y: 0.00")
    vel_ejeY_label.pack(fill=tk.X, padx=5, pady=2)

    def actualizar_vel_ejes(*args):
        vel_motorX = speed_vars['X'].get()
        vel_motorY = speed_vars['Y'].get()
        vel_ejeX = vel_motorX / xRatio
        vel_ejeY = vel_motorY / yRatio
        vel_ejeX_label.config(text=f"Velocidad eje X: {vel_ejeX:.2f}")
        vel_ejeY_label.config(text=f"Velocidad eje Y: {vel_ejeY:.2f}")

    # Vincula la actualización a ambos sliders
    speed_vars['X'].trace_add('write', lambda *a: actualizar_vel_ejes())
    speed_vars['Y'].trace_add('write', lambda *a: actualizar_vel_ejes())

    # Llama una vez al inicio para mostrar los valores iniciales
    actualizar_vel_ejes()


