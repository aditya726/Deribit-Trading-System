# Deribit Trading System

A C++ trading application for connecting to the Deribit cryptocurrency derivatives exchange, featuring real-time market data streaming and order management.

## Features

- **Real-time WebSocket Connection** to Deribit exchange
- **Order Management** - Place, cancel, and modify orders
- **Market Data Streaming** - Live orderbook updates for BTC-PERPETUAL and ETH-PERPETUAL
- **Position Tracking** - View positions by currency and instrument type
- **WebSocket Server** - Broadcast market data to connected clients
- **Secure TLS/SSL Support** - Encrypted connections for both client and server
- **Interactive Menu System** - Console-based interface for all operations
- **Multi-threading Support** - Asynchronous message handling

## Installation

### Prerequisites

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libboost-all-dev libssl-dev nlohmann-json3-dev
sudo apt install libwebsocketpp-dev
```

#### Windows (with vcpkg)
```bash
# Install vcpkg first
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install dependencies
.\vcpkg install boost:x64-windows
.\vcpkg install openssl:x64-windows
.\vcpkg install nlohmann-json:x64-windows
.\vcpkg install websocketpp:x64-windows
```

### Build Instructions

#### Ubuntu/Debian
```bash
git clone <repository-url>
cd deribit-trading-system
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### Windows (Visual Studio)
```bash
git clone <repository-url>
cd deribit-trading-system
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Configuration

1. Update `main.cpp` with your Deribit API credentials:
   ```cpp
   const std::string client_id = "your_client_id";
   const std::string client_secret = "your_client_secret";
   ```

2. For SSL server functionality, provide certificate paths:
   ```cpp
   const std::string cert_file = "/path/to/certificate.pem";
   const std::string key_file = "/path/to/private_key.pem";
   ```

### Usage

Run the executable and use the interactive menu to:
- Place and manage orders
- Subscribe to market data feeds
- View real-time orderbooks
- Monitor positions
- Access WebSocket data stream on port 9002

**Note**: The application connects to Deribit's test environment by default. Update the WebSocket URL in `deribit_client.cpp` for production use.
