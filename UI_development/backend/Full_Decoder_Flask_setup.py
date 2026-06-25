from flask import Flask, render_template
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import cantools
import struct
import json
import time
import os
import csv
from datetime import datetime

app = Flask(__name__,
            template_folder=os.path.join(os.path.dirname(__file__), 'templates'),
            static_folder=os.path.join(os.path.dirname(__file__), 'static'),
            static_url_path='')

socketio = SocketIO(app, cors_allowed_origins="*")

# ── CAN Frame Config ──────────────────────────────────────────────────────────
FRAME_SIZE = 23
FRAMES_PER_BATCH = 32
BATCH_SIZE = FRAME_SIZE * FRAMES_PER_BATCH

# ── DBC ───────────────────────────────────────────────────────────────────────
db = cantools.database.load_file('Orion_CANBUV1_1_copy.dbc', strict=False)
print("DBC loaded successfully")
id_to_message = {msg.frame_id: msg for msg in db.messages}

SIGNAL_MAP = {
    "Pack_Current":     "Current",
    "Pack_Inst_Voltage":"Voltage",
    "Pack_SOC":         "Battery_SOC",
    "High_Temperature": "Temp_High",
    "Low_Temperature":  "Temp_Low"
}

# ── Session Log ─────────────────────────────────────────────────────────────
session_log = []  # list of dicts with keys: timestamp, total_elapsed_us,

# ── Timestamp Sync ────────────────────────────────────────────────────────────
startup_wall_us = time.time_ns() // 1000
startup_esp_us  = None
prev_esp_us     = None
current_time_us = None
total_elapsed_us = 0

def esp_to_unix_time(esp_ts_us):
    global startup_esp_us, prev_esp_us, current_time_us,total_elapsed_us

    if startup_esp_us is None:
        startup_esp_us  = esp_ts_us
        prev_esp_us     = esp_ts_us
        current_time_us = startup_wall_us
        total_elapsed_us = 0
        return current_time_us, 0, 0

    delta_us        = esp_ts_us - prev_esp_us
    current_time_us = current_time_us + delta_us
    prev_esp_us     = esp_ts_us
    total_elapsed_us = total_elapsed_us + delta_us
    return current_time_us, delta_us, total_elapsed_us

def format_unix_us(unix_us):
    unix_sec = unix_us / 1_000_000
    dt = datetime.fromtimestamp(unix_sec)
    ms = (unix_us % 1_000_000) // 1000
    us = unix_us % 1000
    return dt.strftime(f"%H:%M:%S.") + f"{ms:03d}.{us:03d}"

# ── Decoder ───────────────────────────────────────────────────────────────────
def decode_payload(can_id_int, payload_bytes, dlc):
    try:
        message = id_to_message.get(can_id_int)
        if not message:
            return None
        return message.decode(payload_bytes[:dlc])
    except Exception as e:
        print(f"Decode error: {e}")
        return None

def parse_batch(batch_bytes):
    for i in range(FRAMES_PER_BATCH):
        frame_bytes = batch_bytes[i*FRAME_SIZE:(i+1)*FRAME_SIZE]

        if len(frame_bytes) < FRAME_SIZE:
            print('Incomplete frame issue')
            break

        can_id_int = struct.unpack_from('<I', frame_bytes, 0)[0]
        dlc_int    = struct.unpack_from('<H', frame_bytes, 4)[0]
        ts_us      = struct.unpack_from('<Q', frame_bytes, 7)[0]
        payload    = frame_bytes[15:23]

        unix_us, delta_us, total_elapsed_us = esp_to_unix_time(ts_us)
        can_log_time_str     = format_unix_us(unix_us)

        decoded = decode_payload(can_id_int, payload, dlc_int)

        if decoded:
            for signal_name, value in decoded.items():
                if signal_name not in SIGNAL_MAP:
                    continue
                dashboard_label = SIGNAL_MAP[signal_name]
               
                session_log.append({ # log each signal emission
                
                "timestamp": "'" + can_log_time_str,
                "total_elapsed_us":  total_elapsed_us,
                "delta_us":          delta_us,
                "signal":            dashboard_label,
                "value":             round(float(value), 2)
                })

                payload_out = json.dumps({
                    "signal":    dashboard_label,
                    "value":     round(float(value), 4),
                    "timestamp": can_log_time_str,
                    "delta_us":  delta_us,
                    "total_elapsed_us": total_elapsed_us
                })
                #print(f"Emitting: {payload_out}")
                socketio.emit("can_data", {"topic": "vehicle/signals", "data": payload_out})

# ── MQTT ──────────────────────────────────────────────────────────────────────
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT   = 1883
MQTT_TOPIC  = "can/frames"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to HiveMQ broker")
        client.subscribe(MQTT_TOPIC)
    else:
        print(f"Connection failed: {rc}")

def on_message(client, userdata, msg):
    data = bytearray(msg.payload)
    if data.endswith(b'\x0a'):
        data = data[:-1]
    if len(data) == BATCH_SIZE:
        parse_batch(data)
    else:
        print(f"Unexpected message size: {len(data)} bytes — skipping")

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
try:
    socketio.run(app, host="0.0.0.0", port=5001, debug=True, use_reloader=False)
except KeyboardInterrupt:
    pass
finally:
    if session_log:
        filename = f"can_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
        with open(filename, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=["timestamp", "total_elapsed_us", "delta_us", "signal", "value"])
            writer.writeheader()
            writer.writerows(session_log)
        print(f"Session saved to {filename}")