# PixelClock — Reloj LED HUB75 con ESP32

Reloj inteligente con 2 paneles LED RGB P4 64×32 (128×32 total),
controlado por ESP32 en placa DMDos V3. Clima vía Open-Meteo,
configuración por WiFiManager + Web UI vía MQTT.

![Hardware](https://img.shields.io/badge/ESP32-ESP--WROOM--32-blue)
![Paneles](https://img.shields.io/badge/Paneles-2×P4_64×32-orange)
![MQTT](https://img.shields.io/badge/MQTT-Mosquitto-green)

## Hardware

| Componente | Detalle |
|---|---|
| **Placa** | DMDos V3 (ESP32-D0WD-V3 + CP2102 + conector HUB75) |
| **Paneles** | 2× P4 64×32 RGB, scan 1/16, interfaz HUB75 |
| **Resolución** | 128 × 32 píxeles |
| **Alimentación** | ESP32 por USB-C, paneles por fuente 5V externa |

### Pinout HUB75 (fijo DMDos V3)

| HUB75 | GPIO | | HUB75 | GPIO |
|---|---|---|---|---|
| R1 | 25 | | G1 | 26 |
| B1 | 27 | | R2 | 14 |
| G2 | 12 | | B2 | 13 |
| A | 33 | | B | 32 |
| C | 22 | | D | 17 |
| E | -1 | | LAT | 4 |
| OE | 15 | | CLK | 16 |

## Funcionalidades

### Reloj
- Hora HH:MM:SS con gradiente de color configurable
- Fecha y día de la semana (marquee animado)
- Brillo automático día/noche con horarios configurables
- Gradiente independiente para día y noche

### Clima
- Datos meteorológicos desde [Open-Meteo](https://open-meteo.com/) (gratuito, sin API key)
- Temperatura e icono del tiempo (sol, nube, lluvia, nieve, niebla, tormenta)
- Coordenadas configurables vía web o MQTT
- Zona horaria automática según coordenadas

### Eventos de calendario
El ESP32 se suscribe a `reloj/events` para recibir eventos del día y mostrarlos en el panel.

**Formato MQTT (compacto):**
```json
{"e":[["14:00","Reunión equipo"],["17:30","Cita"],["","Todo el día"]]}
```
- `e`: array de eventos, cada uno es `[hora, título]`
- Hora vacía `""` para eventos de todo el día
- Máximo 8 eventos por mensaje
- Para limpiar: `{"e":[]}`

**En el panel LED:**
- Icono calendario bicolor (rojo + cyan) en esquina superior izquierda
- Texto con hora y título en cyan, fuente 6×8
- Los eventos rotan automáticamente cada 5 segundos
- Si el texto es demasiado largo para el espacio disponible, se trunca con `..`
- Los eventos cuya hora ya ha pasado se filtran automáticamente al cargar
- El filtro también se aplica durante la rotación: si la hora de un evento expira mientras se muestra, se salta

**Control:**
- `calendar_activo` (0/1) activa o desactiva la visualización de eventos, configurable vía `reloj/config`
- Por defecto está activo

**Ejemplo desde terminal:**
```bash
# Enviar eventos
mosquitto_pub -h BROKER_IP -t "reloj/events" \
  -m '{"e":[["14:00","Reunión equipo"]]}' \
  -u usuario -P contraseña

# Desactivar calendario
mosquitto_pub -h BROKER_IP -t "reloj/config" \
  -m '{"calendar_activo":0}' -u usuario -P contraseña
```

### Primer arranque (WiFiManager)
1. El ESP32 crea una red WiFi **PixelClock-AP** (sin contraseña)
2. El panel muestra un código QR para conectar directamente
3. Conéctate desde el móvil y se abrirá el portal de configuración
4. Introduce los datos de tu red WiFi y del broker MQTT
5. El ESP32 guarda la configuración en NVS y se conecta

### Reset de fábrica
Durante el funcionamiento normal, pulsa y mantén el botón **BOOT** durante **5 segundos**.
El panel mostrará la cuenta atrás y el ESP32 se reiniciará borrando toda la configuración.

### Web UI

Interfaz web que conecta al broker MQTT por WebSocket.
No requiere backend — solo HTML + JavaScript.

Para usarla, copia `index.html` al servidor web (Nginx en la VM del broker):

```bash
sudo cp index.html /var/www/html/index.html
```

**Secciones de la interfaz:**

| Sección | Parámetros |
|---|---|
| **Conexión MQTT** (colapsable) | Servidor (IP), Puerto (9001 por defecto — el que configuramos en Mosquitto para WebSocket, aunque puede ser cualquiera), Usuario, Contraseña |
| **Esquema de Color** | Brillo día/noche, Gradiente día/noche, Modo noche (toggle), Horario noche |
| **Fecha** | Mostrar fecha (toggle), Formato, Día semana |
| **Clima** | Latitud, Longitud, Refresco (minutos) |
| **Estado actual** | JSON de configuración recibido del ESP32 |

> **Nota:** Las secciones de configuración (Esquema Color, Fecha, Clima y botón Guardar)
> solo se muestran cuando hay conexión MQTT activa. Al desconectar, solo queda visible
> la tarjeta de Conexión y el Estado actual.

### Control por MQTT
Suscríbete a `reloj/status` para ver el estado actual.
Publica en `reloj/config` con formato JSON para cambiar parámetros:

```json
{
  "brillo_dia": 40,
  "brillo_noche": 1,
  "inicio_noche": 23,
  "fin_noche": 7,
  "gradiente": 0,
  "gradiente_noche": 2,
  "clima_refresh": 30,
  "clima_lat": 40.4168,
  "clima_lon": -3.7038
}
```

| Parámetro | Rango | Descripción |
|---|---|---|
| `brillo_dia` | 0–255 | Brillo en horario diurno |
| `brillo_noche` | 0–255 | Brillo en horario nocturno |
| `inicio_noche` | 0–23 | Hora de inicio del modo noche |
| `fin_noche` | 0–23 | Hora de fin del modo noche |
| `gradiente` | 0–1 | Gradiente diurno |
| `gradiente_noche` | 0–2 | Gradiente nocturno (2 = azul tenue) |
| `clima_refresh` | 5–120 | Minutos entre actualizaciones |
| `clima_lat` | -90–90 | Latitud |
| `clima_lon` | -180–180 | Longitud |
| `calendar_activo` | 0–1 | Mostrar eventos de calendario en el panel |

Ejemplo desde terminal:

```bash
# Ver estado
mosquitto_sub -h BROKER_IP -t "reloj/status" -u usuario -P contraseña

# Cambiar brillo nocturno
mosquitto_pub -h BROKER_IP -t "reloj/config" \
  -m '{"brillo_noche":2}' -u usuario -P contraseña
```

## Montar el broker MQTT

En cualquier máquina Debian/Ubuntu (o VM en ProxMox):

```bash
# Instalar Mosquitto
sudo apt update && sudo apt install -y mosquitto mosquitto-clients

# Configurar
sudo tee /etc/mosquitto/conf.d/pixelclock.conf << EOF
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd

listener 9001
protocol websockets
EOF

# Crear usuario y contraseña
sudo mosquitto_passwd -c /etc/mosquitto/passwd pixelclock
# (el sistema pedirá la contraseña dos veces)

# Reiniciar y habilitar
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto

# Verificar que escucha en ambos puertos
sudo netstat -tlnp | grep mosquitto
```

El ESP32 se conecta al broker automáticamente si configuras los datos
desde el portal WiFiManager (PixelClock-AP).

## Compilar y flashear

```bash
# Requisitos: PlatformIO CLI (pipx install platformio)

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
├── platformio.ini          ← Dependencias y configuración de compilación
├── index.html              ← Web UI (copiar en Nginx de la VM)
├── include/
│   ├── clock.h             ← API del módulo de reloj
│   ├── display.h           ← Configuración de pines HUB75
│   ├── mqtt_handler.h      ← Cliente MQTT
│   ├── nvs_config.h        ← Persistencia en NVS (flash)
│   └── qr_display.h        ← Código QR para configuración WiFi
├── src/
│   ├── main.cpp            ← Setup, bucle principal y WiFiManager
│   ├── clock.cpp           ← Reloj, clima, marquee animado
│   ├── display.cpp         ← Inicialización del panel HUB75
│   ├── mqtt_handler.cpp    ← Conexión MQTT y procesamiento de comandos
│   ├── nvs_config.cpp      ← Lectura/escritura de configuración persistente
│   └── qr_display.cpp      ← Renderizado del código QR en el panel
└── README.md
```

## Dependencias

| Librería | Propósito |
|---|---|
| `ESP32 HUB75 LED MATRIX PANEL DMA Display` | Driver I2S-DMA para paneles HUB75 |
| `Adafruit GFX Library` | Gráficos básicos para el display |
| `WiFiManager` | Portal cautivo para configuración inicial |
| `PubSubClient` | Cliente MQTT |
| `ArduinoJson` | Parseo de mensajes JSON |

## Licencia

MIT
