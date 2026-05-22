import paho.mqtt.client as mqtt
import json
from datetime import datetime

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "can/frames"

FRAME_SIZE = 23
FRAMES_PER_BATCH = 32
BATCH_SIZE = FRAME_SIZE * FRAMES_PER_BATCH  # 736 bytes

def parse_batch(batch_bytes):
    frames = []
    for i in range(FRAMES_PER_BATCH):
        frame_bytes = batch_bytes[i*FRAME_SIZE:(i+1)*FRAME_SIZE]
        
        if len(frame_bytes) < FRAME_SIZE:
            print('Incomplete frame issue')
            break

        frame = {
            "can_id":       frame_bytes[0:4].hex(),
            "dlc":          frame_bytes[4:6].hex(),
            "flags":        frame_bytes[6:7].hex(),
            "timestamp_us": frame_bytes[7:15].hex(),
            "payload_hex":  frame_bytes[15:23].hex()
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