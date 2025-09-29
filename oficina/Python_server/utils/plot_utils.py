import tkinter as tk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

def update_lp8_plot(tab_graph, device_id, app):
    fig, ax = plt.subplots(figsize=(5, 3))
    line1, = ax.plot([], [], color='blue', label='LP8_1')
    line2, = ax.plot([], [], color='green', label='LP8_2')
    ax.set_title(f"LP8 - {device_id}")
    ax.set_xlabel("Tiempo")
    ax.set_ylabel("CO2 (ppm)")
    ax.grid(True)
    ax.legend()
    canvas = FigureCanvasTkAgg(fig, master=tab_graph)
    canvas.get_tk_widget().pack(fill='both', expand=True)

    # Inicializa los labels con "---"
    state_frame = tk.Frame(tab_graph)
    state_frame.pack(pady=5)
    lp8_1_label = tk.Label(state_frame, text="LP8_1 - Vcap1: ---   Vcap2: ---   error: ---", font=("Courier", 10))
    lp8_1_label.pack(anchor="w")
    lp8_2_label = tk.Label(state_frame, text="LP8_2 - Vcap1: ---   Vcap2: ---   error: ---", font=("Courier", 10))
    lp8_2_label.pack(anchor="w")

    return (fig, ax, canvas, line1, line2, lp8_1_label, lp8_2_label)

def update_bme_plot(tab_bme, device_id, app):
    fig, ax = plt.subplots(figsize=(5, 3))
    line_temp, = ax.plot([], [], color='red', label='Temp (°C)')
    line_hum, = ax.plot([], [], color='blue', label='Humedad (%)')
    line_pres, = ax.plot([], [], color='green', label='Presión (hPa)')
    ax.set_title(f"BME280 - {device_id}")
    ax.set_xlabel("Tiempo")
    ax.set_ylabel("Valor")
    ax.grid(True)
    ax.legend()
    canvas = FigureCanvasTkAgg(fig, master=tab_bme)
    canvas.get_tk_widget().pack(fill='both', expand=True)
    return (fig, ax, canvas, line_temp, line_hum, line_pres)