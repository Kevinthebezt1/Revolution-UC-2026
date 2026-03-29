# Smart Pickup Locker

Smart Pickup Locker is a secure package delivery and pickup system built for hackathon use. It combines a website, cloud database, Raspberry Pi, ESP32, camera, sensors, and a keypad to protect packages from drop off to pickup.

## Overview

Users create an expected delivery on the website. The system generates an `orderId` and turns it into a QR code for staff. When the package arrives, the locker scans the QR code and checks the database. If the code is valid, the locker unlocks so staff can place the package inside. The system checks that the locker is closed and updates the order status.

When the user wants to collect the package, the system generates a one time password, or OTP, that is valid for 60 seconds. The user enters the OTP on a membrane keypad. If the OTP is correct, the locker opens. If the user enters the wrong OTP three times, the system applies a temporary penalty.

## Problem

Package theft is a common issue. Many people either lose deliveries or waste time traveling to pickup hubs. This project targets a safer and more convenient last-step delivery experience.

## Features

- Create delivery orders through a website
- Generate a unique `orderId`
- Convert `orderId` into a QR code
- Scan QR code at delivery time
- Validate orders against Firebase
- Unlock locker for valid deliveries
- Count detected items per order
- Detect door closed state
- Auto close the locker after deposit
- Generate 6 digit OTP for pickup
- OTP expires in 60 seconds
- OTP entry through membrane keypad
- Temporary lockout after 3 failed attempts
- Send notifications through `ntfy`
- Track order status in the database

## System Architecture

### Web App

The website is used to:

- create orders
- generate order IDs
- show all orders in the database
- verify deliveries
- manage OTP generation
- send notifications

### Cloud

Firebase is used for:

- website hosting
- Cloud Firestore database
- real-time order state tracking

### Raspberry Pi

The Raspberry Pi handles:

- QR scanning
- backend logic
- communication with Firebase
- coordination with ESP32 and sensors

### ESP32

The ESP32 handles:

- servo motor control
- locker open and close actions
- local device endpoints

### Hardware

The physical locker uses:

- camera
- membrane keypad
- servo motor
- reed switch
- ultrasonic sensor
- acrylic and wood enclosure

## Tech Stack

### Languages

- HTML
- CSS
- JavaScript
- C or C++ for ESP32 firmware

### Frameworks and Libraries

- Firebase Web SDK
- Arduino framework
- WiFi library
- WebServer library
- ESP32Servo library

### Platforms and Services

- Firebase Hosting
- Cloud Firestore
- Raspberry Pi
- ESP32
- `ntfy` notifications
- QR code generation API

## How It Works

### 1. Create Order

A user enters package information on the website.

Input:

- customer name
- item name

Output:

- unique `orderId`
- QR code linked to that order

### 2. Delivery Verification

When staff deliver the package:

- the camera scans the QR code
- the Raspberry Pi checks Firestore
- if valid, the locker opens
- staff place the package inside
- the system updates the order as delivered
- the system checks if the locker is closed

### 3. Pickup Flow

When the user wants to collect the package:

- the system generates a 6 digit OTP
- OTP is valid for 60 seconds
- user enters OTP on the membrane keypad
- if valid, the locker opens
- if invalid 3 times, the system applies a timeout penalty

## Firestore Data Model

### `orders`

Example fields:

```json
{
  "orderId": "123456",
  "customerName": "Alex Nguyen",
  "itemName": "Bluetooth Earbuds",
  "detectedCount": 1,
  "status": "waiting",
  "createdAt": "server timestamp",
  "deliveredAt": "server timestamp or null"
}
```

### `delivery_requests`

Example fields:

```json
{
  "orderId": "123456",
  "status": "pending",
  "source": "esp32-1",
  "createdAt": "server timestamp"
}
```

### `pickup_otps`

Recommended model:

```json
{
  "orderId": "123456",
  "otpHash": "hashed value",
  "status": "active",
  "createdAt": "server timestamp",
  "expiresAt": "server timestamp",
  "usedAt": null,
  "deviceId": "locker-1",
  "requestId": "unique request id",
  "attemptCount": 0,
  "lockedUntil": null
}
```

Possible status values:

- active
- used
- expired
- blocked

## OTP Security Design

The OTP system is designed to be one time and time limited.

Rules:

- generate a 6 digit OTP on a trusted backend
- do not generate OTP in the browser
- store a hash of the OTP instead of plain text
- send plain OTP to the user through notification or SMS
- expire OTP after 60 seconds
- allow only one successful use
- reject expired OTP
- block access after repeated failures

### Verification Flow

Use a Firestore transaction to:

- read the OTP document
- confirm status is active
- confirm current time is before `expiresAt`
- confirm attempt count is below the limit
- compare entered OTP hash with stored hash
- mark OTP as used if valid
- increment failed attempts if invalid
- unlock only after the transaction succeeds

## Hardware Components

- Raspberry Pi
- ESP32
- camera module
- membrane keypad
- servo motor
- reed switch
- ultrasonic sensor
- acrylic board
- wooden frame

## Setup

### 1. Clone the project

```bash
git clone <your-repo-url>
cd smart-pickup-locker
```

### 2. Configure Firebase

Add your Firebase config in the web app:

```js
const firebaseConfig = {
  apiKey: "YOUR_API_KEY",
  authDomain: "YOUR_PROJECT.firebaseapp.com",
  projectId: "YOUR_PROJECT_ID",
  storageBucket: "YOUR_PROJECT.firebasestorage.app",
  messagingSenderId: "YOUR_SENDER_ID",
  appId: "YOUR_APP_ID"
};
```

### 3. Deploy Website

Host the website with Firebase Hosting.

```bash
firebase login
firebase init
firebase deploy
```

### 4. Upload ESP32 Firmware

Flash the ESP32 with the Arduino code and connect:

- servo
- power
- WiFi
- locker control pins

### 5. Connect Raspberry Pi

Set up the Raspberry Pi to:

- read QR scans
- communicate with Firestore
- send delivery requests
- manage OTP verification
- talk to ESP32

### 6. Configure Notifications

Set your `ntfy` topic in the website or backend.

Example:

```txt
revolution-locker-alerts
```

## Usage

### Create a New Order

- Open the website
- Enter customer name and item
- Submit the form
- Copy the generated `orderId`
- Share the QR code with staff

### Verify a Delivery

- Scan the QR code
- Check the order in Firebase
- Open the locker if valid
- Place the package inside
- Close the locker
- Update order status

### Pick Up a Package

- Generate an OTP
- Send it to the user
- User enters OTP on keypad
- Verify OTP
- Open locker if valid

## Challenges

- syncing data between the website and Raspberry Pi
- keeping real-time states consistent
- handling OTP expiration and retry logic
- integrating cloud software with physical hardware
- making sensors reliable during live demo use

## Accomplishments

- built a full hardware and cloud workflow
- completed secure delivery verification
- added OTP-based pickup control
- integrated sensors, website, and embedded devices
- created a working smart locker prototype

## What We Learned

- hardware and software integration needs strong testing
- real-time systems need careful state handling
- security logic matters even in small prototypes
- cloud and device sync is one of the hardest parts
- physical demos need simple and reliable flows

## Future Improvements

- multi-locker support
- camera proof of delivery storage
- hashed OTP storage only
- device-bound OTP validation
- tamper detection
- analytics dashboard
- mobile notifications
- better offline recovery
- weight sensor for item removal confirmation

## Demo Pitch

Smart Pickup Locker secures the last step of delivery. It verifies package drop off with QR scanning and protects pickup with a 60 second OTP entered on a keypad. Every step is tracked, verified, and linked to the database to reduce theft and delivery errors.

## Team

Smart Pickup Locker was built by:

- Quan Tran
- Van Nguyen
- Dinh Manh Nguyen
- Duc Tinh Ngo

## Team Roles

- Quan Tran, mechanical design of the locker
- Van Nguyen, mechanical design of the locker and full stack website development
- Dinh Manh Nguyen, ESP32 development
- Duc Tinh Ngo, Raspberry Pi and ESP32 integration

## Contact

For project questions, please contact the team members above.

## Credits

This project combines mechanical design, embedded systems, cloud database integration, and web development into one secure smart delivery system.
