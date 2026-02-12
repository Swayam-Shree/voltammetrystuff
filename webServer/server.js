const WebSocket = require('ws');
const express = require('express');
const http = require('http');
const path = require('path');
const cors = require('cors');

const app = express();

app.use(cors());
app.use(express.static(path.join(__dirname, 'public/dist')));
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = process.env.PORT || 6969;

let espSocket = null;
let browserSocket = null;

let idCounter = 0;

wss.on('connection', (socket) => {
    socket.id = idCounter;
    ++idCounter;
    console.log('Client connected', socket.id);

    socket.on('message', (message) => {
        let parsed;
        try {
            parsed = JSON.parse(message);
        } catch (e) {
            console.error('Failed to parse message:', e.message);
            return;
        }

        if (!parsed) return;

        switch (parsed.type) {
            case "init":
                switch (parsed.data) {
                    case "espInit":
                        if (espSocket && espSocket !== socket) {
                            console.warn("Replacing existing ESP connection");
                        }
                        espSocket = socket;
                        console.log("ESP connected");
                        break;
                    case "browserInit":
                        if (browserSocket && browserSocket !== socket) {
                            console.warn("Replacing existing browser connection");
                        }
                        browserSocket = socket;
                        console.log("Browser connected");
                        break;
                }
                break;

            case "voltammetryResult":
                if (browserSocket) {
                    browserSocket.send(JSON.stringify(parsed));
                }
                break;
            case "processingState":
                if (browserSocket) {
                    browserSocket.send(JSON.stringify(parsed));
                }
                break;
        }
    });

    socket.on('close', () => {
        console.log('Client disconnected', socket.id);
        if (socket === espSocket) {
            espSocket = null;
            console.log("ESP disconnected, reference cleared");
        }
        if (socket === browserSocket) {
            browserSocket = null;
            console.log("Browser disconnected, reference cleared");
        }
    });
});

server.listen(PORT, '0.0.0.0', () => {
    console.log(`Server running on port ${PORT}`);
});