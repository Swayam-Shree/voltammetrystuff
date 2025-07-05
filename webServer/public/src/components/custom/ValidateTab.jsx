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

import { useContext } from 'react'
import { EcgContext } from '../../main.jsx'

export default function ValidateTab() {
    const { prediction } = useContext(EcgContext);

    return (
        <Card>
            <CardHeader>
                <CardTitle>Validate</CardTitle>
                <CardDescription>
                    Validate your ECG here, put you figer on the sensor.
                </CardDescription>
            </CardHeader>
            <CardContent className="space-y-2">
                <div className="space-y-1">
                    <Label htmlFor="current">{prediction.user}</Label>
                </div>
                {/* <div className="space-y-1">
                <Label htmlFor="current">Current password</Label>
                <Input id="current" type="password" />
                </div>
                <div className="space-y-1">
                <Label htmlFor="new">New password</Label>
                <Input id="new" type="password" />
                </div> */}
            </CardContent>
            <CardFooter>
                <Label htmlFor="current">{prediction.confidence}%</Label>
            </CardFooter>
        </Card>
    )
}