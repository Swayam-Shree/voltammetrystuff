import { initializeApp } from 'firebase/app';
import { getFirestore } from 'firebase/firestore';

const firebaseConfig = {
    apiKey: "AIzaSyB7JF1cyRcItL3pU3nRMlITxMmjiYE-zgY",
    authDomain: "othlanding.firebaseapp.com",
    databaseURL: "https://othlanding-default-rtdb.firebaseio.com",
    projectId: "othlanding",
    storageBucket: "othlanding.appspot.com",
    messagingSenderId: "516659795219",
    appId: "1:516659795219:web:1a6339da77533f6d3336e6",
    measurementId: "G-M0TFLNB5C1"
};

const firebaseApp = initializeApp(firebaseConfig);
export const db = getFirestore(firebaseApp);
