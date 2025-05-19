import socket
import os
from datetime import datetime
import tkinter as tk
from threading import Thread
import json

HOST = '0.0.0.0'
PORT = 8080
MAX_BUFFER_SIZE = 4096

count = 0
server_running = False

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Servidor TCP - Monitor de Datos")
        self.geometry("400x300")

        self.label_status = tk.Label(self, text="Servidor detenido.", font=("Arial", 12))
        self.label_status.pack(pady=5)

        self.label_count = tk.Label(self, text="Datos recibidos: 0", font=("Arial", 12))
        self.label_count.pack(pady=5)

        self.text_data = tk.Text(self, height=8, width=40, state=tk.DISABLED)
        self.text_data.pack(pady=5)

        self.button_start = tk.Button(self, text="Iniciar", font=("Arial", 12), command=self.start_server)
        self.button_start.pack(pady=5)

        self.button_stop = tk.Button(self, text="Detener", font=("Arial", 12), command=self.stop_server, state=tk.DISABLED)
        self.button_stop.pack(pady=5)

        self.protocol("WM_DELETE_WINDOW", self.on_close)
        self.server_thread = None

    def start_server(self):
        global server_running
        if not server_running:
            self.server_thread = Thread(target=self.run_server)
            self.server_thread.daemon = True
            self.server_thread.start()
            self.button_start.config(state=tk.DISABLED)
            self.button_stop.config(state=tk.NORMAL)

    def run_server(self):
        global count, server_running
        server_running = True
        file_path = os.path.abspath("./datos_mpu6050.txt")
        self.update_status(f"Escuchando en {HOST}:{PORT}...")

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s, open(file_path, "a") as file:
            s.bind((HOST, PORT))
            s.listen(1)

            while server_running:
                try:
                    s.settimeout(1)
                    conn, addr = s.accept()
                    self.update_status(f"Conexión establecida desde {addr}")

                    with conn:
                        buffer = ""
                        while server_running:
                            data = conn.recv(1024)
                            if not data:
                                break

                            buffer += data.decode()

                            if len(buffer) > MAX_BUFFER_SIZE:
                                buffer = ""

                            while "}" in buffer:
                                json_str, buffer = buffer.split("}", 1)
                                json_str += "}"
                                timestamp = datetime.now().strftime("[%Y-%m-%d %H:%M:%S]")
                                count += 1

                                log_line = f"{timestamp} {json_str}"
                                self.update_count(count)
                                file.write(log_line + "\n")
                                file.flush()

                                self.display_data(log_line)

                except socket.timeout:
                    continue
                except Exception as e:
                    print(f"Error: {e}")
                    break

        self.update_status("Servidor detenido.")

    def display_data(self, data):
        self.text_data.config(state=tk.NORMAL)
        self.text_data.insert(tk.END, data + "\n")
        self.text_data.see(tk.END)
        self.text_data.config(state=tk.DISABLED)

    def stop_server(self):
        global server_running
        server_running = False
        self.button_start.config(state=tk.NORMAL)
        self.button_stop.config(state=tk.DISABLED)
        self.update_status("Servidor detenido.")

    def update_status(self, message):
        self.label_status.config(text=message)

    def update_count(self, count):
        self.label_count.config(text=f"Datos recibidos: {count}")

    def on_close(self):
        self.stop_server()
        self.destroy()

if __name__ == "__main__":
    app = App()
    app.mainloop()
