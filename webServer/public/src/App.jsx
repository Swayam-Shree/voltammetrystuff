import {
    Card,
    CardContent,
    CardDescription,
    CardHeader,
    CardTitle,
} from "./components/ui/card"
import { Badge } from "./components/ui/badge"

import { useContext } from 'react'
import { VoltammetryContext } from './main.jsx'

export default function App() {
    const { currentBatch, completedBatch, isLoading } = useContext(VoltammetryContext);

    const formatTimestamp = (timestamp) => {
        return new Date(timestamp).toLocaleTimeString();
    };

    return (
        <div className="p-6 space-y-6">
            <h1 className="text-3xl font-bold">Renal Health Monitor</h1>
            
            {/* Current Results Card */}
            <Card>
                <CardHeader>
                    <CardTitle>Current Results</CardTitle>
                    <CardDescription>
                        Latest batch of voltammetry measurements
                    </CardDescription>
                </CardHeader>
                <CardContent>
                    {isLoading ? (
                        <div className="text-center py-8">
                            <p className="text-muted-foreground">
                                Loading results... ({currentBatch.length}/3)
                            </p>
                        </div>
                    ) : completedBatch ? (
                        <div className="space-y-4">
                            <div className="text-sm text-muted-foreground mb-4">
                                Batch completed at {formatTimestamp(Date.now())}
                            </div>
                            {completedBatch.map((result, index) => (
                                <div key={index} className="border rounded-lg p-4 space-y-2">
                                    <div className="flex justify-between items-start">
                                        <div className="space-y-2">
                                            <div className="flex items-center gap-2">
                                                <Badge variant="outline" className="text-lg px-3 py-1">
                                                    {result.analyte}
                                                </Badge>
                                            </div>
                                            <div className="grid grid-cols-2 gap-4">
                                                <div>
                                                    <p className="text-sm font-medium text-muted-foreground">Concentration</p>
                                                    {
                                                        result.analyte === "Urea" ? (
                                                            <p className="text-xl font-bold">{result.concentration?.toFixed(3)} mM</p>
                                                        ) : (
                                                            <p className="text-xl font-bold">{result.concentration?.toFixed(3)} µM</p>
                                                        )
                                                    }
                                                </div>
                                                <div>
                                                    <p className="text-sm font-medium text-muted-foreground">Max Current</p>
                                                    <p className="text-xl font-bold">{result.peakCurrent?.toFixed(3)} µA</p>
                                                </div>
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            ))}
                        </div>
                    ) : (
                        <div className="text-center py-8">
                            <p className="text-muted-foreground">Waiting for data...</p>
                        </div>
                    )}
                </CardContent>
            </Card>
        </div>
    );
}