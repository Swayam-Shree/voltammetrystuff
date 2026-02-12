import { useState, useEffect } from 'react'
import { db } from './firebase.js'
import { collection, onSnapshot, query, orderBy, limit } from 'firebase/firestore'
import {
    Card,
    CardContent,
    CardDescription,
    CardHeader,
    CardTitle,
} from "./components/ui/card"
import { Badge } from "./components/ui/badge"

function DeviceList({ devices, onSelectDevice }) {
    return (
        <div className="p-6 max-w-4xl mx-auto space-y-6">
            <div className="space-y-2">
                <h1 className="text-3xl font-bold tracking-tight">Renal Health Monitor</h1>
                <p className="text-muted-foreground">Select a device to view its readings</p>
            </div>

            {devices.length === 0 ? (
                <Card>
                    <CardContent className="py-12">
                        <div className="text-center space-y-2">
                            <div className="text-4xl">📡</div>
                            <p className="text-muted-foreground">No devices found</p>
                            <p className="text-sm text-muted-foreground">
                                Connect an ESP32 device to start collecting data
                            </p>
                        </div>
                    </CardContent>
                </Card>
            ) : (
                <div className="grid gap-4">
                    {devices.map((device) => (
                        <Card
                            key={device.id}
                            className="cursor-pointer transition-all duration-200 hover:shadow-md hover:border-primary/50 hover:scale-[1.01]"
                            onClick={() => onSelectDevice(device.id)}
                        >
                            <CardHeader>
                                <div className="flex items-center justify-between">
                                    <CardTitle className="text-lg">{device.id}</CardTitle>
                                    <StatusBadge status={device.status} />
                                </div>
                                <CardDescription>
                                    {device.lastSeen
                                        ? `Last seen: ${new Date(device.lastSeen.seconds * 1000).toLocaleString()}`
                                        : 'Never connected'
                                    }
                                </CardDescription>
                            </CardHeader>
                        </Card>
                    ))}
                </div>
            )}
        </div>
    )
}

function StatusBadge({ status }) {
    const variant = status === 'PROCESSING' ? 'default'
        : status === 'COMPLETED' ? 'secondary'
            : 'outline';

    const dotColor = status === 'PROCESSING' ? 'bg-green-400 animate-pulse'
        : status === 'COMPLETED' ? 'bg-blue-400'
            : 'bg-gray-400';

    return (
        <Badge variant={variant} className="gap-1.5">
            <span className={`inline-block w-2 h-2 rounded-full ${dotColor}`}></span>
            {status || 'IDLE'}
        </Badge>
    )
}

function ReadingCard({ reading, isLatest }) {
    return (
        <div className={`border rounded-lg p-4 space-y-2 transition-all ${isLatest ? 'border-primary/50 bg-primary/5 shadow-sm' : ''}`}>
            <div className="flex justify-between items-start">
                <div className="space-y-2 w-full">
                    <div className="flex items-center justify-between">
                        <Badge variant="outline" className="text-base px-3 py-1">
                            {reading.analyte}
                        </Badge>
                        <span className="text-xs text-muted-foreground">
                            {reading.timestamp
                                ? new Date(reading.timestamp.seconds * 1000).toLocaleString()
                                : '—'
                            }
                        </span>
                    </div>
                    <div className="grid grid-cols-2 gap-4">
                        <div>
                            <p className="text-sm font-medium text-muted-foreground">Concentration</p>
                            {
                                reading.analyte === "Urea" ? (
                                    <p className="text-xl font-bold">{reading.concentration?.toFixed(3)} mM</p>
                                ) : (
                                    <p className="text-xl font-bold">{reading.concentration?.toFixed(3)} µM</p>
                                )
                            }
                        </div>
                        <div>
                            <p className="text-sm font-medium text-muted-foreground">Peak Current</p>
                            <p className="text-xl font-bold">{reading.peakCurrent?.toFixed(3)} µA</p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    )
}

function DeviceDetail({ deviceId, onBack }) {
    const [readings, setReadings] = useState([]);
    const [deviceInfo, setDeviceInfo] = useState(null);

    // Listen to device info
    useEffect(() => {
        const unsubscribe = onSnapshot(
            collection(db, 'devices'),
            (snapshot) => {
                const doc = snapshot.docs.find(d => d.id === deviceId);
                if (doc) {
                    setDeviceInfo({ id: doc.id, ...doc.data() });
                }
            }
        );
        return unsubscribe;
    }, [deviceId]);

    // Listen to readings (latest 50, ordered by timestamp desc)
    useEffect(() => {
        const q = query(
            collection(db, 'devices', deviceId, 'readings'),
            orderBy('timestamp', 'desc'),
            limit(50)
        );
        const unsubscribe = onSnapshot(q, (snapshot) => {
            const data = snapshot.docs.map(doc => ({ id: doc.id, ...doc.data() }));
            setReadings(data);
        });
        return unsubscribe;
    }, [deviceId]);

    // Group latest 3 readings (one per analyte) as the "current batch"
    const latestBatch = readings.slice(0, 3);
    const history = readings.slice(3);

    return (
        <div className="p-6 max-w-4xl mx-auto space-y-6">
            {/* Header */}
            <div className="flex items-center gap-4">
                <button
                    onClick={onBack}
                    className="text-muted-foreground hover:text-foreground transition-colors text-lg cursor-pointer"
                >
                    ← Back
                </button>
                <div className="flex-1">
                    <div className="flex items-center gap-3">
                        <h1 className="text-3xl font-bold tracking-tight">{deviceId}</h1>
                        {deviceInfo && <StatusBadge status={deviceInfo.status} />}
                    </div>
                    {deviceInfo?.lastSeen && (
                        <p className="text-sm text-muted-foreground">
                            Last seen: {new Date(deviceInfo.lastSeen.seconds * 1000).toLocaleString()}
                        </p>
                    )}
                </div>
            </div>

            {/* Latest Results */}
            <Card>
                <CardHeader>
                    <CardTitle>Latest Results</CardTitle>
                    <CardDescription>
                        Most recent voltammetry measurements
                    </CardDescription>
                </CardHeader>
                <CardContent>
                    {latestBatch.length > 0 ? (
                        <div className="space-y-3">
                            {latestBatch.map((reading) => (
                                <ReadingCard key={reading.id} reading={reading} isLatest={true} />
                            ))}
                        </div>
                    ) : (
                        <div className="text-center py-8">
                            <p className="text-muted-foreground">No readings yet</p>
                        </div>
                    )}
                </CardContent>
            </Card>

            {/* History */}
            {history.length > 0 && (
                <Card>
                    <CardHeader>
                        <CardTitle>History</CardTitle>
                        <CardDescription>
                            Previous measurements ({history.length} readings)
                        </CardDescription>
                    </CardHeader>
                    <CardContent>
                        <div className="space-y-3">
                            {history.map((reading) => (
                                <ReadingCard key={reading.id} reading={reading} isLatest={false} />
                            ))}
                        </div>
                    </CardContent>
                </Card>
            )}
        </div>
    )
}

export default function App() {
    const [devices, setDevices] = useState([]);
    const [selectedDeviceId, setSelectedDeviceId] = useState(null);

    // Listen to the devices collection
    useEffect(() => {
        const unsubscribe = onSnapshot(collection(db, 'devices'), (snapshot) => {
            const data = snapshot.docs.map(doc => ({ id: doc.id, ...doc.data() }));
            setDevices(data);
        });
        return unsubscribe;
    }, []);

    if (selectedDeviceId) {
        return (
            <DeviceDetail
                deviceId={selectedDeviceId}
                onBack={() => setSelectedDeviceId(null)}
            />
        );
    }

    return (
        <DeviceList
            devices={devices}
            onSelectDevice={setSelectedDeviceId}
        />
    );
}