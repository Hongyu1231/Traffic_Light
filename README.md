# 🚦 Smart Traffic Management Simulation (ESP32 + MCXC444)

An IoT-driven, RTOS-based smart traffic light system inspired by Singapore’s dynamic transport infrastructure. This project resolves the inefficiencies of traditional rigid timers by integrating real-time traffic data, RFID authentication, and a full-stack telemetry dashboard.

## 🌟 Key Features

* **Real-Time Traffic Adaptation:** Fetches live traffic `SpeedBands` from the LTA DataMall API to dynamically adjust vehicle green light durations.
* **"Green Man +" RFID System:** Validates user RFID tags to extend pedestrian crossing times for authorized/elderly users.
* **Speed Trap & Telegram Alerts:** Uses Hall effect sensors to calculate vehicle speed. Speed violations automatically trigger an instant Telegram warning to administrators.
* **Secure UART Communication:** Employs a custom XOR-encrypted UART protocol with checksum verification for secure data exchange between the ESP32 and MCXC444.
* **Full-Stack Telemetry:** Real-time feedback is uploaded to a Node.js/Express backend and stored in MongoDB Atlas, visualized on a custom HTML5/Chart.js live dashboard.
* **FreeRTOS Architecture:** Highly concurrent task management utilizing Mutexes (e.g., `g_wifiMutex`, `g_trafficMutex`), Queues, and Semaphores to ensure thread safety and responsiveness.

## 🛠️ Hardware Components

* **Microcontrollers:** NXP MCXC444 (Primary Logic) & ESP32 (Network Bridge)
* **Sensors:**
  * MFRC-522 RFID Reader (Pedestrian Authentication)
  * KY-024 Linear Hall Magnetic Modules (Speed Trap)
  * Photoresistor (Ambient light detection for Yellow light duration)
  * Push Button (Pedestrian crossing request)
* **Actuators:** MCXC444 Onboard RGB LED, KY-011 2-Color LED, Active Buzzer, LCD2004A, Segmented LCD.

## 💻 Software Stack

* **Embedded:** C/C++, FreeRTOS, Arduino Core (ESP32)
* **Backend:** Node.js, Express.js, TypeScript (`ts-node`)
* **Database:** MongoDB Atlas (Mongoose ORM)
* **Frontend:** HTML5, JavaScript (Fetch API), Chart.js
* **External APIs:** LTA DataMall API, Telegram Bot API

## 🚀 Getting Started

### 1. Backend & Database Setup
1. Navigate to the backend directory: `cd SmartTraffic-Backend`
2. Install dependencies: `npm install`
3. Configure your MongoDB Atlas connection string and ensure your current IP is added to the **Network Access Whitelist** (`0.0.0.0/0` for global access during demos).
4. Start the server: 
   ```bash
   npx ts-node server.ts
