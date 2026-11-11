#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pines que usamos
const uint8_t LedVerde = 9;
const uint8_t PinAzar  = A0; // pin sin conectar, lo usamos para generar numeros aleatorios

// Comunicacion serial con el esclavo (RX en 10, TX en 11)
SoftwareSerial Enlace(10, 11);

// Pantalla LCD con direccion I2C 0x27 (si no funciona probar con 0x3F)
LiquidCrystal_I2C Pantalla(0x27, 16, 2);

enum Estado { INACTIVO, JUGANDO, VICTORIA, DERROTA };
Estado EstadoJuego = INACTIVO;

uint32_t TiempoPartida   = 0;
uint32_t TiempoRecepcion = 0;
uint32_t TiempoEnvio     = 0;
uint32_t TiempoVictoria  = 0;
uint32_t TiempoAnimacion = 0;
uint32_t TiempoPantalla  = 0;

uint8_t PosicionX = 0;
uint8_t PosicionY = 0;
uint8_t TesoroX   = 0;
uint8_t TesoroY   = 0;
uint8_t NivelX    = 0;
uint8_t NivelY    = 0;

uint8_t ParpadeosDerrota = 0;
bool    AlternarLed      = false;

// Calcula que tan cerca esta el jugador del tesoro (0=frio, 3=aqui)
uint8_t obtenerNivel(uint8_t pos, uint8_t tesoro) {
  uint8_t distancia = (pos >= tesoro) ? (uint8_t)(pos - tesoro) : (uint8_t)(tesoro - pos);
  if (distancia <= 1) distancia = 0; // si esta a 1 unidad lo contamos como "aqui"

  if (distancia == 0)  return 3; // AQUI
  if (distancia <= 5)  return 2; // CALIENTE
  if (distancia <= 10) return 1; // TIBIO
  return 0;                      // FRIO
}

// Actualiza la pantalla cada 150ms segun el estado del juego
void actualizarPantalla(uint32_t ahora) {
  if (ahora - TiempoPantalla < 150) return;
  TiempoPantalla = ahora;
  Pantalla.setCursor(0, 0);

  if (ahora - TiempoRecepcion > 2000) {
    Pantalla.print("LINK PERDIDO!   ");
    Pantalla.setCursor(0, 1);
    Pantalla.print("Revisar cables  ");
  } else if (EstadoJuego == INACTIVO) {
    Pantalla.print("BUSQUEDA TESORO ");
    Pantalla.setCursor(0, 1);
    Pantalla.print("Pulsa INICIO    ");
  } else if (EstadoJuego == JUGANDO) {
    uint32_t transcurrido = ahora - TiempoPartida;
    uint8_t segundos = (transcurrido < 60000) ? (uint8_t)((60000UL - transcurrido) / 1000UL) : 0;
    char fila0[17];
    char fila1[17];
    const char* textoNivel[] = {"FRIO", "TIBI", "CALI", "AQUI"};

    sprintf(fila0, "X:%02d Y:%02d T:%02ds ", PosicionX, PosicionY, segundos);
    sprintf(fila1, "X:%s Y:%s  ", textoNivel[NivelX], textoNivel[NivelY]);

    Pantalla.print(fila0);
    Pantalla.setCursor(0, 1);
    Pantalla.print(fila1);
  } else if (EstadoJuego == VICTORIA) {
    Pantalla.print("  HAS GANADO!   ");
    Pantalla.setCursor(0, 1);
    Pantalla.print("Tesoro hallado  ");
  } else if (EstadoJuego == DERROTA) {
    Pantalla.print(" TIEMPO AGOTADO ");
    Pantalla.setCursor(0, 1);
    Pantalla.print("Fin de partida  ");
  }
}

void setup() {
  pinMode(LedVerde, OUTPUT);
  analogWrite(LedVerde, 0);

  Enlace.begin(9600);
  Pantalla.init();
  Pantalla.backlight();

  TiempoRecepcion = millis();
}

void loop() {
  uint32_t ahora = millis();

  // Leemos los datos que manda el esclavo
  while (Enlace.available() > 0) {
    uint8_t dato = Enlace.read();
    TiempoRecepcion = ahora;

    if (dato == 'S' && EstadoJuego == INACTIVO) {
      // El esclavo presiono inicio: generamos posicion aleatoria del tesoro
      randomSeed(analogRead(PinAzar));
      TesoroX     = random(12, 101);
      TesoroY     = random(12, 101);
      EstadoJuego = JUGANDO;
      TiempoPartida  = ahora;
      TiempoVictoria = 0;
    } else if (dato == 'S') {
      // ignoramos 'S' si el juego ya esta corriendo
    } else if ((dato & 0x80) == 0) {
      PosicionX = dato & 0x7F; // bit 7 en 0 = coordenada X
    } else {
      PosicionY = dato & 0x7F; // bit 7 en 1 = coordenada Y
    }
  }

  // Si no recibimos datos en 2 segundos asumimos que se corto la conexion
  if (ahora - TiempoRecepcion > 2000) {
    if (EstadoJuego == JUGANDO) EstadoJuego = INACTIVO;
    if (ahora - TiempoAnimacion >= 250) {
      TiempoAnimacion = ahora;
      AlternarLed     = !AlternarLed;
      digitalWrite(LedVerde, AlternarLed ? HIGH : LOW);
    }
  }
  else if (EstadoJuego == JUGANDO) {
    uint32_t transcurrido = ahora - TiempoPartida;

    if (transcurrido >= 60000) {
      EstadoJuego      = DERROTA;
      ParpadeosDerrota = 0;
      TiempoAnimacion  = ahora;
    } else {
      // El LED verde va bajando de intensidad a medida que pasa el tiempo
      uint32_t restante = 60000 - transcurrido;
      analogWrite(LedVerde, (uint8_t)((restante * 255UL) / 60000UL));

      NivelX = obtenerNivel(PosicionX, TesoroX);
      NivelY = obtenerNivel(PosicionY, TesoroY);

      // Si el jugador se queda 1 segundo encima del tesoro, gana
      if (NivelX == 3 && NivelY == 3) {
        if (TiempoVictoria == 0) {
          TiempoVictoria = ahora;
        } else if (ahora - TiempoVictoria >= 1000) {
          EstadoJuego     = VICTORIA;
          TiempoAnimacion = ahora;
        }
      } else {
        TiempoVictoria = 0;
      }
    }
  } else if (EstadoJuego == VICTORIA) {
    analogWrite(LedVerde, 255); // LED al maximo durante la victoria
    if (ahora - TiempoAnimacion >= 5000) {
      EstadoJuego = INACTIVO;
    }
  } else if (EstadoJuego == DERROTA) {
    if (ParpadeosDerrota < 20) {
      if (ahora - TiempoAnimacion >= 250) {
        TiempoAnimacion  = ahora;
        AlternarLed      = !AlternarLed;
        digitalWrite(LedVerde, AlternarLed ? HIGH : LOW);
        ParpadeosDerrota++;
      }
    } else {
      EstadoJuego = INACTIVO;
    }
  } else {
    analogWrite(LedVerde, 0);
  }

  // Cada 40ms le mandamos el estado al esclavo
  if (ahora - TiempoEnvio >= 40) {
    TiempoEnvio = ahora;
    if (EstadoJuego == JUGANDO) {
      // Empaquetamos los dos niveles en un byte (bit 7 siempre en 1)
      Enlace.write(0x80 | ((NivelX & 0x03) << 2) | (NivelY & 0x03));
    } else if (EstadoJuego == INACTIVO)  Enlace.write('I');
    else if (EstadoJuego == VICTORIA)    Enlace.write('V');
    else if (EstadoJuego == DERROTA)     Enlace.write('D');
  }

  // Refrescamos la pantalla
  actualizarPantalla(ahora);
}