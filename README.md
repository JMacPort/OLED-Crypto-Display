# STM32 Bitcoin Price Tracker

A bare metal implementation of a Bitcoin price tracker using STM32F446RE Nucleo, ESP32, and OLED display. The system fetches real-time Bitcoin prices through an API and displays them on an OLED screen.

## Hardware Components
- STM32F446RE Development Board
- ESP32 WiFi Module
- OLED Display (I2C, 0x3C)
- Breadboard Power Module

## Features
- Real-time Bitcoin price updates every 30 seconds
- Bare metal implementation (no HAL)
- Multiple communication protocols:
  - I2C for OLED display control
  - UART for ESP32 communication (115200 baud)
  - UART for debug output (115200 baud)
- Timer-based interrupt scheduling
- Font implementation for OLED display

## Implementation Details

### Communication Protocols
- **I2C**: Implements complete I2C protocol for OLED communication including:
  - Start/Stop conditions
  - Address sending with read/write bit
  - Data transmission
  - Busy state checking
  - Timeout mechanisms

- **UART**: Dual UART implementation
  - USART1: ESP32 communication
  - USART2: Debug output
  - Configured for 115200 baud rate
  - Error checking and timeout handling

### Timer Configuration
- Timer2 configured for 30-second intervals
- Uses interrupt-based timing for regular price updates
- Prescaler and auto-reload values optimized for timing accuracy

### OLED Display
- 128x64 OLED display
- 5x7 font implementation
- Functions for:
  - Screen clearing
  - Cursor positioning
  - Character and string writing
  - Command sending

### ESP32 WiFi Communication
- SSL connection to CryptoCompare API
- HTTP GET request implementation
- Response parsing for price extraction
- Timeout handling for reliable communication

## Pin Configurations
- **I2C (OLED)**
  - PB8: SCL
  - PB9: SDA

- **UART1 (ESP32)**
  - PA9: TX
  - PA10: RX

- **UART2 (Debug)**

## Setup and Usage
1. Connect OLED display to I2C pins
2. Connect ESP32 to UART1 pins
3. Optional: Connect debug terminal to UART2
4. Flash the program to STM32F446RE
5. System will automatically start fetching and displaying Bitcoin prices

## Register-Level Details
All peripherals are configured directly through registers:
- Clock configuration
- GPIO alternate function setup
- Communication protocol initialization
- Timer setup
- Interrupt configuration

## Demo

![Full Setup](/images/Full_Setup.jpeg)

## Notes
This project is implemented without relying on HAL or other middleware, demonstrating direct register manipulation and bare metal programming concepts.