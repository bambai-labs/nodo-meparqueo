// Definiciones de pines
#define BOTON_NEGRO 1
#define BOTON_ROJO 3

#include "LoRaWan_APP.h"
#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "images.h"

// Inicialización de la pantalla
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);

// Variables globales para la gestión de estados
int disponibilidadState = 0;  // 0 = Más de 5, 1 = Menos de 5, 2 = Sin disponibilidad
int accesoState = 0;          // 0 = Parqueadero Abierto, 1 = Parqueadero Cerrado
bool dataChanged = false;

// Nuevos parámetros OTAA
uint8_t devEui[] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x07, 0x17, 0xA9 };
uint8_t appEui[] = { 0xDA, 0x35, 0x55, 0x5A, 0xD1, 0x1D, 0xAD, 0x5A };
uint8_t appKey[] = { 0x01, 0x2F, 0xA5, 0x5A, 0x94, 0xD9, 0xD1, 0xD6, 0x79, 0x66, 0x64, 0x4A, 0x9E, 0x63, 0x9A, 0x2C };

// Parámetros ABP (no usados, definidos por compatibilidad)
uint8_t nwkSKey[] = { 0x15, 0xB1, 0xD0, 0xEF, 0xA4, 0x63, 0xDF, 0xBE, 0x3D, 0x11, 0x18, 0x1E, 0x1E, 0xC7, 0xDA, 0x85 };
uint8_t appSKey[] = { 0xD7, 0x2C, 0x78, 0x75, 0x8C, 0xDC, 0xCA, 0xBF, 0x55, 0xEE, 0x4A, 0x77, 0x8D, 0x16, 0xEF, 0x67 };
uint32_t devAddr = (uint32_t)0x007e6ae1;

// Máscara de canales (ejemplo para canales 0 a 7)
// uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
uint16_t userChannelsMask[6] = { 0xFF00, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };

// Región y Clase LoRaWAN
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;

// Otros parámetros
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;
uint32_t appTxDutyCycle = 60 * 1000;

// Función para encender la alimentación de la pantalla
void VextON() {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

/*!
 * Prepara el payload para la gestión de parqueaderos.
 * El payload tendrá:
 *  - Bytes 0-3: "P001"
 *  - Byte 4: disponibilidadState
 *  - Byte 5: accesoState
 */
static void preparePayloadParking(uint8_t port) {
  appData[0] = 'P';
  appData[1] = '0';
  appData[2] = '0';
  appData[3] = '2';
  appData[4] = (uint8_t)disponibilidadState; //0: MORE_THAN_FIVE, 1: LESS_THAN_FIVE, 2: NO_AVAILABILITY
  appData[5] = (uint8_t)accesoState; //0: OPEN, 1: CLOSED
  appDataSize = 6;
}

void downLinkDataHandle(McpsIndication_t *mcpsIndication) {
  Serial.printf("+REV DATA:%s,RXSIZE %d,PORT %d\r\n",
                mcpsIndication->RxSlot ? "RXWIN2" : "RXWIN1",
                mcpsIndication->BufferSize,
                mcpsIndication->Port);
  Serial.print("+REV DATA:");
  for (uint8_t i = 0; i < mcpsIndication->BufferSize; i++) {
    Serial.printf("%02X", mcpsIndication->Buffer[i]);
  }
  Serial.println();
}

// Función para actualizar la pantalla con información del parqueadero
void updateDisplay() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  
  // Mostrar nombre del parqueadero
  display.drawString(display.getWidth()/2, 0, "El Faro");
  
  // Mostrar línea separadora
  display.drawLine(10, 18, display.getWidth()-10, 18);
  
  // Mostrar estado de disponibilidad
  display.setFont(ArialMT_Plain_10);
  String disponibilidad;
  switch(disponibilidadState) {
    case 0:
      disponibilidad = "Disponible";
      break;
    case 1:
      disponibilidad = "Pocos Espacios";
      break;
    case 2:
      disponibilidad = "Lleno";
      break;
  }
  display.drawString(display.getWidth()/2, 25, disponibilidad);
  
  // Mostrar estado de acceso
  String acceso = (accesoState == 0) ? "Abierto" : "Cerrado";
  display.drawString(display.getWidth()/2, 40, acceso);
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar pantalla y mostrar splash screen
  VextON();
  delay(100);
  
  display.init();
  display.flipScreenVertically(); // Rotar la pantalla 180 grados
  display.clear();
  
  // Mostrar splash screen centrado
  int x = (display.width() - Logo_width) / 2;
  int y = (display.height() - Logo_height) / 2;
  display.drawXbm(x, y, Logo_width, Logo_height, Logo_bits);
  display.display();
  delay(2000);
  
  // Mostrar información inicial del parqueadero
  updateDisplay();
  
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // Configuración de pines para botones
  pinMode(BOTON_NEGRO, INPUT_PULLUP);
  pinMode(BOTON_ROJO, INPUT_PULLUP);
}

void loop() {
  // Procesa la máquina de estados de LoRaWAN
  switch (deviceState) {
    case DEVICE_STATE_INIT:
      {
#if (LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
        LoRaWAN.init(loraWanClass, loraWanRegion);
        LoRaWAN.setDefaultDR(3);
        break;
      }
    case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        preparePayloadParking(appPort);
        LoRaWAN.send();
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep(loraWanClass);
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }

  // Comprobación del botón para cambiar la disponibilidad (BOTON_NEGRO)
  if (digitalRead(BOTON_NEGRO) == LOW) {
    delay(50);  // Debounce
    if (digitalRead(BOTON_NEGRO) == LOW) {
      // Cicla los estados: 0 -> 1 -> 2 -> 0
      disponibilidadState = (disponibilidadState + 1) % 3;
      dataChanged = true;
      updateDisplay(); // Actualizar pantalla
      // Espera a que se libere el botón para evitar múltiples cambios
      while (digitalRead(BOTON_NEGRO) == LOW) { delay(10); }
    }
  }

  // Comprobación del botón para cambiar el acceso (BOTON_ROJO)
  if (digitalRead(BOTON_ROJO) == LOW) {
    delay(50);  // Debounce
    if (digitalRead(BOTON_ROJO) == LOW) {
      // Alterna entre 0 y 1
      accesoState = (accesoState + 1) % 2;
      dataChanged = true;
      updateDisplay(); // Actualizar pantalla
      // Espera a que se libere el botón
      while (digitalRead(BOTON_ROJO) == LOW) { delay(10); }
    }
  }

  // Si hubo cambios y el dispositivo está en estado SLEEP, se forzará el envío
  if (dataChanged && deviceState == DEVICE_STATE_SLEEP) {
    deviceState = DEVICE_STATE_SEND;
    dataChanged = false;
  }
}