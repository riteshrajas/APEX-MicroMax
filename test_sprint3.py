import serial
import serial.tools.list_ports
import json
import time

def find_micromax_port():
    """Quickly finds the first available Micromax port based on our Sprint 1 logic."""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        try:
            with serial.Serial(port.device, 115200, timeout=2) as ser:
                time.sleep(2)
                ser.reset_input_buffer()
                ser.write(b'{"command": "ping"}\n')
                response = ser.readline().decode('utf-8').strip()
                if "micromax_uno_01" in response:
                    return port.device
        except Exception:
            pass
    return None

def send_command(ser, command_dict):
    """Helper function to send JSON and read the immediate response."""
    payload = json.dumps(command_dict) + "\n"
    ser.write(payload.encode('utf-8'))
    response = ser.readline().decode('utf-8').strip()
    try:
        return json.loads(response)
    except:
        return {"raw_response": response}

def run_sprint_3_test():
    print("[Apex] Locating Micromax Node (Arduino Uno)...")
    port = find_micromax_port()
    
    if not port:
        print("[Apex Error] Node not found. Is it plugged in and the Serial Monitor closed?")
        return

    print(f"[Apex] Connected to {port}. Initiating Sprint 3 Diagnostics (AHT20).\n")
    
    with serial.Serial(port, 115200, timeout=1) as ser:
        time.sleep(2) # Wait for bootloader
        ser.reset_input_buffer()

        # --- Test 1: Sentry Mode (Reading the AHT20) ---
        print("--- Initiating Sentry Mode Test ---")
        reply = send_command(ser, {"command": "set_role", "role": "sentry"})
        print(f"Node Reply: {reply}")
        
        print("[Apex] Listening for AHT20 telemetry for 10 seconds...")
        end_time = time.time() + 10
        while time.time() < end_time:
            line = ser.readline().decode('utf-8').strip()
            if line:
                print(f"  -> Incoming Data: {line}")

        # --- Test 2: Action Mode ---
        print("\n--- Initiating Action Mode Test ---")
        reply = send_command(ser, {"command": "set_role", "role": "action"})
        print(f"Node Reply: {reply}")
        
        print("[Apex] Triggering Built-in LED (Pin 13) ON...")
        reply = send_command(ser, {"command": "trigger", "pin": 13, "state": "HIGH"})
        print(f"Node Reply: {reply}")
        
        time.sleep(2) # Keep the LED on for 2 seconds
        
        print("[Apex] Triggering Built-in LED (Pin 13) OFF...")
        reply = send_command(ser, {"command": "trigger", "pin": 13, "state": "LOW"})
        print(f"Node Reply: {reply}")

        print("\n[Apex System] Sprint 3 Diagnostics Complete.")

if __name__ == "__main__":
    run_sprint_3_test()