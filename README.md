# Comprehensive Guide: Setting Up and Deploying the Web App

This guide is written for non-technical users. It will take you step-by-step through setting up Google Firebase (for your database and authentication) and deploying your repository on Render (for hosting your server and website so others can see it on the internet).

## Table of Contents
1. [Step 1: Setting up Firebase](#step-1-setting-up-firebase)
2. [Step 2: Updating Your Code](#step-2-updating-your-code)
3. [Step 3: Setting up Render (Hosting)](#step-3-setting-up-render-hosting)
4. [Step 4: Finalizing Firebase Permissions (CORS & Auth)](#step-4-finalizing-firebase-permissions-cors--auth)

---

## Step 1: Setting up Firebase

Firebase is a service by Google that stores our data and handles user logins.

### 1. Create a Firebase Project
1. Go to the [Firebase Console](https://console.firebase.google.com/).
2. Click the big **"Add project"** or **"Create a project"** box.
3. Enter a name for your project (e.g., `My Voltammetry App`).
4. Click **Continue**. (You can turn off Google Analytics for this project to make it simpler, then click **Create project**).
5. Once it finishes loading, click **Continue** to go to your project dashboard.

### 2. Register the Web App to Get Your API Keys
1. On your Firebase dashboard, look for the icons that say "Get started by adding Firebase to your app". Click the web icon, which looks like this: `</>`.
2. Enter an App nickname (like "WebApp"). You do NOT need to check "Also set up Firebase Hosting".
3. Click **Register app**.
4. You will see a block of code with a section that starts with `const firebaseConfig = { ... }`.
   * **IMPORTANT**: Keep this page open or copy all the text inside the `{ ... }` brackets. You will need these "API keys" for the code later.
5. Click **Continue to console**.

### 3. Set Up the Firestore Database
1. On the left-hand sidebar, look under the **"Build"** menu and click **"Firestore Database"**.
2. Click the **"Create database"** button.
3. A popup will appear. Choose to start in **"Test mode"** (this makes it easy to set up right now without getting blocked by permissions).
4. Click **Next**, choose a location close to you, and click **Enable**.
5. Your database is now ready!

### 4. Set Up Authentication (Sign-in)
1. On the left-hand sidebar under **"Build"**, click **"Authentication"**.
2. Click **"Get started"**.
3. In the **"Sign-in method"** tab, click **"Google"** to enable users to log in with their Google accounts.
4. Flip the **"Enable"** toggle to ON.
5. Provide a project support email (just select your email from the dropdown) and click **Save**.

### 5. Generate a Service Account Key (For Render)
Because our backend server also needs to talk to this database, we need to give it a special skeleton key called a Service Account key.
1. Look at the left sidebar again. Next to **"Project Overview"** at the top, click the small gear icon (⚙️) and select **"Project settings"**.
2. Click on the **"Service accounts"** tab at the top.
3. Make sure **"Node.js"** is selected, and click the **"Generate new private key"** button.
4. Click **Generate Key** to confirm.
5. A `.json` file will download to your computer. Open this file using a simple text editor like Notepad on Windows or TextEdit on Mac. 
6. Wait, select **all** the text in this file and **Copy** it. You will paste this entire text into Render later.

---

## Step 2: Updating Your Code

Now that your database is ready, you need to tell your code where to find it.

1. Open the project folder (`voltammetrystuff`) on your computer.
2. Go to `webServer/public/src/` and open `firebase.js` in a text editor.
   * Look for the `const firebaseConfig = { ... }` block.
   * Replace the text inside there with the API keys you copied from Firebase earlier (in Step 1.2).
   * Save the file.
3. Go back to the `webServer/` folder and open `server.js` in a text editor.
   * Look around lines 9-12 for `const firebaseConfig = { ... }`.
   * Change the `projectId` to match your new project's ID.
   * Change the `databaseURL` to match your new database URL (Usually `https://YOUR-PROJECT-ID.firebaseio.com` or `https://YOUR-PROJECT-ID-default-rtdb.firebaseio.com`).
   * Save the file.

4. **Commit and push** these changes to your GitHub repository so Render can see the updated keys. 
   *(If you are new to GitHub, just open your terminal or GitHub Desktop, commit the files, and click "Push").*

---

## Step 3: Setting up Render (Hosting)

Render is the server that will keep your website and backend running 24/7 on the internet.

1. Go to [Render.com](https://render.com/) and click **"Get Started"** or **"Sign In"**. Logging in with GitHub is the easiest way.
2. Once you are in your Render Dashboard, click the **"New"** button at the top and select **"Web Service"**.
3. Connect your GitHub account if it asks, and select your repository from the list.
4. Now, fill in the settings EXACTLY like this:
   * **Name**: Type whatever name you want for your site (e.g., `my-voltammetry-app`). This will be part of your link.
   * **Region**: Pick whichever is closest to you.
   * **Branch**: Leave as `main` or `master`.
   * **Root Directory**: Type exactly `webServer`
   * **Environment**: Select `Node`
   * **Build Command**: Copy and paste exactly this: 
     ```bash
     npm install && cd public && npm install && npm run build && cd ..
     ```
     *(This command installs the backend, goes to the frontend folder, installs the frontend, builds the website pages, and goes back).*
   * **Start Command**: Copy and paste exactly this:
     ```bash
     node server.js
     ```
5. **Scroll down to "Environment Variables"**. Click the **"Add Environment Variable"** button.
   * **Key**: Type exactly `GOOGLE_APPLICATION_CREDENTIALS_JSON`
   * **Value**: Paste the **entire** text from the `.json` Service Account key you downloaded and copied in Step 1.5. Make sure the curly brackets `{` and `}` are included at the beginning and end.
6. Scroll down to the bottom and click the **"Create Web Service"** button.
7. Render will now start downloading and building your app. This takes a few minutes. You will see a lot of text scrolling on the screen.
8. Wait until you see a green **"Live"** text appear next to your service name. At the top left, under your app's name, you will see a URL (like `https://my-voltammetry-app.onrender.com`). **Copy this URL**.

---

## Step 4: Finalizing Firebase Permissions (CORS & Auth)

Now that your site is on the internet with a specific Render URL, you must tell Firebase that this new website is allowed to communicate with it.

### 1. Allow Logins From Your Render URL
1. Go back to your [Firebase Console](https://console.firebase.google.com/).
2. On the left sidebar go to **"Build"** -> **"Authentication"**.
3. Click the **"Settings"** tab, and then click **"Authorized domains"** on the left menu of the settings box.
4. Click the **"Add domain"** button.
5. Paste your Render URL here (for example, `my-voltammetry-app.onrender.com` without the `https://`).
6. Click **Add**.

### 2. Setting Up CORS for Firebase Storage (Optional but recommended)
If your app later needs to upload or download files across domains, you need CORS (Cross-Origin Resource Sharing).
1. Go to the [Google Cloud Console](https://console.cloud.google.com/) and ensure you are logged in with the same account.
2. In the top-left dropdown, select your Firebase project name.
3. At the top-right of your screen, click the **"Activate Cloud Shell"** icon (it looks like a small square terminal `>_`). A black terminal window will open at the bottom.
4. Type `nano cors.json` and press **Enter**.
5. Paste the following text into the dark box:
   ```json
   [
     {
       "origin": ["*"],
       "responseHeader": ["Content-Type"],
       "method": ["GET", "HEAD", "DELETE"],
       "maxAgeSeconds": 3600
     }
   ]
   ```
6. Press the keys **Ctrl + O** (the letter 'O'), press **Enter** to save, then press **Ctrl + X** to exit.
7. Run the following command (replace `YOUR_PROJECT_ID` with the actual project ID of your Firebase app):
   ```bash
   gcloud storage buckets update gs://YOUR_PROJECT_ID.appspot.com --cors-file=cors.json
   ```
   *(You can find your Project ID in the Firebase Project Settings page).*

### Success! 🎉
You are completely done. You can now visit your Render URL, and the web app will connect directly to your new Firebase database!
