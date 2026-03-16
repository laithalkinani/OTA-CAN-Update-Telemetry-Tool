from flask import Flask, redirect, url_for, render_template
import os

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), 'static'),
            static_url_path='')

@app.route("/")
def dashboard():
    return render_template("dashboard/index.html")

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
    app.run(debug=True)

'''

from flask import Flask, render_template
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import os

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), '..', 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), '..', 'static'),
            static_url_path='')

socketio = SocketIO(app, cors_allowed_origins="*")

# ── MQTT ──────────────────────────────────────────────────────────────────────
MQTT_BROKER = "localhost"
MQTT_PORT   = 1883
MQTT_TOPIC  = "vehicle/#"

def on_connect(client, userdata, flags, rc):
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
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
    return render_template("dashboard/index.html")

@app.route("/login")
def login():
    return render_template("pages/login.html")

@app.route("/pastlog")
def pastlog():
    return render_template("pastlog/pastlog.html")

@app.route("/checkpoint-login")
def checkpoint_login():
    return render_template("checkpoint_login/checkpoint_login.html")  # fixed path

@app.route("/checkpoint")
def checkpoint():
    return render_template("checkpoint/checkpoint1.html")

@app.route("/message")
def message():
    return render_template("message/message.html")

# ── Run ───────────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000, debug=True)
    '''