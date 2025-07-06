const WebSocket = require('ws');
const express = require('express');
const http = require('http');
const path = require('path');
const cors = require('cors');
const { type } = require('os');

const app = express();

app.use(cors());
app.use(express.static(path.join(__dirname, 'public/dist')));
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

let espSocket = null;
let browserSocket = null;

let idCounter = 0;

wss.on('connection', (socket) => {
    socket.id = idCounter;
    ++idCounter;
    console.log('Client connected', socket.id);

    socket.on('message', (message) => { 
        let messageStr = message;
        message = JSON.parse(messageStr);
        
        if (!message) return;

        switch (message.type) {
            case "init":
                switch (message.data) {
                    case "espInit":
                        espSocket = socket;
                        console.log("ESP connected");
                        break;
                    case "browserInit":
                        browserSocket = socket;
                        console.log("Browser connected");
                        break;
                }
                break;

            case "voltammetryResult":
                if (browserSocket) {
                    browserSocket.send(JSON.stringify(message));
                }
                break;
            case "processingState":
                if (browserSocket) {
                    browserSocket.send(JSON.stringify(message));
                }
        }
    });

    socket.on('close', () => console.log('Client disconnected'));
});

server.listen(6969, () => {
    console.log('Server running on http://localhost:6969');
});