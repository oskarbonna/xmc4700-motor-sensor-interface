# xmc4700-motor-sensor-interface
Bare-metal C firmware for a 4WD mobile platform on Infineon XMC4700 — custom UART protocol, differential drive control, IR edge detection

# Motor and Sensor Interface for a 4WD Mobile Platform

Bare-metal firmware in C for an **Infineon XMC4700**, controlling a four-wheel-drive
mobile platform. Developed as my bachelor's thesis at Berliner Hochschule für Technik
(Berlin University of Applied Sciences and Technology), 2026.

The platform can be driven either from a **joystick** or remotely over a **custom binary
UART protocol**, and stops autonomously when it detects an obstacle or a drop-off.

---

## What it does

- Drives four DC gear motors through H-bridge drivers with PWM speed control
- Computes **differential drive**: wheel speeds derived from the commanded turning radius
- Reads three infrared distance sensors through the 12-bit ADC
- Detects obstacles, downward steps and upward steps, and brakes before reaching them
- Accepts commands over UART with checksum validation and a communication watchdog
- Falls back to a defined safe state whenever communication or sensor data is lost

---

## Architecture

The firmware is split into three layers:

| Layer | Responsibility |
|---|---|
| **Driver** | Hardware access — PWM, ADC, UART, timers |
| **Logic** | Differential drive, sensor evaluation, protocol parsing |
| **Application** | Mode selection, safety supervision, main loop |

Each module exposes a narrow interface, so the driver layer can be swapped without
touching the control logic.

---

## UART protocol

Commands are **4 bytes**, responses **8 bytes**. Every frame ends with an **XOR checksum**
over the preceding bytes, so a corrupted frame is discarded rather than executed.

| Byte | Meaning |
|---|---|
| 0 | Command identifier |
| 1 | Speed |
| 2 | Steering |
| 3 | XOR checksum |

The steering byte encodes **curvature, not radius**. Radius is unbounded — a straight line
is infinity, which does not fit in a byte — while curvature is bounded and maps linearly
onto the available range. This also puts the resolution where it matters: near the minimum
turning radius, one step changes the radius by well under one percent.

A missing or late response triggers the watchdog, which stops the drives.

---

## Edge detection

The infrared sensors are mounted at a fixed angle, so a change in measured distance
corresponds to a change in ground height. Two separate thresholds are used — one for a
step **down**, one for a step **up** — because the geometry is not symmetric: a drop-off
must be detected earlier than a rise.

The sensor characteristic is inversely proportional to distance and is linearised in
firmware using a constant derived from a nine-point calibration measurement.

---

## Toolchain

- Infineon **DAVE** IDE
- Target: XMC4700, 144-pin LQFP
- Validation: oscilloscope, logic analyser, serial terminal

---

## Repository contents

| File | Description |
|---|---|
| `uart2.c` / `.h` | Protocol parser, command execution, response framing |
| `ir_sensor.c` / `.h` | ADC conversion, distance calculation, threshold logic |
| `joystick.c` / `.h` | Joystick acquisition, dead zone, out-of-range detection |
| `motor.c` / `.h` | PWM generation and direction control |

---

## Note

This repository contains the firmware I wrote for my thesis. The hardware platform
belongs to Berliner Hochschule für Technik.
