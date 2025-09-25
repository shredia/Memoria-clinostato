import os
import time
import json

class SensorState:
    def __init__(self, app, broker_ip="192.168.31.81", broker_port=1883):
        self.app = app
        self.BROKER = broker_ip
        self.PORT = broker_port
        self.HEARTBEAT_TOPIC = "esp32/#"
        self.HEARTBEAT_TIMEOUT = 15

        self.lp8_buffer = {}
        self.lp8_buffer_time = {}
        self.lp8_buffer_timeout = 1.0
        self.bme_buffer = {}
        self.bme_buffer_time = {}
        self.bme_buffer_timeout = 2.0  # segundos
        self.check_bme_buffers()

    def time(self):
        return time.time()

    def setup_device_files(self, device_id):
        device_dir = device_id.replace(":", "_")
        os.makedirs(device_dir, exist_ok=True)
        # ...LP8...
        # BME280
        bme_filename = os.path.join(device_dir, "bme280.txt")
        if not os.path.exists(bme_filename):
            with open(bme_filename, "w") as f:
                f.write("timestamp,temp,hum,presion\n")
            self.app.lp8_files[f"{device_dir}_bme280"] = bme_filename

    def create_info_labels(self, tab):
        # ...tu código para crear labels de estado arriba...
        return {}  # Devuelve el diccionario de labels

    def handle_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode()
        # --- HEARTBEAT ---
        if topic.startswith("esp32/heartbeat/"):
            client_id = topic.split("/")[-1]
            self.app.esp32_devices[client_id] = self.time()
            self.app.root.after(0, self.app.refresh_listbox)
        # --- LP8 ---
        elif topic.startswith("esp32/"):
            parts = topic.split("/")
            # --- LP8 ---
            if len(parts) >= 4 and parts[2].startswith("LP8_") and parts[3] in ["co2", "presion", "vcap1", "vcap2", "errores"]:
                device_id = parts[1]
                if device_id in self.app.tabs:
                    lp8_id = parts[2]
                    var = parts[3]
                    device_dir = device_id.replace(":", "_")
                    file_key = f"{device_dir}_{lp8_id}"
                    filename = os.path.join(device_dir, f"{lp8_id}.txt")
                    now = time.time()
                    now_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(now))
                    if (file_key not in self.lp8_buffer) or (now - self.lp8_buffer_time[file_key] > self.lp8_buffer_timeout):
                        if file_key in self.lp8_buffer:
                            self.flush_lp8_buffer(file_key, filename)
                        self.lp8_buffer[file_key] = {'timestamp': now_str, 'data': {}, 'device_id': device_id}
                        self.lp8_buffer_time[file_key] = now
                    self.lp8_buffer[file_key]['data'][var] = payload
                    if all(k in self.lp8_buffer[file_key]['data'] for k in ["co2", "presion", "vcap1", "vcap2", "errores"]):
                        self.flush_lp8_buffer(file_key, filename)
            # --- BME280 ---
            elif len(parts) >= 4 and parts[2] == "bme280" and parts[3] in ["temp", "hum", "presion"]:
                device_id = parts[1]
                if device_id in self.app.tabs:
                    var = parts[3]
                    device_dir = device_id.replace(":", "_")
                    file_key = f"{device_dir}_bme280"
                    filename = os.path.join(device_dir, "bme280.txt")
                    now = time.time()
                    now_str = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(now))
                    print(f"[DEBUG BME] Recibido {var}={payload} para {device_id} ({file_key})")
                    if (file_key not in self.bme_buffer) or (now - self.bme_buffer_time[file_key] > self.bme_buffer_timeout):
                        if file_key in self.bme_buffer:
                            print(f"[DEBUG BME] Timeout alcanzado, vaciando buffer para {file_key}")
                            self.flush_bme_buffer(file_key, filename)
                        self.bme_buffer[file_key] = {'timestamp': now_str, 'data': {}, 'device_id': device_id}
                        self.bme_buffer_time[file_key] = now
                    self.bme_buffer[file_key]['data'][var] = payload
                    print(f"[DEBUG BME] Buffer actual: {self.bme_buffer[file_key]['data']}")
                    if all(k in self.bme_buffer[file_key]['data'] for k in ["temp", "hum", "presion"]):
                        print(f"[DEBUG BME] Buffer completo, vaciando para {file_key}")
                        self.flush_bme_buffer(file_key, filename)

    def flush_lp8_buffer(self, file_key, filename):
        buf = self.lp8_buffer.get(file_key)
        if not buf:
            return
        # Si falta algún campo, pon -1
        vals = {"co2": "-1", "presion": "-1", "vcap1": "-1", "vcap2": "-1", "errores": "-1"}
        vals.update(buf['data'])
        line = f"{buf['timestamp']},{vals['co2']},{vals['presion']},{vals['vcap1']},{vals['vcap2']},{vals['errores']}\n"
        with open(filename, "a") as f:
            f.write(line)
        device_id = buf.get('device_id', None)
        if device_id:
            self.app.update_plot(device_id)
        del self.lp8_buffer[file_key]
        del self.lp8_buffer_time[file_key]

    def flush_bme_buffer(self, file_key, filename):
        buf = self.bme_buffer.get(file_key)
        if not buf:
            print(f"[DEBUG BME] flush_bme_buffer llamado pero buffer vacío para {file_key}")
            return
        vals = {"temp": "-1", "hum": "-1", "presion": "-1"}
        vals.update(buf['data'])
        line = f"{buf['timestamp']},{vals['temp']},{vals['hum']},{vals['presion']}\n"
        print(f"[DEBUG BME] Guardando línea en {filename}: {line.strip()}")
        with open(filename, "a") as f:
            f.write(line)
        device_id = buf.get('device_id', None)
        if device_id:
            self.app.update_bme_plot(device_id)
        del self.bme_buffer[file_key]
        del self.bme_buffer_time[file_key]

    def check_lp8_buffers(self):
        now = time.time()
        for file_key in list(self.lp8_buffer.keys()):
            t0 = self.lp8_buffer_time[file_key]
            if now - t0 > 5:
                device_dir, lp8_id = file_key.rsplit("_", 1)
                filename = os.path.join(device_dir, f"{lp8_id}.txt")
                self.flush_lp8_buffer(file_key, filename)
        self.app.root.after(1000, self.check_lp8_buffers)

    def check_bme_buffers(self):
        now = time.time()
        for file_key in list(self.bme_buffer.keys()):
            t0 = self.bme_buffer_time[file_key]
            if now - t0 > self.bme_buffer_timeout:
                device_dir = file_key.rsplit("_", 1)[0]
                filename = os.path.join(device_dir, "bme280.txt")
                self.flush_bme_buffer(file_key, filename)
        self.app.root.after(500, self.check_bme_buffers)