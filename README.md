# Kiwi

Kiwi is a mobile AI assistant robot currently being developed by combining an autonomous navigation system and a voice assistant system.

## Navigation System

Built using:
- Arduino Nano
- L298N Motor Driver
- 2 DC Motors
- 3 IR Sensors

Current capabilities:
- Autonomous movement
- Obstacle avoidance

Planned upgrades:
- Ultrasonic sensor-based detection
- More functions

## Voice Assistant System

Built using:
- ESP32
- INMP441 Microphone
- MAX98357A DAC
- Speaker
- 128×64 OLED Display

Current flow:

Button Press → Voice Recording → Speech-to-Text → AI Processing → Text-to-Speech → Speaker Output

Planned upgrades:
- Wake-word activation
- Continuous listening mode
- facial expressions and animations

## Future Integration

The goal is to combine both systems so that Kiwi can:

- Respond to spoken commands
- Control its movement through voice
- Navigate autonomously
- Express status and emotions through animated eyes
- Function as a mobile AI companion robot

## Project Status

Both the navigation and voice assistant systems are working independently.

Current focus:
- Integrating the ESP32 and Arduino Nano
- Implementing voice-controlled movement
- Preparing for a single-board ESP32 architecture in future versions
