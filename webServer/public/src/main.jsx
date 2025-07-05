import React, { createContext, useState, useContext, useEffect } from 'react'
import ReactDOM from 'react-dom/client'
import App from './App.jsx'
import { ThemeProvider } from './components/custom/ThemeProvider.jsx'
import './index.css'

export const socket = new WebSocket('ws://localhost:6969');

export const VoltammetryContext = createContext();

function VoltammetryProvider({ children }) {
	const [currentBatch, setCurrentBatch] = useState([]);
	const [completedBatch, setCompletedBatch] = useState(null);
	const [isLoading, setIsLoading] = useState(true);

	function updateVoltammetryResult(result) {
		setCurrentBatch(prevBatch => {
			const newBatch = [...prevBatch, result];
			
			// If we have 3 results, complete the batch
			if (newBatch.length === 3) {
				setCompletedBatch(newBatch);
				setIsLoading(false);
				return []; // Reset for next batch
			}
			
			// If this is the first result of a new batch, set loading to true
			if (newBatch.length === 1) {
				setIsLoading(true);
			}
			
			return newBatch;
		});
	}

	return (
		<VoltammetryContext.Provider value={{ 
			currentBatch,
			completedBatch,
			isLoading,
			updateVoltammetryResult 
		}}>
			{children}
		</VoltammetryContext.Provider>
	)
}

socket.addEventListener('open', (event) => {
	console.log("Connected to WebSocket server");

	socket.send(JSON.stringify({
		type: "init",
		data: "browserInit"
	}));
});

function SocketHandler() {
	const { updateVoltammetryResult } = useContext(VoltammetryContext);

	useEffect(() => {
		async function handleMessage(event) {
			let message = JSON.parse(event.data);
			
			switch (message.type) {
				case "voltammetryResult":
					updateVoltammetryResult(message);
					break;
				case "processingState":
					console.log(message);
					break;
			}
		}
		
		socket.addEventListener('message', handleMessage);

		return () => {
			socket.removeEventListener('message', handleMessage);
		};
	}, [updateVoltammetryResult]);

	return <></>;
}

ReactDOM.createRoot(document.getElementById("root")).render(
	<ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
		<VoltammetryProvider>
			<SocketHandler />
			<App />
		</VoltammetryProvider>
	</ThemeProvider>
)