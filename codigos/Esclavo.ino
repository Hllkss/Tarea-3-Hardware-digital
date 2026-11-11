#include <SoftwareSerial.h>

// Pines que vamos a usar
const uint8_t PotenciometroX = A0;
const uint8_t PotenciometroY = A1;
const uint8_t BotonInicio    = 2;
const uint8_t LedX           = 5;
const uint8_t LedY           = 6;

// Brillos para cada nivel: apagado, poco, medio, maximo
const uint8_t BrillosPWM[4] = {0, 20, 80, 255};

// Comunicacion serial con el maestro (RX en 8, TX en 9)
SoftwareSerial Enlace(8, 9);

uint32_t TiempoEnvio     = 0;
uint32_t TiempoRecepcion = 0;
uint32_t TiempoAnimacion = 0;
uint32_t TiempoBoton     = 0;
uint32_t TiempoEnvioS    = 0; // para reenviar 'S' mientras el boton esta presionado

char    EstadoActual      = 'I'; // I=Inactivo, J=Jugando, V=Victoria, D=Derrota
uint8_t NivelX            = 0;
uint8_t NivelY            = 0;
uint8_t ContadorParpadeos = 0;
uint8_t OffsetY           = 0; // guarda el valor inicial del pot Y para corregirlo

bool    AlternarLed   = false;
uint8_t BotonAnterior = HIGH;

// Lee el potenciometro varias veces y devuelve un promedio en rango 0-100
// descartamos 2 lecturas al inicio porque el ADC tarda en estabilizarse al cambiar de canal
uint8_t leerCoordenada(uint8_t pin) {
  analogRead(pin);
  analogRead(pin); // dos descartes para que el ADC se estabilice bien
  uint32_t suma = 0;
  for (uint8_t i = 0; i < 8; i++) { // 8 muestras para suavizar el ruido
    suma += analogRead(pin);
  }
  uint16_t promedio = suma / 8;
  return (uint8_t)constrain(map(promedio, 0, 1023, 0, 100), 0, 100);
}

void setup() {
  pinMode(BotonInicio, INPUT_PULLUP);
  pinMode(LedX, OUTPUT);
  pinMode(LedY, OUTPUT);

  analogWrite(LedX, 0);
  analogWrite(LedY, 0);

  Enlace.begin(9600);
  TiempoRecepcion = millis();

  // Guardamos el valor inicial del pot Y para usarlo como cero de referencia
  // Esto arregla el problema de que TinkerCAD arranca el pot en posicion media
  OffsetY = leerCoordenada(PotenciometroY);
}

void loop() {
  uint32_t ahora = millis();

  // Detectamos el boton con antirrebote de 50ms
  uint8_t boton = digitalRead(BotonInicio);
  if (boton != BotonAnterior && (ahora - TiempoBoton > 50)) {
    TiempoBoton   = ahora;
    BotonAnterior = boton;
  }

  // Mientras el boton este presionado, enviamos 'S' cada 200ms
  // (lo repetimos porque SoftwareSerial puede perder bytes si ambos transmiten a la vez)
  if (BotonAnterior == LOW && EstadoActual == 'I' && (ahora - TiempoEnvioS >= 200)) {
    TiempoEnvioS = ahora;
    Enlace.write('S');
  }

  // Leemos lo que nos manda el maestro
  while (Enlace.available() > 0) {
    uint8_t dato = Enlace.read();
    TiempoRecepcion = ahora;

    if (dato == 'I' || dato == 'V' || dato == 'D') {
      if (EstadoActual != dato) {
        EstadoActual      = (char)dato;
        ContadorParpadeos = 0;
      }
    } else if ((dato & 0x80) == 0x80) {
      // El maestro manda los niveles X e Y empaquetados en un solo byte
      EstadoActual = 'J';
      NivelX = (dato >> 2) & 0x03;
      NivelY = dato & 0x03;
    }
  }

  // Manejamos los LEDs segun el estado
  if (ahora - TiempoRecepcion > 2000) {
    // Si llevamos mas de 2 segundos sin recibir nada, parpadeamos alternado
    if (ahora - TiempoAnimacion >= 500) {
      TiempoAnimacion = ahora;
      AlternarLed     = !AlternarLed;
      digitalWrite(LedX, AlternarLed ? HIGH : LOW);
      digitalWrite(LedY, AlternarLed ? LOW  : HIGH);
    }
  } else if (EstadoActual == 'J') {
    // Durante el juego mostramos la proximidad con el brillo
    analogWrite(LedX, BrillosPWM[NivelX]);
    analogWrite(LedY, BrillosPWM[NivelY]);
  } else if (EstadoActual == 'V' && ContadorParpadeos < 20) {
    // Victoria: ambos leds parpadean juntos
    if (ahora - TiempoAnimacion >= 250) {
      TiempoAnimacion = ahora;
      AlternarLed     = !AlternarLed;
      ContadorParpadeos++;
      digitalWrite(LedX, AlternarLed ? HIGH : LOW);
      digitalWrite(LedY, AlternarLed ? HIGH : LOW);
    }
  } else {
    analogWrite(LedX, 0);
    analogWrite(LedY, 0);
  }

  // Cada 40ms enviamos la posicion del joystick al maestro
  if (ahora - TiempoEnvio >= 40) {
    TiempoEnvio = ahora;
    uint8_t x = leerCoordenada(PotenciometroX);
    uint8_t y = leerCoordenada(PotenciometroY);

    // restamos el offset inicial para que Y arranque siempre desde 0
    y = (y >= OffsetY) ? (y - OffsetY) : 0;
    y = constrain(y, 0, 100);

    // Usamos el bit 7 para distinguir X (bit7=0) de Y (bit7=1)
    Enlace.write(x & 0x7F);
    Enlace.write((y & 0x7F) | 0x80);
  }
}