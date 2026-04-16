# KERN Balance ESP32 WebSocket Dashboard

This project provides a real-time web interface for monitoring weight data from an ESP32-connected scale (specifically designed for KERN balances). It uses Node.js, Express, and WebSockets to enable bi-directional communication between the hardware and the browser.

## Features

- **Real-time Weight Display**: A premium, high-contrast digital display using a 7-segment font.
- **Bi-directional WebSockets**: Receive weight data from the ESP32 and send commands (like Tare) back.
- **Responsive Design**: Works on desktops, tablets, and mobile devices.
- **Network Exposable**: Automatically binds to `0.0.0.0` for access across your local network.

## Project Structure

- `server.js`: The Node.js/Express server that handles HTTP requests and acts as a WebSocket broker.
- `public/`: Contains the frontend assets (HTML, CSS, JS).
- `scratch/fake_esp32.js`: A simulation script to test the UI without physical hardware.

## Quick Start

### 1. Install Dependencies

```bash
npm install
```

### 2. Start the Server

```bash
npm start
```

The dashboard will be available at `http://localhost:3000` (or `http://[YOUR_IP]:3000` on your network).

### 3. Testing with Fake Data

If you don't have the ESP32 connected yet, you can simulate it:

```bash
node scratch/fake_esp32.js
```

## WebSocket Protocol

### Weight Update (ESP32 -> UI)
The ESP32 should send a JSON string:
```json
{ "weight": "123.456" }
```

### Tare Command (UI -> ESP32)
When the Tare button is clicked, the UI sends:
```json
{ "command": "tare" }
```

## License

MIT
