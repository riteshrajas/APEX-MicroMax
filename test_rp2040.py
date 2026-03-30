import serial
import serial.tools.list_ports
import json
import time

def find_micromax_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        try:
            with serial.Serial(port.device, 115200, timeout=2) as ser:
                time.sleep(2)
                ser.reset_input_buffer()
                ser.write(b'{"command": "ping"}\n')
                response = ser.readline().decode('utf-8').strip()
                if "micromax_rp2040_01" in response:
                    return port.device
        except Exception:
            pass
    return None

def send_command(ser, command_dict):
    payload = json.dumps(command_dict) + "\n"
    ser.write(payload.encode('utf-8'))
    response = ser.readline().decode('utf-8').strip()
    try:
        return json.loads(response)
    except:
        return {"raw_response": response}

def run_rp2040_test():
    print("[Apex] Locating Micromax Node (RP2040)...")
    port = find_micromax_port()
    
    if not port:
        print("[Apex Error] Node not found. Ensure the RP2040 is plugged in and monitor is closed.")
        return

    print(f"[Apex] Connected to {port}. Initiating RP2040 Diagnostics.\n")
    
    with serial.Serial(port, 115200, timeout=1) as ser:
        time.sleep(2)
        ser.reset_input_buffer()

        # --- Test 1: Sentry Mode (Internal CPU Temp) ---
        print("--- Initiating Sentry Mode Test (Core Temp) ---")
        reply = send_command(ser, {"command": "set_role", "role": "sentry"})
        print(f"Node Reply: {reply}")
        
        print("[Apex] Listening for RP2040 internal telemetry for 6 seconds...")
        end_time = time.time() + 6
        while time.time() < end_time:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"  -> Incoming Data: {line}")

        # --- Test 2: Action Mode (NeoPixel RGB Control) ---
        print("\n--- Initiating Action Mode Test (NeoPixel) ---")
        reply = send_command(ser, {"command": "set_role", "role": "action"})
        print(f"Node Reply: {reply}")
        
        print("[Apex] Triggering NeoPixel: RED")
        reply = send_command(ser, {"command": "trigger", "target": "neopixel", "r": 255, "g": 0, "b": 0})
        print(f"Node Reply: {reply}")
        time.sleep(1.5)
        
        print("[Apex] Triggering NeoPixel: BLUE")
        reply = send_command(ser, {"command": "trigger", "target": "neopixel", "r": 0, "g": 0, "b": 255})
        print(f"Node Reply: {reply}")
        time.sleep(1.5)

        print("[Apex] Triggering NeoPixel: OFF")
        reply = send_command(ser, {"command": "trigger", "target": "neopixel", "r": 0, "g": 0, "b": 0})
        print(f"Node Reply: {reply}")

        print("\n[Apex System] RP2040 Diagnostics Complete.")

if __name__ == "__main__":
    run_rp2040_test()