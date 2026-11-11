Hardware-Tarea-3: Búsqueda del Tesoro en dos Arduino UNO
1. Descripción del Proyecto
Juego de búsqueda del tesoro desarrollado en dos placas Arduino UNO comunicadas vía UART.

Esclavo (Joystick): Lee y filtra dos potenciómetros (ejes X e Y), gestiona un botón de inicio (INPUT_PULLUP) y muestra la proximidad en dos LEDs (PWM).

Maestro (Juego): Administra la máquina de estados, genera la posición aleatoria del tesoro (2\times2 en grilla de 100\times100), calcula distancias y niveles, controla el temporizador (LED verde PWM) y define victoria o derrota.

2. Componentes Utilizados
Esclavo: 1x Arduino UNO, 2x Potenciómetros (X, Y), 1x Pulsador, 2x LEDs + resistencias.

Maestro: 1x Arduino UNO, 1x LED Verde (temporizador) + resistencia, 1x Pantalla LCD 16\times2 con módulo I2C (pines A4/A5).

3. Diagrama de Conexiones
El diagrama de conexiones se entrega de forma anexa en el repositorio de GitHub.

4. Pines y Conexión Serial
Enlace Serial: 9600 baudios (TX \rightarrow RX, RX \rightarrow TX, GND común) mediante SoftwareSerial.

Esclavo: Entradas analógicas A0/A1 (potenciómetros), botón de inicio y pines PWM para LEDs X e Y.

Maestro: Pin PWM para LED verde (temporizador), bus I2C para pantalla LCD 16\times2 y pines seriales por software.

5. Protocolo Serial y Robustez
Mensajes de 1 Byte (No bloqueantes):

Esclavo \rightarrow Maestro: Coordenada X (0-100), Coordenada Y (0-100), Solicitud de inicio.

Maestro \rightarrow Esclavo: Estado del juego (INACTIVO, JUGANDO, VICTORIA, DERROTA), Niveles de proximidad.

Transmisión Binaria: Uso de write() y recepción no bloqueante con available().

Timeout (2000 ms): Pérdida de enlace activa parpadeos de alerta. Se recupera automáticamente al reconectar.

6. Lógica de Funcionamiento
Conversión ADC Uniforme: Mapeo directo valor * 100L / 1024 para cubrir el rango 0-100 uniformemente. Se descarta la primera lectura al cambiar de canal analógico.

Niveles de Proximidad por Eje:

FRÍO (d > 10): PWM 0 (Apagado)

TIBIO (5 \le d \le 10): PWM 20 (Tenue)

CALIENTE (1 \le d \le 5): PWM 80 (Medio)

AQUÍ (d = 0): PWM 255 (Máximo)

Condición de Victoria: Mantener dx=dy=0 durante 2000 ms continuos antes de agotar los 60 s.

7. Mapeo de Conexiones y Pines
Arduino Esclavo (Joystick)
A0: Potenciómetro Eje X (Terminal central)

A1: Potenciómetro Eje Y (Terminal central)

Pin Digital 2: Pulsador de Inicio (Conectado a GND, usa INPUT_PULLUP)

Pin Digital 5 (PWM): LED Eje X (con resistencia limitadora)

Pin Digital 6 (PWM): LED Eje Y (con resistencia limitadora)

Pin Digital 10 (Rx): Conectado al Tx del Maestro

Pin Digital 11 (Tx): Conectado al Rx del Maestro

Arduino Maestro (Juego)
Pin Digital 9 (PWM): LED Verde de Temporizador (con resistencia limitadora)

Pin Digital 10 (Rx): Conectado al Tx del Esclavo

Pin Digital 11 (Tx): Conectado al Rx del Esclavo

A4 (SDA) / A5 (SCL): Pantalla LCD 16\times2 (Módulo I2C)

A0: Pin libre para semilla aleatoria (randomSeed(analogRead(A0)))

8. Instrucciones de Carga
Abrir Arduino IDE y cargar Esclavo.ino y Maestro.ino en sus respectivas placas.

Verificar las conexiones según el mapa de pines detallado arriba.

Presionar el pulsador en el Esclavo para iniciar la partida.

9. Declaración de Uso de Inteligencia Artificial
En el desarrollo de este proyecto, se empleó la herramienta de Inteligencia Artificial Gemini como apoyo y consulta técnica complementaria. Su utilización se centró en la estructuración conceptual del protocolo de comunicación, la revisión de buenas prácticas para la programación no bloqueante con millis() y la depuración general del código base, sirviendo como soporte en la toma de decisiones de diseño de software y hardware, sin reemplazar la autoría ni el desarrollo analítico del equipo de trabajo.