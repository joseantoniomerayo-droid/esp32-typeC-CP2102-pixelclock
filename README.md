# ESP32 Type-C CP2102

Proyecto para ESP32 con conector USB Type-C y chip CP2102.

## Hardware

- **MCU:** ESP32
- **USB-UART:** CP2102
- **Conector:** USB Type-C

## Requisitos

- ESP-IDF
- Python 3

## Compilar y flashear

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
