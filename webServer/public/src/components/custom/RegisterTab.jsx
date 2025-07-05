import { Button } from "@/components/ui/button"
import {
    Card,
    CardContent,
    CardDescription,
    CardFooter,
    CardHeader,
    CardTitle,
} from "@/components/ui/card"
import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"

import { useState } from 'react'

import { socket } from '../../main.jsx'

export default function RegisterTab() {
    const [name, setName] = useState("");
    const [showEmptyError, setShowEmptyError] = useState(false);
    const [success, setSuccess] = useState(false);

    function captureRecording() {
        if (name.length > 0) {
            setShowEmptyError(false);

            socket.send(JSON.stringify({
                type: "browserEcgCapture",
                data: name
            }));

            setName("");
            setSuccess(true);
            setTimeout(() => {
                setSuccess(false);
            }, 2000);
        } else {
            setSuccess(false);
            setShowEmptyError(true);
        }
    }

    return (
        <Card>
            <CardHeader>
                <CardTitle>Register</CardTitle>
                <CardDescription>
                    Register your ECG here.
                </CardDescription>
            </CardHeader>
            <CardContent className="space-y-2">
                <div className="space-y-1">
                    <Label htmlFor="name">Name</Label>
                    <Input 
                        id="name"
                        defaultValue=""
                        value={name}
                        onChange={(e) => setName(e.target.value)}
                    />
                    {showEmptyError && <p className="text-red-500 text-sm">Please enter a name.</p>}
                    {success && <p className="text-green-500 text-sm">Captured successfully.</p>}
                </div>
            </CardContent>
            <CardFooter>
                <Button onClick={captureRecording}>Capture Recording</Button>
            </CardFooter>
        </Card>
    )
}