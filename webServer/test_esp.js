const WebSocket = require('ws');

// --- Config ---
const SERVER_URL = 'wss://renalhealthmonitor.onrender.com';
const DEVICE_ID = 'ESP32_TEST_LAPTOP';

// Analytes the real ESP cycles through
const ANALYTES = [
    { name: 'Urea', concentration: 14.5, peakCurrent: 39.0 },
    { name: 'Uric Acid', concentration: 8.2, peakCurrent: 20.3 },
    { name: 'Creatinine', concentration: 5.7, peakCurrent: 26.25 },
];

// --- Helpers ---
function send(ws, obj) {
    const msg = JSON.stringify(obj);
    console.log('  ➜ SEND:', msg);
    ws.send(msg);
}

function sleep(ms) {
    return new Promise(r => setTimeout(r, ms));
}

// --- Main sequence ---
async function run() {
    console.log(`\nConnecting to ${SERVER_URL} …`);

    const ws = new WebSocket(SERVER_URL);

    ws.on('open', async () => {
        console.log('✓ Connected!\n');

        // 1. Init – register device
        console.log('[1/5] Sending init …');
        send(ws, { type: 'init', data: 'espInit', deviceId: DEVICE_ID });
        await sleep(1500);

        // 2. Processing state → PROCESSING
        console.log('[2/5] Sending processingState → PROCESSING …');
        send(ws, { type: 'processingState', data: 'PROCESSING', deviceId: DEVICE_ID });
        await sleep(1500);

        // 3. Voltammetry results for each analyte
        for (let i = 0; i < ANALYTES.length; i++) {
            const a = ANALYTES[i];
            console.log(`[3/5] Sending voltammetryResult for ${a.name} …`);
            send(ws, {
                type: 'voltammetryResult',
                deviceId: DEVICE_ID,
                analyte: a.name,
                concentration: a.concentration,
                peakCurrent: a.peakCurrent,
            });
            await sleep(1500);
        }

        // 4. Processing state → COMPLETED
        console.log('[4/5] Sending processingState → COMPLETED …');
        send(ws, { type: 'processingState', data: 'COMPLETED', deviceId: DEVICE_ID });
        await sleep(1500);

        // 5. Processing state → IDLE
        console.log('[5/5] Sending processingState → IDLE …');
        send(ws, { type: 'processingState', data: 'IDLE', deviceId: DEVICE_ID });
        await sleep(500);

        console.log('\n✓ All messages sent. Closing connection.');
        ws.close();
    });

    ws.on('message', (data) => {
        console.log('  ← RECV:', data.toString());
    });

    ws.on('error', (err) => {
        console.error('✗ WebSocket error:', err.message);
    });

    ws.on('close', () => {
        console.log('Connection closed.\n');
    });
}

run();
