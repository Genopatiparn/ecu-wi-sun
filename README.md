# Wi-SUN Multi-Node Communication System

Wi-SUN mesh network communication system with Main controller (Raspberry Pi) and multiple nodes (Silicon Labs EFR32FG25).

## 🌐 System Architecture

```
┌─────────────────────────────────────────────────┐
│         Wi-SUN Mesh Network (fd12:3456::/64)    │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌──────────────┐         ┌──────────────┐     │
│  │   Node 1     │◄───────►│   Node 2     │     │
│  │ Port: 5555   │         │ Port: 5555   │     │
│  │ (EFR32FG25)  │         │ (EFR32FG25)  │     │
│  └──────┬───────┘         └──────┬───────┘     │
│         │                        │             │
│         └────────┬───────────────┘             │
│                  │                             │
│         ┌────────▼────────┐                    │
│         │  Main (Pi)      │                    │
│         │  Port: 1234     │                    │
│         │  Border Router  │                    │
│         └─────────────────┘                    │
└─────────────────────────────────────────────────┘
```

## 📋 Features

### ✅ Multi-Node Messaging
- **Single target**: `Node 1:Hello`
- **Multiple targets**: `Node 1,Node 2:coffee`
- **Broadcast**: `all:Hi everyone`

### ✅ Peer-to-Peer Communication
- Nodes can send messages directly to each other
- No need to wait for Main to initiate
- Real-time bidirectional communication

### ✅ Network Components
- **Main (Raspberry Pi)**: Border Router + Python UDP server
- **Node 1 & 2 (EFR32FG25)**: Wi-SUN nodes with custom UDP implementation

## 🛠️ Hardware Requirements

- **Raspberry Pi** (Border Router)
  - Wi-SUN Border Router daemon (wsbrd)
  - CPC daemon (cpcd)
  
- **Silicon Labs EFR32FG25** (x2)
  - BRD4270B Radio Board
  - BRD4002A WSTK Mainboard

## 📦 Software Requirements

### Main (Raspberry Pi)
- Python 3.x
- Wi-SUN Border Router (wsbrd)
- CPC Daemon (cpcd)

### Nodes (EFR32FG25)
- Simplicity Studio 5
- Gecko SDK 2025.6.3
- Wi-SUN Stack 2.9.2

## 🚀 Getting Started

### 1. Setup Border Router (Main)

```bash
# Start CPC daemon
sudo cpcd -c /path/to/cpcd.conf

# Start Wi-SUN Border Router
sudo wsbrd -F /path/to/wsbrd.conf

# Run Python script
cd main
python3 wi-sun-node-main.py
```

### 2. Build and Flash Nodes

**Using Simplicity Studio:**
1. Open `ECU-Shop-Node-001` or `ECU-Shop-Node-002`
2. Build Project
3. Flash to Device

### 3. Usage Examples

**From Main:**
```
>>> Node 1:Start brewing coffee
>>> all:System check
>>> Node 1,Node 2:Status update
```

**From Node 1:**
```
>>> Main:Coffee ready
>>> Node 2:Temperature OK
>>> all:System online
```

**From Node 2:**
```
>>> Main:Task completed
>>> Node 1:Acknowledged
```

## 📁 Project Structure

```
.
├── ECU-Shop-Node-001/          # Node 1 firmware
│   ├── app.c                   # Main application
│   ├── app.h
│   ├── app_init.c
│   ├── app_custom_callback.c
│   └── ECU-Shop-Node-001.slcp  # Project config
│
├── ECU-Shop-Node-002/          # Node 2 firmware
│   ├── app.c                   # Main application
│   ├── app.h
│   ├── app_init.c
│   ├── app_custom_callback.c
│   └── ECU-Shop-Node-002.slcp  # Project config
│
└── main/                       # Border Router scripts
    ├── wi-sun-node-main.py     # Python UDP server
    ├── wsbrd.conf              # Border Router config
    └── cpcd.conf               # CPC daemon config
```

## 🔧 Configuration

### Network Settings

**IPv6 Prefix**: `fd12:3456::/64`

**Node Addresses:**
- Main: `fd12:3456::b635:22ff:fe98:2478` (Port 1234)
- Node 1: `fd12:3456::da7a:3bff:fe41:991f` (Port 5555)
- Node 2: `fd12:3456::b635:22ff:fe98:2462` (Port 5555)

### Wi-SUN Settings
- **Network Name**: Wi-SUN Network
- **Domain**: NA (North America)
- **Channel Plan**: 1
- **PHY Mode**: 2

## 🐛 Troubleshooting

### Node not connecting
- Check Wi-SUN Border Router is running
- Verify network name matches in all devices
- Check IPv6 addresses are correct

### Messages not received
- Verify UDP ports (1234 for Main, 5555 for Nodes)
- Check firewall settings
- Ensure all devices are on same Wi-SUN network

### Build errors
- Clean project before building
- Verify Gecko SDK version (2025.6.3)
- Check all required components are installed

## 📝 License

This project uses Silicon Labs SDK components licensed under Zlib license.

## 🤝 Contributing

Feel free to submit issues and enhancement requests!

## 📧 Contact

For questions and support, please open an issue on GitHub.
