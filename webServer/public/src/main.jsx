import React from 'react'
import ReactDOM from 'react-dom/client'
import App from './App.jsx'
import { ThemeProvider } from './components/custom/ThemeProvider.jsx'
import { AuthProvider } from './components/custom/AuthProvider.jsx'
import './index.css'

ReactDOM.createRoot(document.getElementById("root")).render(
	<ThemeProvider defaultTheme="system" storageKey="vite-ui-theme">
		<AuthProvider>
			<App />
		</AuthProvider>
	</ThemeProvider>
)