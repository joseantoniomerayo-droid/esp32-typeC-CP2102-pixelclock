# ESP32 Type-C CP2102

Placa de desarrollo ESP32 con conector USB Type-C y chip CP2102,
conduciendo 2 paneles LED RGB P4 64×32 por HUB75.

## Hardware

### Placa ESP32

| Componente | Detalle |
|---|---|
| **Módulo** | ESP-WROOM-32 (ESP32 dual-core) |
| **USB-UART** | CP2102 |
| **Conector** | Micro USB |
| **Flash** | 32Mbit (4MB) SPI |
| **Bluetooth** | 4.2 BR/EDR + BLE |
| **Antena** | PCB a bordo, 2dBi |
| **CPU** | 2× Xtensa LX6 @ 240MHz (600 DMIPS) |
| **GPIO** | 22 pines |
| **UART0** | TX=GPIO1, RX=GPIO3 @ 115200 bps |

**LEDs onboard**
- Rojo — power
- Azul — estado GPIO (pendiente confirmar pin)

**Botones**
- **RST** — reset
- **BOOT/FLASH** — mantener + reset → modo descarga

**Alimentación:** 5V USB o 5-12V por pin

### Paneles LED RGB — P4 64×32

| Parámetro | Valor |
|---|---|
| **Modelo** | P4 SMD 3-en-1 indoor |
| **Resolución** | 64 × 32 píxeles cada uno |
| **LED** | SMD2121/2020 (1R 1G 1B) |
| **Scan** | 1/16 corriente constante |
| **Tamaño** | 256 × 128 mm cada uno |
| **Paso** | 4 mm |
| **Tensión** | 5V DC |
| **Consumo** | ~220 W/m² medio, ~600 W/m² pico |
| **Brillo** | >500 nits |
| **Frecuencia refr.** | ≥1920 Hz |
| **Ángulo visión** | 160° V / 160° H |
| **Interfaz** | HUB75 (RGB H = señal H en HUB75) |

### HUB75 — pines del panel

Los paneles P4 64×32 con scan 1/16 usan el conector HUB75 estándar de 16 pines:

```
Pin    Nombre     Descripción
───    ─────      ───────────
 1     R1         Datos rojo — mitad superior
 2     G1         Datos verde — mitad superior
 3     B1         Datos azul — mitad superior
 4     GND        Tierra
 5     R2         Datos rojo — mitad inferior
 6     G2         Datos verde — mitad inferior
 7     B2         Datos azul — mitad inferior
 8     GND        Tierra
 9     A          Dirección bit 0
10     B          Dirección bit 1
11     C          Dirección bit 2
12     D          Dirección bit 3
13     CLK        Reloj
14     LAT        Latch (strobe)
15     OE         Output enable (blanking)
16     GND        Tierra
```

Los 2 paneles se encadenan: salida HUB75 del panel 1 → entrada del panel 2.

### Pinout HUB75 — placa DMDos V3

Los pines los fija la placa DMDos V3. No requiere cableado manual.

| HUB75 | GPIO ESP32 |
|---|---|
| R1 | 25 |
| G1 | 26 |
| B1 | 27 |
| R2 | 14 |
| G2 | 12 |
| B2 | 13 |
| A  | 33 |
| B  | 32 |
| C  | 22 |
| D  | 17 |
| E  | -1 (no usado, scan 1/16 → 4 líneas A-D bastan) |
| LAT | 4 |
| OE  | 15 |
| CLK | 16 |

**Paneles en cadena** (chain=2): la salida del panel 1 → entrada del panel 2.

> ⚠️ **GPIO12 (G2)** tiene pull-up interno que afecta al voltaje del flash.
> La librería HUB75-I2S-DMA lo maneja internamente. Si da problemas de boot,
> se puede reasignar a otro pin修改 la librería.
>
> 🔌 Los paneles se alimentan a **5V externos** (NO desde el ESP32 —
> cada panel puede consumir picos de ~40A a 5V). El ESP32 se alimenta desde
> la propia DMDos V3 por USB.

## Librería recomendada

**[ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA)** — usa el periférico I2S del ESP32
con DMA para generar las señales sin parpadeo y sin ocupar la CPU.
Compatible con ESP-IDF y Arduino.

## Requisitos

- ESP-IDF v5.x
- Python 3

## Compilar y flashear

```bash
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
