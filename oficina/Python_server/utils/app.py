import tkinter as tk
from tkinter import ttk, messagebox
from mqtt_handler import MQTTHandler
from sensor_state import SensorState
from plot_utils import update_lp8_plot, update_bme_plot
from ui_controls import create_controls

class App:
    def __init__(self, root, broker_ip="192.168.31.81", broker_port=1883):
        self.root = root
        self.root.title("Detector ESP32 con MQTT")
        self.esp32_devices = {}
        self.tabs = {}
        self.lp8_files = {}
        self.lp8_plots = {}
        self.bme_plots = {}
        self.status_labels = {}
        self.info_labels = {}
        self._calibrar_flag_state = {}

        # Instancias de módulos
        self.sensor_state = SensorState(self, broker_ip, broker_port)
        self.mqtt = MQTTHandler(self)
        
        frame_list = tk.Frame(root)
        frame_list.pack(padx=10, pady=10, fill=tk.BOTH)
        tk.Label(frame_list, text="ESP32 disponibles:").pack(anchor="w")
        self.listbox = tk.Listbox(frame_list, height=5)
        self.listbox.pack(fill=tk.BOTH, expand=True)
        tk.Button(frame_list, text="Agregar módulo ESP32 seleccionado", command=self.add_module).pack(pady=5)
        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        self.update_list()
     

    def refresh_listbox(self):
        self.listbox.delete(0, tk.END)
        now = self.sensor_state.time()
        for client_id, last_seen in self.esp32_devices.items():
            if now - last_seen < self.sensor_state.HEARTBEAT_TIMEOUT:
                self.listbox.insert(tk.END, client_id)

    def update_list(self):
        now = self.sensor_state.time()
        to_remove = [cid for cid, t in self.esp32_devices.items() if now - t > self.sensor_state.HEARTBEAT_TIMEOUT]
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
        self.sensor_state.setup_device_files(device_id)
        self.info_labels[device_id] = self.sensor_state.create_info_labels(tab)

        # --- SUBPESTAÑAS ---
        sub_tabs = ttk.Notebook(tab)
        sub_tabs.pack(fill=tk.BOTH, expand=True)

        # Subpestaña de estado y controles
        tab_status = tk.Frame(sub_tabs)
        sub_tabs.add(tab_status, text="Estado y Control")
        create_controls(tab_status, device_id, self)

        # Subpestaña de gráfico LP8
        tab_graph = tk.Frame(sub_tabs)
        sub_tabs.add(tab_graph, text="Gráfico LP8")
        self.lp8_plots[device_id] = update_lp8_plot(tab_graph, device_id, self)

        # --- NUEVA SUBPESTAÑA BME280 ---
        tab_bme = tk.Frame(sub_tabs)
        sub_tabs.add(tab_bme, text="BME280")

        frame_temp = tk.Frame(tab_bme)
        frame_temp.pack(fill='both', expand=True)
        frame_hum = tk.Frame(tab_bme)
        frame_hum.pack(fill='both', expand=True)
        frame_pres = tk.Frame(tab_bme)
        frame_pres.pack(fill='both', expand=True)

        self.bme_plots[device_id] = (frame_temp, frame_hum, frame_pres)
        self.update_bme_plot(device_id)

    def update_plot(self, device_id):
        print(f"[DEBUG] update_plot llamado para {device_id}")
        import os
        import datetime
        import matplotlib.dates as mdates

        device_dir = device_id.replace(":", "_")
        key = device_id
        if key not in self.lp8_plots:
            return

        fig, ax, canvas, line1, line2, lp8_1_label, lp8_2_label = self.lp8_plots[key]

        # --- LP8_1 ---
        times1, co2_1, vcap1_1, vcap2_1, errores_1 = [], [], "---", "---", "---"
        try:
            filename1 = os.path.join(device_dir, "LP8_1.txt")
            print(f"[DEBUG] Abriendo archivo: {os.path.join(device_dir, 'LP8_1.txt')}")
            with open(filename1, "r") as f1:
                lines1 = f1.readlines()[1:]
                print(f"[DEBUG] LP8_1.txt líneas leídas: {len(lines1)}")
                for line in lines1[-100:]:
                    parts = line.strip().split(",")
                    print(f"[DEBUG] Línea LP8_1: {parts}")
                    if len(parts) >= 7:
                        try:
                            t = datetime.datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
                            print(f"[DEBUG] Fecha parseada: {t}, CO2: {parts[1]}")
                            times1.append(t)
                            co2_1.append(float(parts[1]))
                            vcap1_1 = parts[3]
                            vcap2_1 = parts[4]
                            errores_1 = parts[5]
                            temp = float(parts[6])
                        except Exception as e:
                            print(f"[DEBUG] Error parseando: {e}")
                            continue
        except Exception:
            pass

        # --- LP8_2 ---
        times2, co2_2, vcap1_2, vcap2_2, errores_2 = [], [], "---", "---", "---"
        try:
            filename2 = os.path.join(device_dir, "LP8_2.txt")
            with open(filename2, "r") as f2:
                lines2 = f2.readlines()[1:]
                for line in lines2[-100:]:
                    parts = line.strip().split(",")
                    if len(parts) >= 6:
                        try:
                            t = datetime.datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
                            times2.append(t)
                            co2_2.append(float(parts[1]))
                            vcap1_2 = parts[3]
                            vcap2_2 = parts[4]
                            errores_2 = parts[5]
                        except Exception:
                            continue
        except Exception:
            pass

        print(f"[DEBUG] LP8_1: {len(times1)} puntos, LP8_2: {len(times2)} puntos")

        # Actualiza la gráfica
        line1.set_data(times1, co2_1)
        line2.set_data(times2, co2_2)
        all_times = times1 + times2
        all_co2 = co2_1 + co2_2
        if all_times:
            ax.set_xlim(min(all_times), max(all_times))
        if all_co2:
            ax.set_ylim(min(all_co2) - 10, max(all_co2) + 10)
        ax.set_xlabel("Hora de medición")
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        fig.autofmt_xdate()
        canvas.draw()
        

        # Actualiza los labels de estado
        lp8_1_label.config(text=f"LP8_1 - Vcap1: {vcap1_1}   Vcap2: {vcap2_1}   error: {errores_1}")
        lp8_2_label.config(text=f"LP8_2 - Vcap1: {vcap1_2}   Vcap2: {vcap2_2}   error: {errores_2}")

    def update_bme_plot(self, device_id):
        import os
        import datetime
        import matplotlib.dates as mdates
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
        import matplotlib.pyplot as plt

        device_dir = device_id.replace(":", "_")
        key = device_id
        if key not in self.bme_plots:
            print(f"[DEBUG BME] No hay plot para {device_id}")
            return

        # Lee los datos
        times, temps, hums, press = [], [], [], []
        try:
            filename = os.path.join(device_dir, "bme280.txt")
            with open(filename, "r") as f:
                lines = f.readlines()[1:]  # Salta encabezado
                for line in lines[-100:]:
                    parts = line.strip().split(",")
                    if len(parts) == 4:
                        try:
                            t = datetime.datetime.strptime(parts[0], "%Y-%m-%d %H:%M:%S")
                            times.append(t)
                            temps.append(float(parts[1]))
                            hums.append(float(parts[2]))
                            press.append(float(parts[3]))
                        except Exception as e:
                            print(f"[DEBUG BME] Error parseando línea: {e}")
                            continue
        except Exception as e:
            print(f"[DEBUG BME] Error leyendo bme280.txt: {e}")

        # Desempaqueta los frames de la subpestaña
        frame_temp, frame_hum, frame_pres = self.bme_plots[key]

        # --- Temperatura ---
        for widget in frame_temp.winfo_children():
            widget.destroy()
        fig1, ax1 = plt.subplots(figsize=(5, 1.5))
        ax1.plot(times, temps, color='red', label='Temp (°C)')
        ax1.set_title("Temperatura")
        ax1.set_ylabel("°C")
        ax1.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        fig1.autofmt_xdate()
        canvas1 = FigureCanvasTkAgg(fig1, master=frame_temp)
        canvas1.get_tk_widget().pack(fill='both', expand=True)
        canvas1.draw()
        plt.close(fig1)  # <-- CIERRA la figura después de dibujar

        # --- Humedad ---
        for widget in frame_hum.winfo_children():
            widget.destroy()
        fig2, ax2 = plt.subplots(figsize=(5, 1.5))
        ax2.plot(times, hums, color='blue', label='Humedad (%)')
        ax2.set_title("Humedad")
        ax2.set_ylabel("%")
        ax2.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        fig2.autofmt_xdate()
        canvas2 = FigureCanvasTkAgg(fig2, master=frame_hum)
        canvas2.get_tk_widget().pack(fill='both', expand=True)
        canvas2.draw()
        plt.close(fig2)  # <-- CIERRA la figura después de dibujar

        # --- Presión ---
        for widget in frame_pres.winfo_children():
            widget.destroy()
        fig3, ax3 = plt.subplots(figsize=(5, 1.5))
        ax3.plot(times, press, color='green', label='Presión (hPa)')
        ax3.set_title("Presión")
        ax3.set_ylabel("hPa")
        ax3.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        fig3.autofmt_xdate()
        canvas3 = FigureCanvasTkAgg(fig3, master=frame_pres)
        canvas3.get_tk_widget().pack(fill='both', expand=True)
        canvas3.draw()
        plt.close(fig3)  # <-- CIERRA la figura después de dibujar