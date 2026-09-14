# XInput to vJoy RC Flight Mixer

A C++ application that bridges standard XInput gamepads (like Xbox controllers) to radio-control (RC) flight simulators. It reads raw gamepad inputs, applies deadzones and aviation-specific control mixing (such as differential ailerons), and translates the signals into standard RC PWM pulse widths (1000–2000µs) before feeding them into a vJoy virtual device.

---

## Tech Stack

*   **Language:** C++
*   **APIs:** Windows API, Microsoft XInput API
*   **Driver Integration:** vJoy SDK (Dynamic DLL injection)

---

## Key Features

*   **Aviation Control Mixing:** Implements programmable differential ailerons (currently configured to 40%) to counteract adverse yaw, accurately mapping a single roll axis input to dual independent servo outputs.
*   **RC Pulse Translation:** Converts normalized floating-point gamepad inputs into standard Radio Control PWM pulse widths (1000µs – 2000µs) expected by flight simulators.
*   **Dynamic Driver Hooking:** Safely loads `vJoyInterface.dll` and resolves memory addresses for core functions at runtime, preventing hard crashes if the virtual driver is missing.
*   **Signal Processing:** Implements custom deadzones and linear magnitude scaling for thumbsticks and analog triggers to guarantee precise control resolution.
*   **Low-Latency Polling:** Operates at a stable 50Hz (20ms) polling rate, perfectly matching standard RC receiver hardware output frequencies.

---

## Hardware Requirements

*   Windows PC
*   XInput-compatible controller (e.g., Xbox One / Series X Controller)
*   [vJoy (Virtual Joystick)](https://github.com/shauleiz/vJoy) installed and configured with at least 5 axes.

---

## Installation & Setup

### 1. Prerequisites
Ensure you have installed the **vJoy** device driver and configured a virtual controller. You will also need a C++ compiler (like MinGW or MSVC) installed on your system.

### 2. Clone the Repository
```bash
git clone [https://github.com/sonnyrising/controller-to-rc-util.git](https://github.com/sonnyrising/controller-to-rc-util.git)
cd controller-to-rc-util
```

### 3. Compile the Code
If you are using GCC/MinGW, compile the project and link the XInput library:
```bash
g++ main.cpp -o rc_mixer.exe -lxinput
```
*Note: Ensure `vJoyInterface.dll` is present in the same directory as the executable, or added to your system PATH.*

### 4. Run the Application
```bash
./rc_mixer.exe
```
Once running, the terminal will output the live PWM telemetry data (1000-2000) for Left Aileron, Right Aileron, Elevator, Rudder, and Throttle. 

---

## Project Architecture

*   `main.cpp` — The monolithic core of the application. It contains the 50Hz polling loop, the mathematical normalization functions, the RC mixing logic (`differentialAilerons`), and the Windows API hooks for both XInput and the vJoy DLL.
