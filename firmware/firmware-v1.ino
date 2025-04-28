#include <SD.h>
#include <Wire.h> 
#include "RTClib.h"


#define DEBUG_MODE false // Mensajes por serie

/// PINES ARDUINO UNO ///
// PIN 0 -> RX
// PIN 1 -> TX
#define PIN_LOGGING_ON_OFF 2 // Pin para encender/apagar registro de datos
#define PIN_PTO 3
// D4 a D7 -> pci (ver configuracion en setup)
#define PIN_CNTR_0 4 // Pin contador 0 (debe ser entre los pines 0 a 7)
#define PIN_CNTR_1 5 // Pin contador 1 (debe ser entre los pines 0 a 7)
#define PIN_CNTR_2 6 // Pin contador 2 (debe ser entre los pines 0 a 7)
#define PIN_CNTR_3 7 // Pin contador 3 (debe ser entre los pines 0 a 7)
#define PIN_LED_LOGGING_OK 8 // Pin para el led de estado de la grabacion de datos 
#define PIN_LED_ERROR 9 // Pin para el led de errores
// PIN 10 -> CS
// PIN 11 -> MOSI
// PIN 12 -> MISO
// PIN 13 -> CLK
#define PIN_AN_0 A3 
#define PIN_AN_1 A2
#define PIN_AN_2 A1
#define PIN_AN_3 A0
// PIN A4 -> SDA
// PIN A5 -> SCL

// Indices del registro de salida
#define OUTPUT_DT 0
#define OUTPUT_A0 1
#define OUTPUT_A1 2
#define OUTPUT_A2 3
#define OUTPUT_A3 4
#define OUTPUT_CNTR_0 5 
#define OUTPUT_CNTR_1 6
#define OUTPUT_CNTR_2 7
#define OUTPUT_CNTR_3 8

// Constates
#define BAUDRATE 57600 // Velocidad de comunicacion serie
#define MAX_NUM_LOGFILES 1000 // Maxima cantidad de registros
#define MAX_NUM_DIG 3 // Cantidad de digitos del indice del nombre de archivos
#define LOGFILE_NAME_PREFIX "LOG_" // Prefijo para los nombres de los registros
#define LOGFILE_NAME_LENGTH 8 // Cantidad de caracteres del nombre de los registros

// Variables globales
File logfile; // Archivo de registro
char logfileName[8];// Nombre del archivo de registro
RTC_DS1307 rtc; // Objeto para controlar reloj de tiempo real
bool dataLogging = false; // Estado del registrador grabando/apagado
bool error = false; // Deteccion de errores
byte timertick = 0; // Contador de ticks para la planificacion de tareas
unsigned int outputs_1[9] = {0,0,0,0,0,0,0,0,0}; // Vector de valores a registrar
unsigned long outputs_2 = 0; // Contador de rpm PTO
bool pin_cntr_state[4] = {false, false, false, false}; // Estado de los pines contadores


void setup()
// Tareas al inicio
{
  // Inicializar librerias
  Serial.begin(BAUDRATE);
  Wire.begin();
  rtc.begin();
  
  // Inicializar SD
  pinMode(10, OUTPUT);
  if (!SD.begin()) {
      if(DEBUG_MODE) Serial.println("ERROR [setup]: No se pudo inicializar SD");
      error = true; // Entrar en modo de error
      return;
  }

  pinMode(PIN_LOGGING_ON_OFF, INPUT); // Pin para encender/apagar registro de datos
  pinMode(PIN_LED_LOGGING_OK, OUTPUT); // Pin para encender/apagar led del registrador
  pinMode(PIN_LED_ERROR, OUTPUT); // Pin para encender/apagar led de alertas
  // Pines entradas sensores inductivos
  pinMode(PIN_CNTR_0, INPUT);
  pinMode(PIN_CNTR_1, INPUT);
  pinMode(PIN_CNTR_2, INPUT);
  pinMode(PIN_CNTR_3, INPUT);
  pinMode(PIN_PTO, INPUT);
  // Pines de entradas analogicas
  pinMode(PIN_AN_0, INPUT);
  pinMode(PIN_AN_1, INPUT);
  pinMode(PIN_AN_2, INPUT);
  pinMode(PIN_AN_3, INPUT);

  // Capturar estado de los pines
  pin_cntr_state[0] = digitalRead(PIN_CNTR_0) == HIGH;
  pin_cntr_state[1] = digitalRead(PIN_CNTR_1) == HIGH;
  pin_cntr_state[2] = digitalRead(PIN_CNTR_2) == HIGH;
  pin_cntr_state[3] = digitalRead(PIN_CNTR_3) == HIGH;

  // Leds de estado del registrador inicialmente apagado
  digitalWrite(PIN_LED_LOGGING_OK, LOW);
  digitalWrite(PIN_LED_ERROR, LOW);

  noInterrupts();
  // PCI D0-D7
  PCMSK2 |= (1 << PCINT20); // Pin D4
  PCMSK2 |= (1 << PCINT21); // Pin D5
  PCMSK2 |= (1 << PCINT22); // Pin D6
  PCMSK2 |= (1 << PCINT23); // Pin D7
  PCIFR |= (1 << PCIF2); // Limpiar interrupciones
  PCICR |= (1 << PCIE2); // Habilitar PCI pines D7-D0

  attachInterrupt(1, ptoPinRising, RISING);

  // TIMER 2
  TCCR2A = 0; // Normal operation
  TCCR2B = 0; // Normal operation
  TCNT2 = 0; // Inicializar en 0
  OCR2A = 156; // Registro de comparacion = 16MHz/1024/100.16Hz ()
  TCCR2A |= (1 << WGM21); // Modo CTC
  TCCR2B |= (1 << CS20); // 1024 prescaler
  TCCR2B |= (1 << CS21); // 1024 prescaler
  TCCR2B |= (1 << CS22); // 1024 prescaler
  TIMSK2 |= (1 << OCIE2A); // Habilitar int.
  interrupts();
}




ISR(TIMER2_COMPA_vect)
// Interrupcion por timer 2 (10 ms)
{
  timertick++;
}




void ptoPinRising()
// Interrupcion para D3
{
  outputs_2++;
}




ISR(PCINT2_vect)
// Interrupcion pci para D0 a D7
{
  // Detectar cambio de estado y contar pulso correspondiente

  // CNTR0
  if(digitalRead(PIN_CNTR_0) == HIGH){ // Si el pin esta encendido
    if(!pin_cntr_state[0]){ // Si el estado anterior era "apagado"
      outputs_1[OUTPUT_CNTR_0]++; // Contar pulso
      pin_cntr_state[0] = true; // Cambiar estado
    }
  } else // Si el pin esta apagado
      if(pin_cntr_state[0]) // Si el estado era "encendido"
        pin_cntr_state[0] = false; // Cambiar estado

  // CNTR1
  if(digitalRead(PIN_CNTR_1) == HIGH){
    if(!pin_cntr_state[1]){
      outputs_1[OUTPUT_CNTR_1]++;
      pin_cntr_state[1] = true;
    }
  } else
      if(pin_cntr_state[1])
        pin_cntr_state[1] = false;

  // CNTR2
  if(digitalRead(PIN_CNTR_2) == HIGH){
    if(!pin_cntr_state[2]){
      outputs_1[OUTPUT_CNTR_2]++;
      pin_cntr_state[2] = true;
    }
  } else
      if(pin_cntr_state[2])
        pin_cntr_state[2] = false;

  // CNTR3
  if(digitalRead(PIN_CNTR_3) == HIGH){
    if(!pin_cntr_state[3]){
      outputs_1[OUTPUT_CNTR_3]++;
      pin_cntr_state[3] = true;
    }
  } else
      if(pin_cntr_state[3])
        pin_cntr_state[3] = false;
}




void startDataLogger()
// Iniciar el registro de datos
{
  outputs_1[OUTPUT_DT] = 0; // Reiniciar contador de datos
  outputs_1[OUTPUT_CNTR_0] = 0; // Reiniciar contador de pulsos canal 1
  outputs_1[OUTPUT_CNTR_1] = 0; // Reiniciar contador de pulsos canal 2
  outputs_1[OUTPUT_CNTR_2] = 0; // Reiniciar contador de pulsos canal 3
  outputs_1[OUTPUT_CNTR_3] = 0; // Reiniciar contador de pulsos canal 4
  outputs_2 = 0; // Reiniciar contador de pulsos PTO
  if(newLogfile()){ // Intentar crear nuevo registro
    // Comenzar registro de datos si se pudo crear el archivo correctamente
    digitalWrite(PIN_LED_LOGGING_OK, HIGH); // Encender led
    timertick = 0; // Reiniciar contador de temporizado
    dataLogging = true;
  } // Si no se pudo crear el archivo, la funcion newLogfile indicara el error
}




void stopDataLogger()
// Detener el registro de datos
{
  dataLogging = false;
  digitalWrite(PIN_LED_LOGGING_OK, LOW); // Apagar led
  timertick = 0; // Reiniciar contador
}




bool logReg(char *fileName)
// Guardar datos en el registro de nombre "logfileName"
// Los datos deben haber sido previamente actualizados
// Retornar "true" si la operacion fue exitosa
{
  // Intentar abrir archivo
  logfile = SD.open(fileName, FILE_WRITE);
  if (logfile){ // Si se pudo abrir el archivo
    for(int i = 0; i < 9; i++) // Guardar datos 0 a 8
      logfile.print(String(outputs_1[i])+" ");
    logfile.println(String(outputs_2)); // Fin de linea
    logfile.close(); // Cerrar archivo (es necesario para guardar datos)
  } else {// Si el archivo no se pudo abrir, mostrar error
    if(DEBUG_MODE) Serial.println("ERROR [logReg]: No se pudo abrir el archivo.");
    return false;
  }
  return true;
}




bool deleteFile(char *fileName)
// Borrar un archivo de la memoria SD
// Indicar si la operacion fue exitosa
{
  if(SD.exists(fileName)){ // Si el archivo existe, borrar
    SD.remove(fileName);
    return true;
  }
  else { // Si no existe, mostrar mensaje de error
    if(DEBUG_MODE) Serial.println("ERROR [deleteFile]: No existe el archivo.");
    return false;
  }
}





void listFiles()
// Listar los archivos presentes en la memoria
{
  File root = SD.open("/"); // Abrir directorio principal
  while(true) { // Recorrer archivos hasta que no haya mas
     File entry =  root.openNextFile(); // Abrir siguiente
     if (!entry){ // Si no hay mas archivos
       root.rewindDirectory();
       root.close();
       return; 
     }
     String fileName = String(entry.name()); // String con el nombre
     if(fileName.substring(0,4) == LOGFILE_NAME_PREFIX ) // Si es archivo de registro
       Serial.println(fileName + " " + entry.readStringUntil('\n') + " " + String(entry.size())); // Imprimir nombres y tamanio
     entry.close();
   }
}




bool newLogfile()
// Crea un nuevo archivo de registro y asigna el nuevo nombre a la vble. global corresp.
// Indicar si la operacion fue exitosa
{
  String newLogfileName; // Nombre a generar para el nuevo archivo
  char temp[LOGFILE_NAME_LENGTH]; // Nombre del archivo como arreglo de 8 caracteres
  byte L = 0; // Cantidad de digitos del indice del nombre de archivo
  // Generar nombres numerados hasta encontrar un nombre disponible
  for(int i = 1; i < MAX_NUM_LOGFILES; i++){
    newLogfileName = String(i); // String con el valor del indice
    L = newLogfileName.length(); // Cantidad de digitos
    // Agregar ceros hasta completar 4 digitos
    for(int j = 0; j < MAX_NUM_DIG - L; j++)
      newLogfileName = "0" + newLogfileName;
    newLogfileName = LOGFILE_NAME_PREFIX + newLogfileName; // Agregar prefijo
    newLogfileName.toCharArray(temp,LOGFILE_NAME_LENGTH); // Convertir a arreglo de caracteres
    if(!SD.exists(temp)) // Si el nombre generado esta disponible, terminar ciclo
      break;
  }
  for(int i = 0; i < LOGFILE_NAME_LENGTH; i++)
    logfileName[i] = temp[i]; // La variable global contiene el nombre del registro actual.
  // El encabezado del archivo contiene la estampa de tiempo que a la vez es el nombre
  // del archivo a exportar a la pc.
  logfile = SD.open(logfileName, FILE_WRITE); // Intentar abrir el archivo
  if (logfile){ // Si se pudo abrir, escribir estampa de tiempo
    DateTime now = rtc.now(); // Hora y fecha actuales
    logfile.print(String(now.year())+"_");
    logfile.print(String(now.month())+"_");
    logfile.print(String(now.day())+"_");
    logfile.print(String(now.hour())+"_");
    logfile.print(String(now.minute())+"_");
    logfile.println(String(now.second())+".txt");
    logfile.close(); // Cerrar archivo (es necesario para guardar datos)
  } else {// Si el archivo no se pudo abrir, mostrar error
    if(DEBUG_MODE) Serial.println("ERROR [newLogfile]: No se pudo abrir el archivo.");
    error = true; // Activar estado de error
    return false;
  }
  return true;
}




bool dumpFile(char *fileName)
// Mandar archivo por puerto serie
// Indicar si la operacion fue exitosa
{
  // Abrir el archivo requerido
  if(SD.exists(fileName)){
    logfile = SD.open(fileName);
    if(logfile){ // Si el archivo se pudo abrir, enviar
      while (logfile.available())
        Serial.write(logfile.read());
      logfile.close(); // Cerrar archivo
    } else {// Si el archivo no se pudo abrir, mostrar error
      if(DEBUG_MODE) Serial.println("ERROR [dumpFile]: No se pudo abrir el archivo.");
      return false;
    }
  } else {// Si el archivo no existe
    if(DEBUG_MODE) Serial.println("ERROR [dumpFile]: No existe el archivo.");
    return false;
  }
    
  return true;
}




void serialEvent()
// Interrupcion de puerto serie
{
  String arg;
  if(!dataLogging) // No leer si se esta grabando
    switch((char) Serial.read()){
      case 'a': // Solicitud de lista de archivos
        listFiles(); // Listar archivos presentes en la memoria
        Serial.println("%%EOL%%");
        break;
      case 'b': // Solicitud de archivo
      {
        arg = Serial.readStringUntil('\n'); // Leer argumento
        // Convertir argumento a char[]
        char fileName[LOGFILE_NAME_LENGTH];
        arg.toCharArray(fileName,LOGFILE_NAME_LENGTH);
        if(dumpFile(fileName)) // Enviar archivo si existe
          Serial.println("##EOF##");
        break;
      }
      case 'c': // Solicitud para borrar archivo
      {  
        arg = Serial.readStringUntil('\n'); // Leer argumento
        // Convertir argumento a char[]
        char fileName[LOGFILE_NAME_LENGTH];
        arg.toCharArray(fileName,LOGFILE_NAME_LENGTH);
        if(deleteFile(fileName)) // Intentar eliminar archivo si existe
          Serial.println("$$EOD$$");
        break;
      }
//      case 'd': // Solicitud de eliminar todos los archivos
//        arg = Serial.readStringUntil('\n');
//        if(arg == "LOG_*") // Para no disparar el formateo con un caracter
//          emptySDCard(); // Borrar todos los archivos de registro
//        break;
      case 'e': // Solicitud para ajustar hora
        arg = Serial.readStringUntil('\n'); // Leer argumento
        // Formato de arg debe ser: aaaammddhhmmss
        int date[6];
        date[0] = arg.substring(0,4).toInt();
        date[1] = arg.substring(4,6).toInt();
        date[2] = arg.substring(6,8).toInt();
        date[3] = arg.substring(8,10).toInt();
        date[4] = arg.substring(10,12).toInt();
        date[5] = arg.substring(12,14).toInt();
        rtc.adjust(DateTime(date[0], date[1], date[2], date[3], date[4], date[5]));
        break;
      case 'r': // Solicitud de valores de pines analogicos
        Serial.println(analogRead(PIN_AN_0));
        Serial.println(analogRead(PIN_AN_1));
        Serial.println(analogRead(PIN_AN_2));
        Serial.println(analogRead(PIN_AN_3));
        Serial.println("&&EOW&&");
        break;
      case 's': // Solicitud de valores de pines digitales
      	if(pin_cntr_state[0]) Serial.println('1: Encendido');
      	if(pin_cntr_state[1]) Serial.println('2: Encendido');
      	if(pin_cntr_state[2]) Serial.println('3: Encendido');
      	if(pin_cntr_state[3]) Serial.println('4: Encendido');
      	if(digitalRead(PIN_PTO) == HIGH) Serial.println('5(PTO): Encendido');
      	Serial.println("@@EOS@@");
      default:
        break;
    }
}




void loop()
// Lazo sin fin
{
  if(error){ // Si hubo un error, mantener parpadeando el led indicador hasta reiniciar micro
    if(timertick == 50){ // 500 ms
      digitalWrite(PIN_LED_ERROR, !digitalRead(PIN_LED_ERROR));
      timertick = 0;
    }
  } else {
    if(dataLogging){ // Si el registrador esta grabando
      switch(timertick){
        case 10: // 100 ms
          outputs_1[OUTPUT_A0] = analogRead(PIN_AN_0);
          break;
        case 20: // 200 ms
          outputs_1[OUTPUT_A1] = analogRead(PIN_AN_1);
          break;
        case 30: // 300 ms
          outputs_1[OUTPUT_A2] = analogRead(PIN_AN_2);
          break;
        case 40: // 400 ms
          outputs_1[OUTPUT_A3] = analogRead(PIN_AN_3);
          break;
        case 50: // 500 ms
          outputs_1[OUTPUT_DT]++; // Nuevo dato
          if(!logReg(logfileName)){ // Intentar grabar datos en el registro
            stopDataLogger(); // Si no se pudo, detener registrador
            error = true; // Activar estado de error
          }
          if(digitalRead(PIN_LOGGING_ON_OFF) == HIGH) // Si el pulsador se apaga, detener medicion
            stopDataLogger();
          timertick = 0; // Reiniciar contador de temporizado
          break;
        default: 
          break;
      }
    } else { // Si no se esta grabando
      if(timertick == 30){ // Cada 300 ms revisar el estado del pulsador
          if(digitalRead(PIN_LOGGING_ON_OFF) == LOW) // Si el pulsador esta encendido
            startDataLogger(); // Iniciar medicion
          else 
            timertick = 0; // Reiniciar contador de temporizado 
      }
    }
  }
}

