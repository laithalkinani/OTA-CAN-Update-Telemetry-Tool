import paho.mqtt.client as mqtt
import json
import struct
import cantools
import time
from datetime import datetime, timedelta

# sync on startup
startup_wall_us = time.time_ns() // 1000  # current unix time in microseconds
session_start_wall = None   # wall clock time of first frame
session_start_us = None     # ESP32 timestamp of first frame
prev_timestamp_us = None    # for delta calculation
startup_esp_us  = None
prev_esp_us     = None
current_time_us = None

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC_IN = "can/frames"
TOPIC_OUT = "vehicle/signals"

FRAME_SIZE = 23
FRAMES_PER_BATCH = 32
BATCH_SIZE = FRAME_SIZE * FRAMES_PER_BATCH  # 736 bytes

# Load DBC file
db = cantools.database.load_file('Orion_CANBUV1_1_copy.dbc', strict=False)

print("DBC loaded successfully")

id_to_message = {msg.frame_id: msg for msg in db.messages}

SIGNAL_MAP = {
    "Pack_Current": "Current",
    "Pack_Inst_Voltage": "Voltage",  # need to update tomatch STATS labels
    "Pack_SOC": "Battery_SOC",
    "High_Temperature": "Temp_High",
    "Low_Temperature": "Temp_Low"  
}

def esp_to_unix_time(esp_ts_us):
    global startup_esp_us, prev_esp_us, current_time_us

    # first message — sync ESP32 clock to unix time
    if startup_esp_us is None:
        startup_esp_us = esp_ts_us
        prev_esp_us = esp_ts_us
        current_time_us = startup_wall_us
        return current_time_us, 0  # delta is 0 for first message

    # delta between this message and previous
    delta_us = esp_ts_us - prev_esp_us

    # advance current time by delta
    current_time_us = current_time_us + delta_us

    # update previous
    prev_esp_us = esp_ts_us

    return current_time_us, delta_us

def format_unix_us(unix_us):
    # convert microseconds to seconds for datetime
    unix_sec = unix_us / 1_000_000
    dt = datetime.fromtimestamp(unix_sec)
    return dt.strftime("%H:%M:%S.%f")[:-3]  # trim to milliseconds


def decode_payload(can_id_int, payload_bytes, dlc):
    try:
        message = id_to_message.get(can_id_int)
        if not message:
            return None
        decoded = message.decode(payload_bytes[:dlc])
        return decoded
    except Exception as e:
        print(f"Decode error: {e}")
        return None
'''
def publish_signals(decoded, publish_client):
    timestamp = datetime.now().strftime("%H:%M:%S")
    for signal_name, value in decoded.items():
        dashboard_label = SIGNAL_MAP.get(signal_name, signal_name)
        msg = json.dumps({
            "signal": dashboard_label,
            "value": round(float(value), 2),
            "timestamp": timestamp
            "delta_us": delta_us
        })
        publish_client.publish(TOPIC_OUT, msg)
        print(f"Published: {msg}")'''

def publish_signals(decoded, publish_client, standard_time_str, delta_us):
    for signal_name, value in decoded.items():
        if signal_name not in SIGNAL_MAP:
            continue
        dashboard_label = SIGNAL_MAP[signal_name]
        msg = json.dumps({
            "signal": dashboard_label,
            "value": round(float(value), 2),
            "timestamp": standard_time_str,
            "delta_us": delta_us
        })
        publish_client.publish(TOPIC_OUT, msg)
        print(f"Published: {msg}")

def parse_batch(batch_bytes, publish_client):
    for i in range(FRAMES_PER_BATCH):
        frame_bytes = batch_bytes[i*FRAME_SIZE:(i+1)*FRAME_SIZE]

        if len(frame_bytes) < FRAME_SIZE:
            print('Incomplete frame issue')
            break

        can_id_int = struct.unpack_from('<I', frame_bytes, 0)[0]
        dlc_int    = struct.unpack_from('<H', frame_bytes, 4)[0]
        ts_us      = struct.unpack_from('<Q', frame_bytes, 7)[0]
        payload    = frame_bytes[15:23]

        unix_us, delta_us = esp_to_unix_time(ts_us)
        can_log_time_str = format_unix_us(unix_us)

        decoded = decode_payload(can_id_int, payload, dlc_int)

        if decoded:
            publish_signals(decoded, publish_client, can_log_time_str, delta_us)
'''
def parse_batch(batch_bytes, publish_client):
    for i in range(FRAMES_PER_BATCH):
        frame_bytes = batch_bytes[i*FRAME_SIZE:(i+1)*FRAME_SIZE]

        if len(frame_bytes) < FRAME_SIZE:
            print('Incomplete frame issue')
            break

        can_id_int = struct.unpack_from('<I', frame_bytes, 0)[0]
        dlc_int    = struct.unpack_from('<H', frame_bytes, 4)[0]
        payload    = frame_bytes[15:23]
        time_

        decoded = decode_payload(can_id_int, payload, dlc_int)

        if decoded:
            publish_signals(decoded, publish_client)
'''
def on_message(client, userdata, msg):
    data = bytearray(msg.payload)

    if data.endswith(b'\x0a'):
        data = data[:-1]

    if len(data) == BATCH_SIZE:
        parse_batch(data, client)
    else:
        print(f"Unexpected message size: {len(data)} bytes — skipping")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to HiveMQ broker")
        client.subscribe(TOPIC_IN)
    else:
        print(f"Connection failed with code {rc}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT)

print("Waiting for CAN frames...")
try:
    client.loop_forever()
except KeyboardInterrupt:
    print("Stopped")
    client.disconnect()


