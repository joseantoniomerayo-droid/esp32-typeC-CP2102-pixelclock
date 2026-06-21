# PixelClock — Reloj LED HUB75 con ESP32

Reloj inteligente con 2 paneles LED RGB P4 64×32 (128×32 total), controlado
por ESP32 en placa DMDos V3. Clima vía Open-Meteo, configuración por
WiFiManager + MQTT.

## Hardware

| Componente | Detalle |
|---|---|
| **Placa** | DMDos V3 (ESP32-D0WD-V3 + CP2102 + HUB75) |
| **Paneles** | 2× P4 64×32 RGB, scan 1/16, HUB75 |
| **Resolución** | 128×32 píxeles |
| **Alimentación** | ESP32 por USB, paneles por fuente 5V externa |

### Pinout HUB75 (fijo DMDos V3)

| HUB75 | GPIO | HUB75 | GPIO |
|---|---|---|---|
| R1 | 25 | G1 | 26 |
| B1 | 27 | R2 | 14 |
| G2 | 12 | B2 | 13 |
| A | 33 | B | 32 |
| C | 22 | D | 17 |
| E | -1 | LAT | 4 |
| OE | 15 | CLK | 16 |

## Funcionalidades

### Reloj
- Hora, fecha y día de la semana
- Gradiente configurable (cian→rojo / amarillo→rojo)
- Marquee inferior animado (entra/sale 4 ciclos/min)
- Brillo automático día/noche (horas configurables)

### Clima
- Datos de [Open-Meteo](https://open-meteo.com/) (gratis, sin API key)
- Coordenadas configurables
- Iconos: sol, nube, lluvia, nieve, niebla, tormenta

### Configuración WiFi
- **WiFiManager**: en primer arranque, crea AP `PixelClock-AP`
- **QR** en los paneles para conectar directamente
- Portal cautivo en `192.168.4.1`
- Credenciales guardadas en NVS (persisten al reiniciar)

### Control remoto (MQTT)
- Broker MQTT en servidor local
- Topic `reloj/config` (JSON) para cambiar parámetros
- Topic `reloj/status` para leer estado actual

#### Parámetros configurables por MQTT

```json
{
  "brillo_dia": 40,
  "brillo_noche": 1,
  "inicio_noche": 23,
  "fin_noche": 7,
  "gradiente": 0,
  "clima_refresh": 30,
  "clima_lat": 40.4168,
  "clima_lon": -3.7038
}
```

## Compilar y flashear

```bash
# Instalar dependencias (primera vez)
pio pkg install

# Compilar
pio run

# Flashear
pio run --target upload --upload-port /dev/ttyUSB0

# Monitor serie
pio device monitor
```

## Estructura del proyecto

```
├── platformio.ini          ← dependencias y configuración
├── include/
│   ├── clock.h             ← API del reloj
│   ├── display.h           ← pinout DMDos V3 + initDisplay()
│   ├── mqtt_handler.h      ← cliente MQTT
│   ├── nvs_config.h        ← persistencia en flash
│   └── qr_display.h        ← QR code para setup WiFi
├── src/
│   ├── main.cpp            ← setup + loop (WiFiManager, MQTT, display)
│   ├── clock.cpp           ← reloj + clima + marquee animado
│   ├── display.cpp         ← init HUB75 panels
│   ├── mqtt_handler.cpp    ← callbacks y publicación MQTT
│   ├── nvs_config.cpp      ← getters/setters NVS
│   └── qr_display.cpp      ← render QR 29×29 en panel
└── README.md
```

## Dependencias (PlatformIO)

- `ESP32 HUB75 LED MATRIX PANEL DMA Display` — driver I2S-DMA para HUB75
- `Adafruit GFX Library` — gráficos básicos
- `WiFiManager` — portal cautivo para configuración WiFi
- `PubSubClient` — cliente MQTT
- `ArduinoJson` — parseo de mensajes JSON

## Licencia

MIT
