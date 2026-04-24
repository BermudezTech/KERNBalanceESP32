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
        const msgStr = message.toString();
        
        try {
            const data = JSON.parse(msgStr);
            if (data.weight !== undefined) {
                console.log(`[WS] Balance Update: ${data.weight} kg`);
            } else if (data.command === 'tare') {
                console.log(`[WS] Tare Command from UI`);
            }
        } catch (e) {
            console.log(`[WS] Received Raw: ${msgStr}`);
        }
        
        // Broadcast the message to all other connected clients
        wss.clients.forEach((client) => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(msgStr);
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
