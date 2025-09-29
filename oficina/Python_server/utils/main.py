import tkinter as tk
from tkinter import messagebox
from app import App

def show_config_window():
    config_win = tk.Tk()
    config_win.title("Configuración MQTT")

    tk.Label(config_win, text="Dirección IP del broker:").grid(row=0, column=0, padx=10, pady=5)
    ip_entry = tk.Entry(config_win)
    ip_entry.insert(0, "192.168.31.61")  # Valor por defecto
    ip_entry.grid(row=0, column=1, padx=10, pady=5)

    tk.Label(config_win, text="Puerto:").grid(row=1, column=0, padx=10, pady=5)
    port_entry = tk.Entry(config_win)
    port_entry.insert(0, "1883")  # Valor por defecto
    port_entry.grid(row=1, column=1, padx=10, pady=5)

    def on_connect():
        ip = ip_entry.get().strip()
        try:
            port = int(port_entry.get().strip())
        except ValueError:
            messagebox.showerror("Error", "El puerto debe ser un número entero.")
            return
        config_win.destroy()
        start_main_app(ip, port)

    tk.Button(config_win, text="Conectar", command=on_connect).grid(row=2, column=0, columnspan=2, pady=10)
    config_win.mainloop()

def start_main_app(ip, port):
    root = tk.Tk()
    app = App(root, broker_ip=ip, broker_port=port)
    root.mainloop()

if __name__ == "__main__":
    show_config_window()