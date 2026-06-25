import paho.mqtt.client as mqtt
import json
import struct
import cantools
from datetime import datetime


BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "can/frames"

FRAME_SIZE = 23
FRAMES_PER_BATCH = 32
BATCH_SIZE = FRAME_SIZE * FRAMES_PER_BATCH  # 736 bytes

# Load DBC file
db = cantools.database.load_file('Orion_CANBUV1_1_copy.dbc', strict=False, frame_id_mask=0x1FFFFFFF)
print("DBC loaded successfully")

SIGNAL_MAP = {
    "Pack_Current": "Current",
    "Pack_Inst_Voltage": "Voltage",  # need to update tomatch STATS labels
    "Pack_SOC": "Battery_SOC",
    "High_Temperature": "Temp_High",
    "Low_Temperature": "Temp_Low"  
}

def decode_payload(can_id_int, payload_bytes, dlc):
    try:
        message = db.get_message_by_id(can_id_int)
        decoded = message.decode(payload_bytes[:dlc])
        
        return decoded
    except KeyError:
        return None # CAN ID not found in DBC
    except Exception as e:
        print(f"Error decoding CAN ID {can_id_int}: {e}")
        return None

def parse_batch(batch_bytes):
    frames = []
    for i in range(FRAMES_PER_BATCH):
        frame_bytes = batch_bytes[i*FRAME_SIZE:(i+1)*FRAME_SIZE]
        
        if len(frame_bytes) < FRAME_SIZE:
            print('Incomplete frame issue')
            break
        
        # get raw hex fields
        can_id_hex = frame_bytes[0:4].hex()
        dlc_hex    = frame_bytes[4:6].hex()
        payload    = frame_bytes[15:23]

        # convert to int for cantools lookup
        can_id_int = struct.unpack_from('<I', frame_bytes, 0)[0]
        dlc_int    = struct.unpack_from('<H', frame_bytes, 4)[0]

        # try to decode with DBC
        decoded = decode_payload(can_id_int, payload, dlc_int)

        frame = {
            "can_id":       can_id_hex,
            "can_id_dec":   can_id_int,
            "dlc":          dlc_hex,
            "timestamp_us": frame_bytes[7:15].hex(),
            "payload_hex":  payload.hex(),
            "decoded":      decoded if decoded else "unknown CAN ID"
        }
        frames.append(frame)

    return frames

def on_message(client, userdata, msg):
    data = bytearray(msg.payload)

    # strip trailing 0x0a if present
    if data.endswith(b'\x0a'):
        data = data[:-1]

    if len(data) == BATCH_SIZE:
        frames = parse_batch(data)
        output = {
            "batch_received_at": datetime.now().strftime("%H:%M:%S"),
            "frame_count": len(frames),
            "frames": frames
        }
        print(json.dumps(output, indent=2))
        print("-" * 60)
    else:
        print(f"Unexpected message size: {len(data)} bytes — skipping")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to HiveMQ broker")
        client.subscribe(TOPIC)
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