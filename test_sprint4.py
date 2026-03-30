import paho.mqtt.client as mqtt
import json
import time

# --- Network & MQTT Settings ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
TOPIC_CMD = "apex/micromax/01/commands"
TOPIC_STATUS = "apex/micromax/01/status"
TOPIC_TELEM = "apex/micromax/01/telemetry"

def on_connect(client, userdata, flags, rc):
    print(f"[Apex] Connected to Global MQTT Broker with result code {rc}")
    # Subscribe to status and telemetry topics to hear the board's responses
    client.subscribe(TOPIC_STATUS)
    client.subscribe(TOPIC_TELEM)
    print(f"[Apex] Subscribed to {TOPIC_STATUS} and {TOPIC_TELEM}")

def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    print(f"\n  -> [Incoming on {msg.topic}]: {payload}")

def run_sprint_4_test():
    print("[Apex] Initiating Wireless Diagnostics (Sprint 4)...")
    
    # Setup MQTT Client
    client = mqtt.Client(client_id="Apex_Master_Node_01")
    client.on_connect = on_connect
    client.on_message = on_message
    
    # Connect to the broker
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    
    # Start a background thread to process network traffic continuously
    client.loop_start()
    
    # Give the connection a moment to establish
    time.sleep(2)
    
    print("\n[Apex] Step 1: Sending Discovery Ping...")
    client.publish(TOPIC_CMD, json.dumps({"command": "ping"}))
    time.sleep(2)

    print("\n[Apex] Step 2: Assigning 'Sentry' Role (Listening for 6 seconds)...")
    client.publish(TOPIC_CMD, json.dumps({"command": "set_role", "role": "sentry"}))
    time.sleep(6) # Wait to receive a few telemetry pulses

    print("\n[Apex] Step 3: Assigning 'Action' Role (Triggering built-in LED)...")
    client.publish(TOPIC_CMD, json.dumps({"command": "set_role", "role": "action"}))
    time.sleep(1)
    
    print("[Apex] Triggering LED ON")
    client.publish(TOPIC_CMD, json.dumps({"command": "trigger", "pin": 2, "state": "HIGH"}))
    time.sleep(3)
    
    print("[Apex] Triggering LED OFF")
    client.publish(TOPIC_CMD, json.dumps({"command": "trigger", "pin": 2, "state": "LOW"}))
    time.sleep(1)

    print("\n[Apex System] Sprint 4 Wireless Diagnostics Complete.")
    client.loop_stop()

if __name__ == "__main__":
    run_sprint_4_test()