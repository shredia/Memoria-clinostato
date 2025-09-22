import paho.mqtt.client as mqtt

class MQTTHandler:
    def __init__(self, app):
        self.app = app
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.connect(self.app.sensor_state.BROKER, self.app.sensor_state.PORT, 60)
        import threading
        threading.Thread(target=self.client.loop_forever, daemon=True).start()

    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Conectado al broker MQTT!")
            client.subscribe(self.app.sensor_state.HEARTBEAT_TOPIC)
        else:
            print(f"Error de conexión: {rc}")

    def on_message(self, client, userdata, msg):
        # AQUÍ SE LLAMA A handle_message
        self.app.sensor_state.handle_message(client, userdata, msg)