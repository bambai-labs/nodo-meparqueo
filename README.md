# nodo-meparqueo

Firmware Arduino (C++) del nodo físico de MeParqueo: un dispositivo Heltec ESP32 con pantalla OLED y dos botones que el encargado del parqueadero usa para reportar disponibilidad y acceso en tiempo real vía LoRaWAN.

## Parte del ecosistema MeParqueo

MeParqueo es un sistema IoT de parqueo inteligente en tiempo real (Montería, Colombia). Este repo contiene el firmware de los nodos que originan los datos de disponibilidad.

| Repo | Rol | Acceso |
| --- | --- | --- |
| [app-meparqueo](https://github.com/bambai-labs/app-meparqueo) | App móvil React Native para conductores | Público |
| [api-meparqueo](https://github.com/bambai-labs/api-meparqueo) | Backend NestJS | Privado |
| **nodo-meparqueo** (este repo) | Firmware del nodo físico LoRaWAN | Privado |
| [web-meparqueo](https://github.com/bambai-labs/web-meparqueo) | Frontend web | Privado |
| [landing-meparqueo](https://github.com/bambai-labs/landing-meparqueo) | Landing page | Privado |
| [survey-meparqueo](https://github.com/bambai-labs/survey-meparqueo) | Encuestas y validación | Privado |

## ✨ Características

- **Operación manual con dos botones físicos**: el botón negro cicla la disponibilidad (Disponible → Pocos espacios → Lleno) y el botón rojo alterna el acceso (Abierto / Cerrado), con debounce por software.
- **Pantalla OLED SSD1306 128x64**: splash screen con el logo al arrancar y luego nombre del parqueadero, disponibilidad y estado de acceso.
- **Uplink LoRaWAN Clase A por OTAA**: ADR activado, uplinks confirmados con 4 reintentos, puerto de aplicación 2, DR3 por defecto y máscara de canales 8–15 (subbanda 2).
- **Payload compacto de 6 bytes**: identificador del parqueadero en ASCII + estado de disponibilidad + estado de acceso.
- **Envío periódico y por evento**: ciclo de transmisión de 60 s, y si el operador cambia un estado el nodo fuerza un envío inmediato al salir del sleep.
- **Downlinks registrados por serial** (en v1 además parpadea el LED azul al recibir respuesta).
- **Dos variantes de nodo**:
  - `v1`: parqueadero "Area Chica" (ID `P001`), con LEDs azul/rojo indicadores de estado.
  - `v2`: parqueadero "El Faro" (ID `P002`), sin LEDs y con la pantalla rotada 180°.

### Formato del payload

| Byte | Contenido |
| --- | --- |
| 0–3 | ID del parqueadero en ASCII (`P001` en v1, `P002` en v2) |
| 4 | Disponibilidad: `0` más de 5 espacios, `1` menos de 5, `2` sin disponibilidad |
| 5 | Acceso: `0` abierto, `1` cerrado |

## 🛠️ Stack

- **C++ / Arduino** (sketches `.ino`, sin PlatformIO)
- **Placa Heltec ESP32 LoRa** con OLED integrada (framework de Heltec: `LoRaWan_APP.h`, `HT_SSD1306Wire.h`, `Mcu.begin(HELTEC_BOARD, ...)`)
- **LoRaWAN** Clase A, activación OTAA, ADR, uplinks confirmados
- Logos del splash como bitmaps XBM embebidos (`images.h`)

## 📁 Estructura del proyecto

```
v1/           Firmware del nodo 1 ("Area Chica", P001): botones + LEDs de estado
  v1.ino      Sketch principal (LoRaWAN, pantalla, botones, LEDs)
  images.h    Logo 50x50 en XBM para el splash screen
v2/           Firmware del nodo 2 ("El Faro", P002): botones, sin LEDs, pantalla rotada
  v2.ino      Sketch principal
  images.h    Logo del splash
```

## 🚀 Desarrollo local

No hay `platformio.ini` ni Makefile: el flujo de compilación es con **Arduino IDE**.

1. Instala Arduino IDE y agrega el soporte de placas **Heltec ESP32** desde el Boards Manager (provee `LoRaWan_APP` y `HT_SSD1306Wire`).
2. Selecciona la placa Heltec con LoRa correspondiente y configura la **región LoRaWAN** en el menú de la placa (el código usa `ACTIVE_REGION`, definida por esa configuración).
3. Abre `v1/v1.ino` o `v2/v2.ino` según el nodo a flashear.
4. Ajusta las credenciales OTAA del dispositivo (`devEui`, `appEui`, `appKey`) registradas en tu network server, y el ID del parqueadero en `preparePayloadParking()`.
5. Compila y sube el sketch por USB.
6. Abre el monitor serie a **115200 baudios** para ver el join, los envíos y los downlinks.

Notas de hardware según el sketch:

- `v1`: botón negro en GPIO 7, botón rojo en GPIO 5, LED azul en GPIO 1, LED rojo en GPIO 3.
- `v2`: botón negro en GPIO 1, botón rojo en GPIO 3 (sin LEDs).
- La pantalla se alimenta activando `Vext` y usa la dirección I2C `0x3C`.

## 👥 Hecho por bambai-labs

Desarrollado por el equipo de [bambai-labs](https://github.com/bambai-labs) para MeParqueo.
