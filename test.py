import serial
import serial.tools.list_ports
import json
import time

def find_micromax_node():
    """
    Scans all available COM/USB ports, sends a JSON ping, and listens for a valid 
    Micromax signature. Returns the active port and node data if successful.
    """
    print("[Apex] Initiating Phase 1 Discovery Protocol...")
    print("[Apex] Scanning for Micromax nodes on available Serial ports...\n")
    
    # Retrieve a list of all active serial ports on the host machine
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        print(f"[Apex] Testing port: {port.device} - {port.description}")
        try:
            # Open the serial port. Timeout is crucial so the script doesn't hang indefinitely.
            with serial.Serial(port.device, 115200, timeout=2) as ser:
                
                # CRITICAL: Arduinos auto-reset when a serial connection is opened.
                # We must sleep for 2 seconds to let the bootloader finish before talking.
                time.sleep(2) 
                
                # Clear any lingering startup garbage data in the buffer
                ser.reset_input_buffer()
                
                # Construct and send the Discovery Ping (with newline termination)
                ping_payload = json.dumps({"command": "ping"}) + "\n"
                ser.write(ping_payload.encode('utf-8'))
                
                # Wait and read the response
                response_bytes = ser.readline()
                
                if response_bytes:
                    response_str = response_bytes.decode('utf-8').strip()
                    try:
                        # Attempt to parse the Arduino's response as JSON
                        node_data = json.loads(response_str)
                        
                        # Validate the Micromax signature
                        if node_data.get("status") == "ready":
                            print(f"\n[Apex] SUCCESS! Micromax Node found on {port.device}")
                            print(f"[Apex] Node ID: {node_data.get('id')}")
                            print(f"[Apex] Capabilities: {node_data.get('capabilities')}")
                            return port.device, node_data
                            
                    except json.JSONDecodeError:
                        print(f"  -> [Warning] Received non-JSON response: {response_str}")
                else:
                    print("  -> [Timeout] No response received.")
                    
        except Exception as e:
            print(f"  -> [Error] Could not connect to {port.device}: {e}")
            
    print("\n[Apex] Discovery failed. No Micromax nodes found.")
    return None, None

if __name__ == "__main__":
    # Execute the discovery function
    node_port, node_info = find_micromax_node()
    
    if node_port:
        print("\n[Apex System] Sprint 1 (Discovery & Handshake) Complete.")
        print("[Apex System] Ready to proceed to State 2 (Role Assignment).")
    else:
        print("\n[Apex System] Please check USB connections and ensure the Arduino is flashed.")