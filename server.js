const express = require('express');
const http = require('http');
const WebSocket = require('ws');

// Initialize Express App
const app = express();
const port = 3000;

// Serve static files from the "public" directory
app.use(express.static('public'));

// Create HTTP server
const server = http.createServer(app);

// Attach WebSocket server to the HTTP server
const wss = new WebSocket.Server({ server });

// Track connected clients
wss.on('connection', (ws, req) => {
    const clientIp = req.socket.remoteAddress;
    console.log(`[WS] New client connected from ${clientIp}`);

    ws.on('message', (message) => {
        console.log(`[WS] Received: ${message}`);
        
        // Broadcast the message to all other connected clients
        wss.clients.forEach((client) => {
            // We can send to everyone, including sender (useful if UI wants to confirm tare, but usually we just send it to all others)
            // If we want to send to everyone (broker approach):
            if (client.readyState === WebSocket.OPEN) {
                client.send(message.toString());
            }
        });
    });

    ws.on('close', () => {
        console.log(`[WS] Client disconnected`);
    });
    
    ws.on('error', (error) => {
        console.error(`[WS] Error: ${error}`);
    });
});

// Start the server
server.listen(port, '0.0.0.0', () => {
    console.log(`[HTTP] Server is running on http://0.0.0.0:${port}`);
    console.log(`[WS] WebSocket server is attached`);
});
