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

| Module | Description |
|---|---|
| `main.c` | Entry point, start-up mode selection, main loop |
| `platform.h` | Central configuration — speed limits, minimum turning radius, track width, PWM scaling |
| `uart2.c` / `.h` | Protocol parser, command execution, response framing, communication watchdog |
| `uart_hal.c` / `.h` | UART hardware abstraction layer |
| `ir_sensor.c` / `.h` | ADC conversion, distance calculation, obstacle and edge thresholds |
| `joystick.c` / `.h` | Joystick acquisition, dead zone, out-of-range and cable-break detection |
| `motor.c` / `.h` | PWM generation and direction control for four drives |
| `adc.c` / `.h` | ADC driver |
| `betriebszustandsautomat.c` / `.h` | Operating state machine |
| `systemsteuerung.c` / `.h` | System supervision |
| `status_led.c` / `.h` | Status indication |

> **Note on naming:** the project was developed in German, so module and identifier
> names follow German terminology — `betriebszustandsautomat` is the operating state
> machine, `systemsteuerung` the system supervision.

---

## What I would do differently

The main loop runs freely, so the sampling interval depends on the current workload
rather than being fixed. In practice this produced a varying latency between a sensor
reading and the resulting braking decision.

The next step is a time-triggered design: a periodic timer sets a tick flag and the
sensor evaluation runs on that fixed interval, while the UART handler stays outside the
tick so that commands are still answered immediately. From there, moving to an RTOS with
separate tasks and priorities is a natural continuation.

---

## Note

This repository contains the firmware I wrote for my thesis. The hardware platform
belongs to Berliner Hochschule für Technik.
