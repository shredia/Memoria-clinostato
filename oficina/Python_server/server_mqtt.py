import tkinter as tk
from tkinter import ttk, messagebox
import paho.mqtt.client as mqtt
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque
import os
import json

BROKER = "192.168.31.61"  # Cambia por tu IP
PORT = 1883

HEARTBEAT_TOPIC = "esp32/heartbeat/#"
HEARTBEAT_TIMEOUT = 15  # segundos

class App:
    def __init__(self, root):
        self.root = root
        self.root.title("Detector ESP32 con MQTT")

        self.esp32_devices = {}
        self.tabs = {}
        self.lp8_files = {}
        self.lp8_plots = {}
        self.status_labels = {}

        frame_list = tk.Frame(root)
        frame_list.pack(padx=10, pady=10, fill=tk.BOTH)

        tk.Label(frame_list, text="ESP32 disponibles:").pack(anchor="w")
        self.listbox = tk.Listbox(frame_list, height=5)
        self.listbox.pack(fill=tk.BOTH, expand=True)

        tk.Button(frame_list, text="Agregar módulo ESP32 seleccionado", command=self.add_module).pack(pady=5)

        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill=tk.BOTH, expand=True)

        self.client = mqtt.Client(protocol=mqtt.MQTTv311)
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(BROKER, PORT, 60)
        threading.Thread(target=self.client.loop_forever, daemon=True).start()

        self.update_list()

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Conectado al broker MQTT!")
            client.subscribe(HEARTBEAT_TOPIC)
        else:
            print(f"Error de conexión: {rc}")

    def on_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode()

        if topic.startswith("esp32/heartbeat/"):
            client_id = topic.split("/")[-1]
            self.esp32_devices[client_id] = time.time()
            self.root.after(0, self.refresh_listbox)

        elif "/lp8" in topic:
            client_id = topic.split("/")[1]
            safe_id = client_id.replace(":", "_")
            if safe_id in self.lp8_files:
                try:
                    value = float(payload)
                    timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
                    line = f"{timestamp},{value}\n"
                    with open(self.lp8_files[safe_id], "a") as f:
                        f.write(line)
                    self.update_plot(safe_id)
                except ValueError:
                    pass

        elif topic.endswith("/status"):
            client_id = topic.split("/")[1]
            safe_id = client_id.replace(":", "_")
            try:
                data = json.loads(payload)
                formatted = "\n".join([f"{k}: {v}" for k, v in data.items()])
                if safe_id in self.status_labels:
                    self.status_labels[safe_id].config(
                        text=f"\u2705 Estado del dispositivo:\n\n{formatted}",
                        justify="left",
                        font=("Courier", 10)
                    )
                print(f"Estado recibido de {client_id}: {formatted}")
            except:
                pass

    def refresh_listbox(self):
        self.listbox.delete(0, tk.END)
        now = time.time()
        for client_id, last_seen in self.esp32_devices.items():
            if now - last_seen < HEARTBEAT_TIMEOUT:
                self.listbox.insert(tk.END, client_id)

    def update_list(self):
        now = time.time()
        to_remove = [cid for cid, t in self.esp32_devices.items() if now - t > HEARTBEAT_TIMEOUT]
        for cid in to_remove:
            del self.esp32_devices[cid]
        self.refresh_listbox()
        self.root.after(5000, self.update_list)

    def add_module(self):
        selection = self.listbox.curselection()
        if not selection:
            messagebox.showwarning("Atención", "Selecciona un ESP32 de la lista")
            return

        client_id = self.listbox.get(selection[0])
        if client_id in self.tabs:
            return

        frame = tk.Frame(self.notebook)
        self.notebook.add(frame, text=client_id)
        self.tabs[client_id] = frame

        safe_id = client_id.replace(":", "_")
        filename = f"{safe_id}_lp8.txt"
        with open(filename, "w") as f:
            f.write("timestamp,lp8_value\n")
        self.lp8_files[safe_id] = filename

        sub_tabs = ttk.Notebook(frame)
        sub_tabs.pack(fill=tk.BOTH, expand=True)

        # Sub-pestaña de estado y controles
        tab_status = tk.Frame(sub_tabs)
        sub_tabs.add(tab_status, text="Estado y Control")

        status_label = tk.Label(tab_status, text="Esperando estado...", anchor="w", justify="left")
        status_label.pack(pady=5, fill=tk.X)
        self.status_labels[safe_id] = status_label

        self.create_controls(tab_status, client_id)

        # Sub-pestaña de gráfica
        tab_graph = tk.Frame(sub_tabs)
        sub_tabs.add(tab_graph, text="Gráfico LP8")

        fig, ax = plt.subplots(figsize=(5, 3))
        line, = ax.plot([], [], color='blue')
        ax.set_title(f"LP8 - {client_id}")
        ax.set_xlabel("Tiempo")
        ax.set_ylabel("Valor")
        ax.grid(True)
        canvas = FigureCanvasTkAgg(fig, master=tab_graph)
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        self.lp8_plots[safe_id] = (fig, ax, canvas, line)

        topic_lp8 = f"esp32/{client_id}/lp8"
        topic_status = f"esp32/{client_id}/status"
        self.client.subscribe(topic_lp8)
        self.client.subscribe(topic_status)

    def create_controls(self, frame, client_id):
        control_frame = tk.LabelFrame(frame, text="Control de Motores")
        control_frame.pack(fill=tk.X, pady=10)

        def send(topic_suffix, value):
            topic = f"esp32/{client_id}/{topic_suffix}"
            self.client.publish(topic, str(value))

        for axis in ['X', 'Y']:
            row = tk.Frame(control_frame)
            row.pack(fill=tk.X, padx=5, pady=2)
            tk.Label(row, text=f"Velocidad {axis}", width=15).pack(side=tk.LEFT)
            speed_slider = tk.Scale(row, from_=0, to=2000, orient=tk.HORIZONTAL, length=200,
                                    command=lambda val, a=axis: send(f"motor/speed{a}", val))
            speed_slider.pack(side=tk.LEFT, fill=tk.X, expand=True)

            dir_row = tk.Frame(control_frame)
            dir_row.pack(fill=tk.X, padx=5, pady=2)
            tk.Label(dir_row, text=f"Dirección {axis}", width=15).pack(side=tk.LEFT)
            dir_var = tk.IntVar()
            tk.Radiobutton(dir_row, text="Horario", variable=dir_var, value=1, command=lambda a=axis, v=dir_var: send(f"motor/direction{a}", v.get())).pack(side=tk.LEFT)
            tk.Radiobutton(dir_row, text="Antihorario", variable=dir_var, value=0, command=lambda a=axis, v=dir_var: send(f"motor/direction{a}", v.get())).pack(side=tk.LEFT)

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

        sensor_row = tk.LabelFrame(frame, text="Sensor CO2")
        sensor_row.pack(fill=tk.X, pady=5)
        tk.Button(sensor_row, text="Medir ahora", command=lambda: send("sensor/measure", 1)).pack(side=tk.LEFT, padx=10)
        tk.Button(sensor_row, text="Detener medición", command=lambda: send("sensor/stop", 1)).pack(side=tk.LEFT, padx=10)

    def update_plot(self, safe_id):
        data = []
        try:
            with open(self.lp8_files[safe_id], "r") as f:
                lines = f.readlines()[1:]
                for line in lines[-100:]:
                    _, val = line.strip().split(",")
                    data.append(float(val))
        except:
            return

        fig, ax, canvas, line = self.lp8_plots[safe_id]
        line.set_data(range(len(data)), data)
        ax.set_xlim(0, max(10, len(data)))
        if data:
            ax.set_ylim(min(data) - 1, max(data) + 1)
        canvas.draw()

if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()
