# Sensor Dashboard

A realtime React dashboard that listens to sensor data from Firestore and renders an interactive city scene that reacts to live readings.

## Prerequisites

- Node.js 18+
- A Firebase project with Firestore enabled
- The serial port writer (`index.js`) running separately to push sensor data over USB

## Setup

### 1. Clone the repo

```bash
git clone <repo-url>
cd Sensor-Dashboard-Website
```

### 2. Install dependencies

```bash
npm install
```

### 3. Configure Firebase

Create a `.env.local` file in the project root with your Firebase Web App config. You can find these values in the Firebase Console under **Project Settings → Your apps → Web app**.

```env
VITE_FIREBASE_API_KEY=your_api_key
VITE_FIREBASE_AUTH_DOMAIN=your-project-id.firebaseapp.com
VITE_FIREBASE_DATABASE_URL=https://your-project-id-default-rtdb.region.firebasedatabase.app
VITE_FIREBASE_PROJECT_ID=your-project-id
VITE_FIREBASE_STORAGE_BUCKET=your-project-id.firebasestorage.app
VITE_FIREBASE_MESSAGING_SENDER_ID=your_sender_id
VITE_FIREBASE_APP_ID=your_app_id
```

> **Note:** Use the **Web SDK config** (from Project Settings), not the Admin SDK service account key.

### 4. Set Firestore security rules

For local development, open the Firebase Console → **Firestore → Rules** and set:

```
rules_version = '2';
service cloud.firestore {
  match /databases/{database}/documents {
    match /{document=**} {
      allow read, write: if true;
    }
  }
}
```

> Restrict these rules before any production use.

### 5. Run the dashboard

```bash
npm run dev
```

Open [http://localhost:5173](http://localhost:5173) in your browser.

## How it works

The dashboard listens in realtime to the `sensor/latest` document in Firestore using `onSnapshot`. The serial port writer (`index.js`) reads sensor data over USB, parses the JSON, and writes it to that document on every reading.

```
Microcontroller → USB Serial → index.js → Firestore (sensor/latest) → Dashboard
```

## Sensor fields

| Field | Label | Type | Unit |
|---|---|---|---|
| `temperature` | Temperature | Number | °C |
| `humidity` | Humidity | Number | % (0–100) |
| `waterLevel` | Water Level | Number | % (0–100) |
| `shock` | Shock | Boolean (0 or 1) | — |
| `flame` | Flame | Boolean (0 or 1) | — |

## Visual effects

| Effect | Sensor | Behaviour |
|---|---|---|
| **Sky colour** | — | Fixed 5pm evening gradient — deep indigo → purple → warm burnt-orange at the horizon |
| **Rain** | `humidity` | Canvas rain density scales with 0–100% humidity |
| **Water rising** | `waterLevel` | Animated wave that starts at a visible baseline and rises up to 38% of screen height |
| **Fires on buildings** | `flame` | Animated SVG flames appear on 5 rooftops when flame is detected (`flame > 0`) |
| **Earthquake shake** | `shock` | Entire screen shakes with a decaying oscillation on a rising edge (0→1) |
| **Thermometer** | `temperature` | Animated SVG fill that colour-shifts blue → green → yellow → orange → red |

## Alert thresholds

Cards flash red and show a warning badge when a threshold is crossed. Thresholds are defined in `src/App.jsx`:

| Sensor | Default threshold | Note |
|---|---|---|
| `temperature` | > 35 °C | |
| `humidity` | > 80 % | |
| `waterLevel` | > 75 % | |
| `shock` | any truthy value | Boolean — alerts whenever detected |
| `flame` | any truthy value | Boolean — alerts whenever detected |

## Card layout

Three columns of glassmorphism cards sit flush below the header:

- **Column 1:** Temperature, Humidity
- **Column 2:** Shock, Flame
- **Column 3:** Water Level
- **Thermometer widget** (left of columns): animated SVG thermometer mirroring `temperature`

## Tech stack

- [React 18](https://react.dev/)
- [Vite](https://vitejs.dev/) — dev server and bundler
- [Firebase JS SDK](https://firebase.google.com/docs/web/setup) — Firestore realtime listener
- [MUI (Material UI)](https://mui.com/) — UI components and icons
- [Tailwind CSS v4](https://tailwindcss.com/) — utility-class layout and spacing
- [Framer Motion](https://www.framer.com/motion/) — imperative earthquake shake, water rise, and thermometer animations
