
from flask import Flask, redirect, url_for, render_template
import os

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), 'static'),
            static_url_path='')

@app.route("/")
def dashboard():
    return render_template("dashboard/index copy.html")

@app.route("/login")
def login():
    return render_template("pages/login.html")

@app.route("/pastlog")
def pastlog():
    return render_template("pastlog/pastlog.html")

@app.route("/checkpoint-login")
def checkpoint_login():
    return render_template("checkpoint_login/checkpoint_login.html") 

@app.route("/checkpoint")  
def checkpoint():
    return render_template("checkpoint/checkpoint1.html")

@app.route("/message")
def message():
    return render_template("message/message.html")

if __name__ == "__main__":
    app.run(debug=True, port=5002)



'''
from flask import Flask, render_template
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import os

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), 'static'),
            static_url_path='')

socketio = SocketIO(app, cors_allowed_origins="*")

# ── MQTT ──────────────────────────────────────────────────────────────────────
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT   = 1883
MQTT_TOPIC  = "vehicle/signals"

def on_connect(client, userdata, flags, rc):
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    print(f"Flask received: {msg.topic} -> {msg.payload.decode()}")
    socketio.emit("can_data", {
        "topic": msg.topic,
        "data":  msg.payload.decode()
    })

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT)
mqtt_client.loop_start()

# ── Routes ────────────────────────────────────────────────────────────────────
@app.route("/")
def dashboard():
    return render_template("dashboard/index copy.html")

@app.route("/login")
def login():
    return render_template("pages/login.html")

@app.route("/pastlog")
def pastlog():
    return render_template("pastlog/pastlog.html")

@app.route("/checkpoint-login")
def checkpoint_login():
    return render_template("checkpoint_login/checkpoint_login.html") 

@app.route("/checkpoint")  
def checkpoint():
    return render_template("checkpoint/checkpoint1.html")

@app.route("/message")
def message():
    return render_template("message/message.html")

# ── Run ───────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5001, debug=True)
   '''