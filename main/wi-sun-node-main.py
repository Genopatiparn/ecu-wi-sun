import socket
import threading

UDP_IP = "::"
UDP_PORT = 1234

# Node directory
nodes = {
    "Node 1": {"ip": "fd12:3456::da7a:3bff:fe41:991f", "port": 5555},
    "Node 2": {"ip": "fd12:3456::b635:22ff:fe98:2462", "port": 5555}
}

sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

def receive_task():
    while True:
        try:
            data, addr = sock.recvfrom(1024)
            sender_ip = addr[0]
            message = data.decode('utf-8').strip()

            sender_name = "Unknown Node"
            for name, info in nodes.items():
                if info["ip"] == sender_ip:
                    sender_name = name
                    break
            
            print(f"\n[Received from {sender_name}] {message}")
            print(">>> Target:Message -> ", end="", flush=True)
        except Exception as e:
            print(f"\n[Error] {e}")
            print(">>> Target:Message -> ", end="", flush=True)

rx_thread = threading.Thread(target=receive_task, daemon=True)
rx_thread.start()

print("\n--- Wi-SUN Pi Command Center ---")
print("Format -> TargetName:Your Message")
print("Single   -> Node 1:Hello")
print("Multiple -> Node 1,Node 2:coffee")
print("Broadcast-> all:Hi everyone")
print("----------------------------------")

while True:
    try:
        user_input = input(">>> Target:Message -> ")
        
        if ":" in user_input:
            target_name, message = user_input.split(":", 1)
            target_name = target_name.strip()
            
            # Support broadcast
            if target_name.lower() in ["all", "broadcast"]:
                print("\n[Broadcasting to all nodes...]")
                sent_count = 0
                for name, info in nodes.items():
                    sock.sendto(message.encode('utf-8'), (info["ip"], info["port"]))
                    print(f"  -> Sent to {name}")
                    sent_count += 1
                print(f"[Broadcast complete: {sent_count} nodes]")
            elif target_name in nodes:
                target_ip = nodes[target_name]["ip"]
                target_port = nodes[target_name]["port"]
                sock.sendto(message.encode('utf-8'), (target_ip, target_port))
                print(f"[Sent to {target_name}] Success")
            else:
                # Support multiple targets
                targets = [t.strip() for t in target_name.split(",")]
                sent_count = 0
                not_found_count = 0
                
                for target in targets:
                    if target in nodes:
                        target_ip = nodes[target]["ip"]
                        target_port = nodes[target]["port"]
                        sock.sendto(message.encode('utf-8'), (target_ip, target_port))
                        print(f"[Sent to {target}] Success")
                        sent_count += 1
                    else:
                        print(f"[!] Error: Node '{target}' not found!")
                        not_found_count += 1
                
                if sent_count > 0:
                    print(f"[Summary: {sent_count} sent", end="")
                    if not_found_count > 0:
                        print(f", {not_found_count} failed", end="")
                    print("]")
        else:
            print("[!] Invalid format. Use 'Name:Message'")
    except KeyboardInterrupt:
        print("\n[Exiting...]")
        break
