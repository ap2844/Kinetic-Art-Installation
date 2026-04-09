# Kinetic Breathing Installation

Kinetic art installation built for Art Incept in collaboration with artist Parul Sharma (Jan–Feb 2026).

The system uses an ESP32 and a PCA9685 servo driver to generate slow, rhythmic motion across suspended acrylic elements. Motion is transmitted using fishing lines and thumbtacks to convert servo rotation into pulsed movement.

---

## Overview

The installation was designed to produce a breathing-like motion instead of simple repetitive actuation. Each element moves with a phase offset, creating a staggered and more natural overall pattern.

---

## Hardware

- ESP32
- PCA9685 servo driver (16-channel)
- Servo motors
- Acrylic pieces
- Fishing line
- Thumbtacks
- Mounting structure
- External power supply

---

## Working Principle

Each servo is assigned a channel and driven using a sinusoidal position signal. Different phase offsets are applied across channels so that all elements do not move together.

Servo rotation is converted into linear or pulsed motion through fishing lines connected to the acrylic elements.

---

## Motion Model

θ_i(t) = θ_0 + A sin(ωt + φ_i)

- θ_0: neutral position  
- A: amplitude  
- ω: frequency  
- φ_i: phase offset per channel  

---

## Contributions

- Designed a kinetic art installation using an ESP32 and PCA9685 servo driver to create dynamic breathing motion
- Engineered fishing lines and thumbtacks to translate torque from servo motors into rhythmic pulses of acrylic pieces
- Developed custom control logic to generate unique sinusoidal motion to create separate channels with phase differences

---
