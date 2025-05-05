/*
* Registrador V2.0
* Autor: Matias Micheletto
* Inst: INTA-DIEC
* Fecha: Mayo-Junio 2023
*/

/**** Librerias ****/
#include <SD.h>
#include <Wire.h> // No se requiere con librerias instaladas con sus dependencias
#include "RTClib.h"
#include "LiquidCrystal_I2C.h"


/**** Directivas de precompilacion ****/
#define DEBUG_MODE false // Mensajes por serie

/// PINES ARDUINO UNO ///
// PIN 0 -> RX
// PIN 1 -> TX
#define PIN_LOGGING_ON_OFF 2 // Pin para encender/apagar registro de datos
#define PIN_PTO 3 // Contador de pulsos PTO (pci)
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
#define ANALOG_TOL 1010 // Tolerancia para considerar celda como conectada

// Indices del vector de salida (outputs_1)
#define OUTPUT_DT 0
#define OUTPUT_A0 1
#define OUTPUT_A1 2
#define OUTPUT_A2 3
#define OUTPUT_A3 4
#define OUTPUT_CNTR_0 5 
#define OUTPUT_CNTR_1 6
#define OUTPUT_CNTR_2 7
#define OUTPUT_CNTR_3 8

// Constantes LCD (En caso de usar 16x2, actualizar prints)
#define LCD_ADDR 0x27 // Puede ser 0x3F
#define LCD_COLS 20 // Columnas display LCD
#define LCD_ROWS 4 // Filas display LCD

// Constates archivos
#define MAX_NUM_LOGFILES 1000 // Maxima cantidad de registros
#define MAX_NUM_DIG 3 // Cantidad de digitos del indice del nombre de archivos
#define LOGFILE_NAME_PREFIX "LOG_" // Prefijo para los nombres de los registros
#define LOGFILE_NAME_LENGTH 8 // Cantidad de caracteres del nombre de los registros

// Constantes comunicacion
#define BAUDRATE 19200 // Velocidad de comunicacion serie
// Indicadores de fin de transmision (se usan desde el software para parsear datos serie)
#define LIST_ACK "%%EOL%%" // Listado de archivos
#define FILE_ACK "##EOF##" // Lectura de archivo
#define DEL_ACK "$$EOD$$" // Borrar registro
#define A_READ_ACK "&&EOW&&" // Lectura de entradas analogicas
#define D_READ_ACK "**EOQ**" // Lectura de entradas digitales


// Variables globales
File logfile; // Archivo de registro
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); // Inicializar display
char logfileName[8];// Nombre del archivo de registro
RTC_DS1307 rtc; // Objeto para controlar reloj de tiempo real
bool dataLogging = false; // Estado del registrador grabando/apagado
bool error = false; // Deteccion de errores
byte timertick = 0; // Contador de ticks para la planificacion de tareas
unsigned int outputs_1[9] = {0,0,0,0,0,0,0,0,0}; // Vector de valores a registrar
unsigned long outputs_2 = 0; // Contador de rpm PTO
bool pin_cntr_state[4] = {false, false, false, false}; // Estado de los pines contadores


void lcdPrint(const uint8_t col, const uint8_t row, const char text[], const bool clc = false){
  // Imprimir texto por display LCD
  if(clc) lcd.clear();
  lcd.setCursor(col % LCD_COLS, row % LCD_ROWS); // Div entera para no superar limites
  lcd.print(text);
}


void int2a(const unsigned int value, const char templ[], char result[]){
  // Formatear cadena de caracteres con valor numerico
  sprintf(result, templ, value); // Usar String toCharArray()?
}


void setup(){ // Inicializacion del datalogger
  // Inicializar librerias
  Serial.begin(BAUDRATE);
  Wire.begin();
  
  lcd.init(); // Inicializar #display
  lcd.backlight(); // Encender backlight #display
  
  rtc.begin(); // Inicializar #rtc
  
  // Inicializar SD
  lcdPrint(0,0,"INICIALIZANDO SD...", true);
  pinMode(10, OUTPUT);
  if (!SD.begin()) {
      if(DEBUG_MODE) Serial.println("ERROR [setup]: No se pudo inicializar SD");
      lcdPrint(0,1,"ERROR INICI. SD", false);
      error = true; // Entrar en modo de error
      return;
  }else{
    lcdPrint(0,1,"MODULO SD....OK", false);
  }

  // Configurar pines I/O
  lcdPrint(0,2,"INICIALIZANDO IO...", false);
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
  // Capturar estado inicial de los pines de contadores
  pin_cntr_state[0] = digitalRead(PIN_CNTR_0) == HIGH;
  pin_cntr_state[1] = digitalRead(PIN_CNTR_1) == HIGH;
  pin_cntr_state[2] = digitalRead(PIN_CNTR_2) == HIGH;
  pin_cntr_state[3] = digitalRead(PIN_CNTR_3) == HIGH;
  // Leds de estado del registrador inicialmente apagados
  digitalWrite(PIN_LED_LOGGING_OK, LOW);
  digitalWrite(PIN_LED_ERROR, LOW);
  lcdPrint(0,3,"PUERTOS IO...OK", false);

  // Configurar registro de interrupciones
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
  OCR2A = 156; // Registro de comparacion = 16MHz/1024/100.16Hz
  TCCR2A |= (1 << WGM21); // Modo CTC
  TCCR2B |= (1 << CS20); // 1024 prescaler
  TCCR2B |= (1 << CS21); // 1024 prescaler
  TCCR2B |= (1 << CS22); // 1024 prescaler
  TIMSK2 |= (1 << OCIE2A); // Habilitar int.
  interrupts();
}


ISR(TIMER2_COMPA_vect){ // Interrupcion de timer 2 (10 ms)
  timertick++;
}


void ptoPinRising(){ // Interrupcion para pin D3
  outputs_2++;
}


ISR(PCINT2_vect){ // Interrupcion pci para D0 a D7
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


void startDataLogger(){ // Iniciar el registro de datos
  outputs_1[OUTPUT_DT] = 0; // Reiniciar contador de datos
  outputs_1[OUTPUT_CNTR_0] = 0; // Reiniciar contador de pulsos canal 1
  outputs_1[OUTPUT_CNTR_1] = 0; // Reiniciar contador de pulsos canal 2
  outputs_1[OUTPUT_CNTR_2] = 0; // Reiniciar contador de pulsos canal 3
  outputs_1[OUTPUT_CNTR_3] = 0; // Reiniciar contador de pulsos canal 4
  outputs_2 = 0; // Reiniciar contador de pulsos PTO

  if(newLogfile()){ // Intentar crear nuevo registro
    // Comenzar registro de datos si se pudo crear el archivo correctamente
    digitalWrite(PIN_LED_LOGGING_OK, HIGH); // Encender led
    digitalWrite(PIN_LED_ERROR, HIGH); // El led adicional tambien
    lcdPrint(0, 0, "REGISTRANDO DATOS...", true);
    timertick = 0; // Reiniciar contador de temporizado
    dataLogging = true;
  } // Si no se pudo crear el archivo, la funcion newLogfile indicara el error
}


void stopDataLogger(){ // Detener el registro de datos
  dataLogging = false;
  digitalWrite(PIN_LED_LOGGING_OK, LOW); // Apagar led
  digitalWrite(PIN_LED_ERROR, LOW); // El led adicional tambien
  timertick = 0; // Reiniciar contador
}


bool logReg(char *fileName){ 
  // Guardar datos en el registro de nombre "logfileName"
  // Los datos deben haber sido previamente actualizados
  // Intentar abrir archivo
  logfile = SD.open(fileName, FILE_WRITE);
  if (logfile){ // Si se pudo abrir el archivo
    for(int i = 0; i < 9; i++) // Guardar datos 0 a 8
      logfile.print(String(outputs_1[i])+" ");
    logfile.println(String(outputs_2)); // Dato pto + fin de linea
    logfile.close(); // Cerrar archivo (es necesario para guardar datos)
  } else { // Si el archivo no se pudo abrir, mostrar error
    if(DEBUG_MODE) Serial.println("ERROR [logReg]: No se pudo abrir el archivo.");
    lcdPrint(0,0,"ERR. ABRIR REGISTRO", true);
    return false;
  }
  return true;
}


bool deleteFile(char *fileName){ // Borrar un archivo de la memoria SD
  if(SD.exists(fileName)){ // Si el archivo existe, borrar
    SD.remove(fileName);
    return true; // Indicar si la operacion fue exitosa
  } else { // Si no existe, mostrar mensaje de error
    if(DEBUG_MODE) Serial.println("ERROR [deleteFile]: No existe el archivo.");
    lcdPrint(0,0,"ERR. EL REGISTRO", true);
    lcdPrint(0,1,"NO EXISTE EN MEMORIA", false);
    return false;
  }
}


void listFiles(){ // Listar los archivos presentes en la memoria
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


bool newLogfile(){
  // Crea un nuevo archivo de registro y asigna el nuevo nombre a la vble. global corresp.
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
    lcdPrint(0, 0, "ERR. CREAR REGISTRO", true);
    error = true; // Activar estado de error
    return false;
  }
  return true; // Indicar si la operacion fue exitosa
}


bool dumpFile(char *fileName){ // Mandar archivo por puerto serie
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
  return true; // Operacion exitosa
}


void serialEvent(){ // Interrupcion de puerto serie
  if(!dataLogging){ // No leer si se esta grabando
    String arg;
    switch((char) Serial.read()){
      case 'a': // Solicitud de lista de archivos
        listFiles(); // Listar archivos presentes en la memoria
        Serial.println(LIST_ACK);
        break;
      case 'b': { // Solicitud de archivo
        arg = Serial.readStringUntil('\n'); // Leer argumento
        // Convertir argumento a char[]
        char fileName[LOGFILE_NAME_LENGTH];
        arg.toCharArray(fileName,LOGFILE_NAME_LENGTH);
        if(dumpFile(fileName)) // Enviar archivo si existe
          Serial.println(FILE_ACK);
        break;
      }
      case 'c':{ // Solicitud para borrar archivo
        arg = Serial.readStringUntil('\n'); // Leer argumento
        // Convertir argumento a char[]
        char fileName[LOGFILE_NAME_LENGTH];
        arg.toCharArray(fileName,LOGFILE_NAME_LENGTH);
        if(deleteFile(fileName)) // Intentar eliminar archivo si existe
          Serial.println(DEL_ACK);
        break;
      }
//    case 'd':{ // Solicitud de eliminar todos los archivos
//      arg = Serial.readStringUntil('\n');
//      if(arg == "LOG_*") // Para no disparar el formateo con un caracter
//        emptySDCard(); // Borrar todos los archivos de registro
//      break;
//    }
      case 'e':{ // Solicitud para ajustar hora
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
      }
      case 'r':{ // Solicitud de valores de pines analogicos
        Serial.println(analogRead(PIN_AN_0));
        Serial.println(analogRead(PIN_AN_1));
        Serial.println(analogRead(PIN_AN_2));
        Serial.println(analogRead(PIN_AN_3));
        Serial.println(A_READ_ACK);
        break;
      }
      case 's':{ // Solicitud de valores de contadores de pulso
        Serial.println(digitalRead(PIN_CNTR_0));
        Serial.println(digitalRead(PIN_CNTR_1));
        Serial.println(digitalRead(PIN_CNTR_2));
        Serial.println(digitalRead(PIN_CNTR_3));
        Serial.println(digitalRead(PIN_PTO));
        Serial.println(D_READ_ACK);
      }
      case 'z':{ // Solicitud de reinicio de la placa
        asm("jmp 0x0000"); 
        break;
      }
      default:
        break;
    }
  }
}


void updateDisplay1(){ // En grabacion, mostrar contador de datos
  char str[LCD_COLS];
  lcdPrint(0, 0, "GRABANDO...", true);
  int2a(outputs_1[OUTPUT_DT], "MEDICIONES: %d", str);
  lcdPrint(0, 1, str, false);
}


void updateDisplay2(){ // Ocioso, mostrar lectura de entradas
  char str[LCD_COLS];
  uint8_t row = 0; // Contador de lineas

  lcd.clear();
  
  if(outputs_1[OUTPUT_A0] < ANALOG_TOL){
    int2a(outputs_1[OUTPUT_A0], "CANAL 1: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }
  
  if(outputs_1[OUTPUT_A1] < ANALOG_TOL){
    int2a(outputs_1[OUTPUT_A1],"CANAL 2: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }
  
  if(outputs_1[OUTPUT_A2] < ANALOG_TOL){
    int2a(outputs_1[OUTPUT_A2],"CANAL 3: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }
  
  if(outputs_1[OUTPUT_A3] < ANALOG_TOL){
    int2a(outputs_1[OUTPUT_A3],"CANAL 4: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }

  if(outputs_1[OUTPUT_CNTR_0] > 0){
    int2a(outputs_1[OUTPUT_CNTR_0],"CONT. 1: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }

  if(outputs_1[OUTPUT_CNTR_1] > 0){
    int2a(outputs_1[OUTPUT_CNTR_1],"CONT. 2: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }

  if(outputs_1[OUTPUT_CNTR_2] > 0){
    int2a(outputs_1[OUTPUT_CNTR_2],"CONT. 3: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }

  if(outputs_1[OUTPUT_CNTR_3] > 0){
    int2a(outputs_1[OUTPUT_CNTR_3],"CONT. 4: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }

  if(outputs_2 > 0){
    int2a((unsigned int) outputs_2,"CONT. PTO: %d", str);
    lcdPrint(0, row, str, false);
    row++;
  }

  if(row == 0){ // Si no hay lectura, mostrar fecha y hora
    lcdPrint(0, 0, "REGISTRADOR OCIOSO", true);
    lcdPrint(0, 1, "CONECTE ENTRADAS", false);

    // Mostrar hora y fecha actuales
    DateTime now = rtc.now(); 
    String currentDate = "FECHA: " + String(now.day()) + "/" + String(now.month()) + "/" + String(now.year());
    String currentTime = "HORA: " + String(now.hour()) + ":" + String(now.minute());
    currentDate.toCharArray(str, currentDate.length());
    lcdPrint(0, 2, str, false);
    currentTime.toCharArray(str, currentTime.length());
    lcdPrint(0, 3, str, false);
  }
}


void loop(){
  if(error){ // Si hubo un error, mantener parpadeando el led indicador hasta reiniciar micro
    if(timertick == 50){ // 500 ms
      digitalWrite(PIN_LED_ERROR, !digitalRead(PIN_LED_ERROR));
      timertick = 0;
    }
  }else{
    switch(timertick){ // Tareas
      case 10: // 100 ms
        outputs_1[OUTPUT_A0] = analogRead(PIN_AN_0);
        break;
      case 20: // 200 ms
        outputs_1[OUTPUT_A1] = analogRead(PIN_AN_1);
        break;
      case 30: // 300 ms
        outputs_1[OUTPUT_A2] = analogRead(PIN_AN_2);
        if(!dataLogging && digitalRead(PIN_LOGGING_ON_OFF) == LOW) // Pulsador encendido
          startDataLogger(); // Iniciar registro de datos
        break;
      case 40: // 400 ms
        outputs_1[OUTPUT_A3] = analogRead(PIN_AN_3);
        break;
      case 50: // 500 ms
        if(dataLogging){ // Si esta grabando
          outputs_1[OUTPUT_DT]++; // Nuevo dato
          if(!logReg(logfileName)) { // Intentar grabar datos en el registro
            stopDataLogger(); // Si no se pudo, detener registrador
            lcdPrint(0, 0, "ERR. REGISTRO DATO", true);
            error = true; // Activar estado de error
          }else{ // Si no hubo error durante la grabacion de datos
            if(digitalRead(PIN_LOGGING_ON_OFF) == HIGH) // Si el pulsador se apaga
              stopDataLogger(); // Detener medicion
            updateDisplay1(); // Mostrar mensaje de grabacion
          }
        }else{
          updateDisplay2(); // Mostrar lectura de entradas
        }
        timertick = 0; // Reiniciar contador de temporizado
        break;
      default: 
        break;
    }
  }
}
