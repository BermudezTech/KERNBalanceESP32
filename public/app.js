document.addEventListener('DOMContentLoaded', () => {
    const statusIndicator = document.getElementById('connection-status');
    const statusText = statusIndicator.querySelector('.status-text');
    const weightDisplay = document.getElementById('weight-display');
    const tareBtn = document.getElementById('tare-btn');

    let ws = null;
    let reconnectInterval = null;

    function connect() {
        // Assume ws port is same as http
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}`;
        
        ws = new WebSocket(wsUrl);

        ws.onopen = () => {
            console.log('Connected to server');
            statusText.textContent = 'Waiting for Balance...';
            if (reconnectInterval) {
                clearInterval(reconnectInterval);
                reconnectInterval = null;
            }
        };

        let balanceTimeout = null;

        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                
                if (data.weight !== undefined) {
                    // Update display
                    updateWeight(data.weight);

                    // Update connection status
                    statusIndicator.classList.add('connected');
                    statusText.textContent = 'Connected';

                    // Reset activity timer
                    clearTimeout(balanceTimeout);
                    balanceTimeout = setTimeout(() => {
                        statusIndicator.classList.remove('connected');
                        statusText.textContent = 'Balance Offline';
                    }, 3000);
                }
            } catch (err) {
                console.error('Failed to parse WS message:', err);
            }
        };

        ws.onclose = () => {
            console.log('Disconnected from server');
            statusIndicator.classList.remove('connected');
            statusText.textContent = 'Server Offline';
            clearTimeout(balanceTimeout);
            
            // Try to reconnect
            if (!reconnectInterval) {
                reconnectInterval = setInterval(connect, 3000);
            }
        };


        ws.onerror = (err) => {
            console.error('WebSocket Error:', err);
        };
    }

    function updateWeight(weightStr) {
        // Parse float and fix to 3 decimals, then pad left to make 4 digits before decimal
        // E.g., '12.5' -> '12.500' -> '0012.500'
        const w = Number.parseFloat(weightStr);
        if (isNaN(w)) {
            weightDisplay.textContent = "ERR .   ";
            return;
        }

        const formatted = w.toFixed(3).padStart(8, '0');
        weightDisplay.textContent = formatted;
    }

    tareBtn.addEventListener('click', () => {
        if (ws && ws.readyState === WebSocket.OPEN) {
            console.log('Sending TARE command');
            ws.send(JSON.stringify({ command: 'tare' }));
            
            // Temporary visual feedback
            tareBtn.style.transform = 'scale(0.98)';
            setTimeout(() => {
                tareBtn.style.transform = 'none';
            }, 100);
        } else {
            console.warn('Cannot TARE, disconnected.');
        }
    });

    // Initial connection
    connect();
});
