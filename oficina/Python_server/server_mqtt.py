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

BROKER = "192.168.31.81"  # Cambia por tu IP
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
        self.info_labels = {}  # Para mostrar errores, vcap1, vcap2
        self._calibrar_flag_state = {}
        self.lp8_buffer = {}  # Buffer temporal para cada sensor
        self.lp8_buffer_time = {}  # Timestamp de inicio de cada buffer
        self.lp8_buffer_timeout = 1.0  # segundos

        self.bme_buffer = {}  # <--- Agrega esta línea
        self.bme_buffer_time = {}  # <--- Y esta línea

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
        self.check_lp8_buffers()

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

        # Procesar tópicos de sensores LP8_1 y LP8_2
        elif topic.startswith("esp32/"):
            parts = topic.split("/")
            if len(parts) >= 4 and parts[2].startswith("LP8_") and parts[3] in ["co2", "presion", "vcap1", "vcap2", "errores"]:
                device_id = parts[1]
                if device_id in self.tabs:
                    if parts[2].startswith("LP8_") and parts[3] in ["co2", "presion", "vcap1", "vcap2", "errores"]:
                        lp8_id = parts[2]  # LP8_1 o LP8_2
                        var = parts[3]
                        device_dir = device_id.replace(":", "_")
                        file_key = f"{device_dir}_{lp8_id}"
                        filename = os.path.join(device_dir, f"{lp8_id}.txt")
                        now_str = time.strftime('%Y-%m-%d %H:%M:%S')
                        # Buffer temporal por timestamp
                        if file_key not in self.lp8_buffer or self.lp8_buffer[file_key]['timestamp'] != now_str:
                            # Si hay buffer anterior, vaciarlo
                            if file_key in self.lp8_buffer:
                                self.flush_lp8_buffer(file_key, filename)
                            self.lp8_buffer[file_key] = {'timestamp': now_str, 'data': {}}
                            self.lp8_buffer_time[file_key] = time.time()
                        self.lp8_buffer[file_key]['data'][var] = payload
                        # Si ya tenemos todos los campos, escribir y limpiar
                        if all(k in self.lp8_buffer[file_key]['data'] for k in ["co2", "presion", "vcap1", "vcap2", "errores"]):
                            self.flush_lp8_buffer(file_key, filename)
            elif len(parts) >= 4 and parts[2].lower() == "bme280" and parts[3] in ["temp", "hum", "presion"]:
                device_id = parts[1]
                if device_id in self.tabs:
                    device_dir = device_id.replace(":", "_")
                    bme_file = os.path.join(device_dir, "bme280.txt")
                    now_str = time.strftime('%Y-%m-%d %H:%M:%S')
                    var = parts[3]
                    # Si el archivo no existe, crea encabezado
                    if not os.path.exists(bme_file):
                        with open(bme_file, "w") as f:
                            f.write("timestamp,temp,hum,presion\n")
                    # Buffer temporal por timestamp
                    if not hasattr(self, "bme_buffer"):
                        self.bme_buffer = {}
                        self.bme_buffer_time = {}
                    file_key = device_dir
                    if file_key not in self.bme_buffer or self.bme_buffer[file_key]['timestamp'] != now_str:
                        # Si hay buffer anterior, vaciarlo
                        if file_key in self.bme_buffer:
                            vals = self.bme_buffer[file_key]['data']
                            line = f"{self.bme_buffer[file_key]['timestamp']},{vals.get('temp','')},{vals.get('hum','')},{vals.get('presion','')}\n"
                            with open(bme_file, "a") as f:
                                f.write(line)
                        self.bme_buffer[file_key] = {'timestamp': now_str, 'data': {}}
                        self.bme_buffer_time[file_key] = time.time()
                    self.bme_buffer[file_key]['data'][var] = payload
                    # Si ya tenemos todos los campos, escribir y limpiar
                    if all(k in self.bme_buffer[file_key]['data'] for k in ["temp", "hum", "presion"]):
                        vals = self.bme_buffer[file_key]['data']
                        line = f"{self.bme_buffer[file_key]['timestamp']},{vals.get('temp','')},{vals.get('hum','')},{vals.get('presion','')}\n"
                        with open(bme_file, "a") as f:
                            f.write(line)
                        del self.bme_buffer[file_key]
                        del self.bme_buffer_time[file_key]
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
            messagebox.showwarning("Advertencia", "Selecciona un dispositivo primero.")
            return
        device_id = self.listbox.get(selection[0])
        if device_id in self.tabs:
            self.notebook.select(self.tabs[device_id])
            return
        tab = tk.Frame(self.notebook)
        self.notebook.add(tab, text=device_id)
        self.tabs[device_id] = tab

        # Crear carpeta para el dispositivo y archivos únicos para LP8_1 y LP8_2
        device_dir = device_id.replace(":", "_")
        os.makedirs(device_dir, exist_ok=True)
        for lp8_id in ["LP8_1", "LP8_2"]:
            filename = os.path.join(device_dir, f"{lp8_id}.txt")
            if not os.path.exists(filename):
                with open(filename, "w") as f:
                    f.write("timestamp,co2,presion,vcap1,vcap2,errores\n")
            # Guardar la ruta completa, indexada por device_id+lp8_id
            self.lp8_files[f"{device_dir}_{lp8_id}"] = filename
        # Crear labels para errores, vcap1, vcap2 de cada sensor
        info_frame = tk.LabelFrame(tab, text="Estado sensores LP8")
        info_frame.pack(fill=tk.X, padx=10, pady=5)
        for lp8_id in ["LP8_1", "LP8_2"]:
            if lp8_id not in self.info_labels:
                self.info_labels[lp8_id] = {}
            subf = tk.Frame(info_frame)
            subf.pack(fill=tk.X, padx=5, pady=2)
            tk.Label(subf, text=lp8_id, width=8).pack(side=tk.LEFT)
            for var in ["errores", "vcap1", "vcap2"]:
                lbl = tk.Label(subf, text=f"{var}: ---", width=15)
                lbl.pack(side=tk.LEFT, padx=5)
                self.info_labels[lp8_id][var] = lbl

        sub_tabs = ttk.Notebook(tab)
        sub_tabs.pack(fill=tk.BOTH, expand=True)

        # Sub-pestaña de estado y controles
        tab_status = tk.Frame(sub_tabs)
        sub_tabs.add(tab_status, text="Estado y Control")

        status_label = tk.Label(tab_status, text="Esperando estado...", anchor="w", justify="left")
        status_label.pack(pady=5, fill=tk.X)
        self.status_labels[device_id.replace(":", "_")] = status_label

        self.create_controls(tab_status, device_id)

        # Sub-pestaña de gráfica
        tab_graph = tk.Frame(sub_tabs)
        sub_tabs.add(tab_graph, text="Gráfico LP8")

        fig, ax = plt.subplots(figsize=(5, 3))
        line1, = ax.plot([], [], color='blue', label='LP8_1')
        line2, = ax.plot([], [], color='green', label='LP8_2')
        ax.set_title(f"LP8 - {device_id}")
        ax.set_xlabel("Tiempo")
        ax.set_ylabel("Valor")
        ax.grid(True)
        ax.legend()
        canvas = FigureCanvasTkAgg(fig, master=tab_graph)
        canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        

        # Guardar ambas líneas y los labels en el dict de plots
        self.lp8_plots[device_id.replace(":", "_")] = (fig, ax, canvas, line1, line2, info_labels)

        # Suscribirse a todos los sub-tópicos de LP8_1 y LP8_2
        subtopics = ["co2", "presion", "vcap1", "vcap2", "errores"]
        for lp8 in ["LP8_1", "LP8_2"]:
            for sub in subtopics:
                topic = f"esp32/{device_id}/{lp8}/{sub}"
                self.client.subscribe(topic)
        topic_status = f"esp32/{device_id}/status"
        self.client.subscribe(topic_status)

        # Sensor CO2 tab
        sensor_frame = tk.Frame(tab)
        sensor_frame.pack(fill=tk.BOTH, expand=True)
        tk.Label(sensor_frame, text="Sensor CO2").pack(pady=10)
    # El botón de calibrar se agregará junto a los otros botones en create_controls

    def create_controls(self, frame, client_id):
        control_frame = tk.LabelFrame(frame, text="Control de Motores")
        control_frame.pack(fill=tk.X, pady=10)

        def send(topic_suffix, value):
            topic = f"esp32/{client_id}/{topic_suffix}"
            self.client.publish(topic, str(value))

        speed_vars = {}
        dir_vars = {}

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

            speed_slider = tk.Scale(row, from_=0, to=2000, orient=tk.HORIZONTAL, length=200,
                                    variable=speed_vars[axis], command=lambda val, a=axis: publish_speed(a))
            speed_slider.pack(side=tk.LEFT, fill=tk.X, expand=True)

            dir_row = tk.Frame(control_frame)
            dir_row.pack(fill=tk.X, padx=5, pady=2)
            tk.Label(dir_row, text=f"Dirección {axis}", width=15).pack(side=tk.LEFT)
            tk.Radiobutton(dir_row, text="Horario", variable=dir_vars[axis], value=1,
                           command=lambda a=axis: publish_speed(a)).pack(side=tk.LEFT)
            tk.Radiobutton(dir_row, text="Antihorario", variable=dir_vars[axis], value=0,
                           command=lambda a=axis: publish_speed(a)).pack(side=tk.LEFT)

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
        tk.Button(sensor_row, text="Detener medición", command=lambda: send("sensor/measure", 0)).pack(side=tk.LEFT, padx=10)
        # Botón Flag calibrar junto a los otros
        if client_id not in self._calibrar_flag_state:
            self._calibrar_flag_state[client_id] = False
        def enviar_flag_calibrar():
            import paho.mqtt.client as mqtt
            client = mqtt.Client()
            client.connect(BROKER, PORT, 60)
            topic = f"esp32/{client_id}/sensor/calibrar"
            self._calibrar_flag_state[client_id] = not self._calibrar_flag_state[client_id]
            client.publish(topic, str(int(self._calibrar_flag_state[client_id])))
            client.disconnect()
        tk.Button(sensor_row, text="Flag calibrar", command=enviar_flag_calibrar).pack(side=tk.LEFT, padx=10)

    def flush_lp8_buffer(self, file_key, filename):
        # Escribir y limpiar el buffer si existe
        buf = self.lp8_buffer.get(file_key)
        if not buf:
            return
        vals = {"co2": "", "presion": "", "vcap1": "", "vcap2": "", "errores": ""}
        vals.update(buf['data'])
        line = f"{buf['timestamp']},{vals['co2']},{vals['presion']},{vals['vcap1']},{vals['vcap2']},{vals['errores']}\n"
        with open(filename, "a") as f:
            f.write(line)
        # Extraer device_id del file_key
        device_id = file_key.split("_")[0]
        self.update_plot(device_id)
        del self.lp8_buffer[file_key]
        del self.lp8_buffer_time[file_key]

    def check_lp8_buffers(self):
        # Llamar periódicamente para vaciar buffers expirados
        now = time.time()
        for file_key in list(self.lp8_buffer.keys()):
            t0 = self.lp8_buffer_time[file_key]
            if now - t0 > self.lp8_buffer_timeout:
                filename = self.lp8_files.get(file_key)
                if filename:
                    self.flush_lp8_buffer(file_key, filename)
        self.root.after(200, self.check_lp8_buffers)

    def update_plot(self, device_id):
        import datetime
        import matplotlib.dates as mdates
        device_dir = device_id.replace(":", "_")
        data_lp8_1 = []
        data_lp8_2 = []
        time_lp8_1 = []
        time_lp8_2 = []
        vcap1_1 = vcap2_1 = errores_1 = ""
        vcap1_2 = vcap2_2 = errores_2 = ""

        def parse_time(ts):
            try:
                return datetime.datetime.strptime(ts, "%Y-%m-%d %H:%M:%S")
            except Exception:
                return None

        # LP8_1
        try:
            with open(os.path.join(device_dir, "LP8_1.txt"), "r") as f1:
                lines1 = f1.readlines()[1:]
                for line in lines1[-100:]:
                    parts = line.strip().split(",")
                    # Formato: fecha, co2, presion, vcap1, vcap2, errores
                    if len(parts) >= 6 and parts[1] != "":
                        t = parse_time(parts[0])
                        if t is not None:
                            try:
                                data_lp8_1.append(float(parts[1]))
                                time_lp8_1.append(t)
                                vcap1_1 = parts[3]
                                vcap2_1 = parts[4]
                                errores_1 = parts[5]
                            except ValueError:
                                pass
        except Exception:
            pass

        # LP8_2
        try:
            with open(os.path.join(device_dir, "LP8_2.txt"), "r") as f2:
                lines2 = f2.readlines()[1:]
                for line in lines2[-100:]:
                    parts = line.strip().split(",")
                    if len(parts) >= 6 and parts[1] != "":
                        t = parse_time(parts[0])
                        if t is not None:
                            try:
                                data_lp8_2.append(float(parts[1]))
                                time_lp8_2.append(t)
                                vcap1_2 = parts[3]
                                vcap2_2 = parts[4]
                                errores_2 = parts[5]
                            except ValueError:
                                pass
        except Exception:
            pass

        # Eje X: hora de medición
        x1 = time_lp8_1
        x2 = time_lp8_2

        key = device_id.replace(":", "_")
        if key in self.lp8_plots:
            fig, ax, canvas, line1, line2, info_labels = self.lp8_plots[key]
            line1.set_data(x1, data_lp8_1)
            line2.set_data(x2, data_lp8_2)
            ax.set_xlim(min(x1 + x2) if (x1 + x2) else 0, max(x1 + x2) if (x1 + x2) else 1)
            all_data = data_lp8_1 + data_lp8_2
            if all_data:
                ax.set_ylim(min(all_data) - 1, max(all_data) + 1)
            ax.set_xlabel("Hora de medición")
            ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
            fig.autofmt_xdate()
            canvas.draw()
           

if __name__ == "__main__":
    root = tk.Tk()
    app = App(root)
    root.mainloop()