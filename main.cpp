#include <iostream>
#include <windows.h>
#include <xinput.h>
#include <algorithm>
#include <cmath>

constexpr int RC_MIN = 1000;
constexpr int RC_MID = 1500;
constexpr int RC_MAX = 2000;

constexpr UINT VJOY_X  = 0x30;
constexpr UINT VJOY_Y  = 0x31;
constexpr UINT VJOY_Z  = 0x32;
constexpr UINT VJOY_RX = 0x33;
constexpr UINT VJOY_RY = 0x34;

typedef BOOL (__cdecl *AcquireVJD_t)(UINT rID);
typedef BOOL (__cdecl *SetAxis_t)(LONG Value, UINT rID, UINT Axis);
AcquireVJD_t AcquireVJD = nullptr;
SetAxis_t SetAxis = nullptr;

bool loadVJoy() {
    HMODULE hVJoy = LoadLibraryA("vJoyInterface.dll");
    if (!hVJoy) return false;
    AcquireVJD = (AcquireVJD_t)GetProcAddress(hVJoy, "AcquireVJD");
    SetAxis = (SetAxis_t)GetProcAddress(hVJoy, "SetAxis");
    return AcquireVJD && SetAxis;
}

long rcToVJoy(int rcPulse) {
    float normalized = (rcPulse - 1000) / 1000.0f;
    return static_cast<long>(normalized * 32767.0f) + 1;
}

struct PilotInputs {
    float roll, pitch, yaw, throttle;
};

struct ServoOutputs {
    int aileronLeft, aileronRight, elevator, rudder, throttle;
};

float processStickAxis(short rawValue, int deadzone, bool invert = false) {
    int val = static_cast<int>(rawValue);
    if (std::abs(val) < deadzone) return 0.0f;

    float sign = (val > 0) ? 1.0f : -1.0f;
    float magnitude = static_cast<float>(std::abs(val) - deadzone);
    
    const float physicalMax = 27000.0f; 
    float maxMagnitude = physicalMax - static_cast<float>(deadzone);

    float normalized = sign * (magnitude / maxMagnitude);
    return std::clamp(invert ? -normalized : normalized, -1.0f, 1.0f);
}

float processTrigger(BYTE rawValue, int deadzone) {
    if (rawValue < deadzone) return 0.0f;
    float magnitude = static_cast<float>(rawValue - deadzone);
    float maxMagnitude = 255.0f - static_cast<float>(deadzone);
    return std::clamp(magnitude / maxMagnitude, 0.0f, 1.0f);
}

int normalizedToPulse(float normalized) {
    int pulse = static_cast<int>(RC_MID + (normalized * 500.0f));
    return std::clamp(pulse, RC_MIN, RC_MAX);
}

int triggerToPulse(float normalized) {
    int pulse = static_cast<int>(RC_MIN + (normalized * 1000.0f));
    return std::clamp(pulse, RC_MIN, RC_MAX);
}

void differentialAilerons(float rollInput, float diffFactor, float& outLeft, float& outRight) {
    outLeft = rollInput;
    outRight = rollInput; 
    if (rollInput > 0) outLeft *= (1.0f - diffFactor); 
    else               outRight *= (1.0f - diffFactor);
}

int main() {
    std::cout << "Starting RC Mixer...\n";

    if (!loadVJoy() || !AcquireVJD(1)) {
        std::cerr << "ERROR: Failed to connect to vJoy! Is it installed and configured?\n";
        return 1;
    }
    std::cout << "Successfully connected to vJoy Ghost Controller!\n\n"; // Fixed semicolon here

    XINPUT_STATE state;
    const int STICK_DEAD = 3500;
    const int TRIG_DEAD = 15;

    while (true) {
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        DWORD result = XInputGetState(0, &state);

        if (result == ERROR_SUCCESS) {
            PilotInputs pilot;
            pilot.roll = processStickAxis(state.Gamepad.sThumbLX, STICK_DEAD);
            pilot.pitch = processStickAxis(state.Gamepad.sThumbLY, STICK_DEAD, true); 
            pilot.yaw = processStickAxis(state.Gamepad.sThumbRX, STICK_DEAD);
            pilot.throttle = processTrigger(state.Gamepad.bRightTrigger, TRIG_DEAD);

            float leftAil, rightAil;
            differentialAilerons(pilot.roll, 0.40f, leftAil, rightAil); 

            ServoOutputs servos;
            servos.aileronLeft = normalizedToPulse(leftAil);
            servos.aileronRight = normalizedToPulse(-rightAil); 
            servos.elevator = normalizedToPulse(pilot.pitch);
            servos.rudder = normalizedToPulse(pilot.yaw);
            servos.throttle = triggerToPulse(pilot.throttle);

            SetAxis(rcToVJoy(servos.aileronLeft),  1, VJOY_X);
            SetAxis(rcToVJoy(servos.aileronRight), 1, VJOY_RY);
            SetAxis(rcToVJoy(servos.elevator),     1, VJOY_Y);
            SetAxis(rcToVJoy(servos.rudder),       1, VJOY_RX);
            SetAxis(rcToVJoy(servos.throttle),     1, VJOY_Z); // Fixed semicolon here

            std::cout << "\rL_Ail: " << servos.aileronLeft 
                      << " | R_Ail: " << servos.aileronRight 
                      << " | Elev: " << servos.elevator 
                      << " | Rud: " << servos.rudder 
                      << " | Thr: " << servos.throttle << "   " << std::flush;
        }
        Sleep(20); 
    }
    return 0;
}