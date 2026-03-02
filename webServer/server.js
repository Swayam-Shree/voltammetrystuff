const WebSocket = require('ws');
const express = require('express');
const http = require('http');
const path = require('path');
const cors = require('cors');
const admin = require('firebase-admin');

// Initialize Firebase Admin with the project config
admin.initializeApp({
    projectId: 'othlanding',
    databaseURL: 'https://othlanding-default-rtdb.firebaseio.com',
});

const db = admin.firestore();

const app = express();

app.use(cors());
app.use(express.static(path.join(__dirname, 'public/dist')));
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = process.env.PORT || 6969;

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

        const deviceId = parsed.deviceId;

        switch (parsed.type) {
            case "init":
                if (parsed.data === "espInit" && deviceId) {
                    console.log(`ESP device "${deviceId}" connected`);
                    // Register / update device in Firestore
                    db.collection('devices').doc(deviceId).set({
                        deviceId: deviceId,
                        status: 'IDLE',
                        lastSeen: admin.firestore.FieldValue.serverTimestamp(),
                    }, { merge: true }).catch(err => {
                        console.error('Firestore write error (init):', err.message);
                    });
                }
                break;

            case "voltammetryResult":
                if (deviceId) {
                    console.log(`Result from "${deviceId}":`, parsed.analyte, parsed.concentration);
                    // Write reading to subcollection
                    const deviceRef = db.collection('devices').doc(deviceId);
                    deviceRef.collection('readings').add({
                        timestamp: admin.firestore.FieldValue.serverTimestamp(),
                        analyte: parsed.analyte,
                        concentration: parsed.concentration,
                        peakCurrent: parsed.peakCurrent,
                    }).catch(err => {
                        console.error('Firestore write error (reading):', err.message);
                    });
                    // Update lastSeen
                    deviceRef.set({
                        lastSeen: admin.firestore.FieldValue.serverTimestamp(),
                    }, { merge: true }).catch(err => {
                        console.error('Firestore write error (lastSeen):', err.message);
                    });
                }
                break;

            case "processingState":
                if (deviceId) {
                    console.log(`State from "${deviceId}":`, parsed.data);
                    db.collection('devices').doc(deviceId).set({
                        status: parsed.data,
                        lastSeen: admin.firestore.FieldValue.serverTimestamp(),
                    }, { merge: true }).catch(err => {
                        console.error('Firestore write error (state):', err.message);
                    });
                }
                break;
        }
    });

    socket.on('close', () => {
        console.log('Client disconnected', socket.id);
    });
});

server.listen(PORT, '0.0.0.0', () => {
    console.log(`Server running on port ${PORT}`);
});