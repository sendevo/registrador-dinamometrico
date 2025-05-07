/*
* Emulador de registrador 
* Autor: Matias Micheletto
* Inst: INTA-DIEC
* Fecha: Mayo 2025
*/

/**** Librerias ****/
#include <Wire.h> // No se requiere con librerias instaladas con sus dependencias
#include "LiquidCrystal_I2C.h"


// Constantes LCD
#define LCD_ADDR 0x27 // Puede ser 0x3F
#define LCD_COLS 20 // Columnas display LCD
#define LCD_ROWS 4 // Filas display LCD

// Constates archivos
#define LOGFILE_NAME_PREFIX "LOG_" // Prefijo para los nombres de los registros

// Constantes comunicacion
#define BAUDRATE 19200 // Velocidad de comunicacion serie
// Indicadores de fin de transmision (se usan desde el software para parsear datos serie)
#define LIST_ACK "%%EOL%%" // Listado de archivos
#define FILE_ACK "##EOF##" // Lectura de archivo
#define DEL_ACK "$$EOD$$" // Borrar registro
#define A_READ_ACK "&&EOW&&" // Lectura de entradas analogicas
#define D_READ_ACK "**EOQ**" // Lectura de entradas digitales


LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS); // Inicializar display


void lcdPrint(const uint8_t col, const uint8_t row, const char text[], const bool clc = false){
    // Imprimir texto por display LCD
    if(clc) lcd.clear();
    lcd.setCursor(col % LCD_COLS, row % LCD_ROWS); // Div entera para no superar limites
    lcd.print(text);
}

void setup(){ // Inicializacion del datalogger
    randomSeed(analogRead(0)); 

    // Inicializar librerias
    Serial.begin(BAUDRATE);
    Wire.begin();

    lcd.init(); // Inicializar #display
    lcd.backlight(); // Encender backlight #display
    lcdPrint(0,3,"INICIALIZADO OK", false);
}

void printTwoDigits(int number) {
    if (number < 10) Serial.print("0");
    Serial.print(number);
}

void printRandomFileName() {
    int year = random(2000, 2030);
    int month = random(1, 13);
    int day = random(1, 29);
    int hour = random(0, 24);
    int minute = random(0, 60);
    int second = random(0, 60);

    // Print formatted string
    Serial.print(year);
    Serial.print("_");
    printTwoDigits(month);
    Serial.print("_");
    printTwoDigits(day);
    Serial.print("_");
    printTwoDigits(hour);
    Serial.print("_");
    printTwoDigits(minute);
    Serial.print("_");
    printTwoDigits(second);
    Serial.print(".txt");
}

void dumpFile() {
    printRandomFileName();
    Serial.println("");
    const int numLines = random(10,20); // Cantidad de lineas a enviar
    for(int i = 0; i < numLines; i++){
        Serial.print(i);
        for(int j = 0; j < 4; j++){
            Serial.print(" ");
            Serial.print(random(0, 1023));
        }
        for(int j = 0; j < 5; j++){
            Serial.print(" ");
            Serial.print(random(0, 100));
        }
        Serial.println();
    }
}

void serialEvent(){ // Interrupcion de puerto serie
    if(Serial.available()){
        String input = Serial.readStringUntil('\n');
        if (input.length() == 0) return;
        char command = input.charAt(0);
        String arg = input.substring(1);  // The rest of the command, if any
        /*
        Serial.print("Received: ");
        Serial.print(command);
        Serial.print(" - Command: ");
        Serial.println(arg);
        */
        switch(command){
            case 'a': {// Solicitud de lista de archivos
                int filesCnt = random(1, 100);
                lcdPrint(0, 0, "ARCHIVOS EN SD", true);
                lcdPrint(0, 1, String(filesCnt).c_str(), false);
                for(int i = 0; i < filesCnt; i++){
                    Serial.print(LOGFILE_NAME_PREFIX);
                    Serial.print(i);
                    Serial.print(" ");
                    printRandomFileName();
                    Serial.print(" ");
                    Serial.print(random(0, 100));
                    Serial.println();
                }
                Serial.println(LIST_ACK);
                return;
            }
            case 'b': { // Solicitud de archivo
                //arg = Serial.readStringUntil('\n'); // Leer argumento
                lcdPrint(0, 0, "LEYENDO ARCHIVO", true);
                lcdPrint(0, 1, arg.c_str(), false);
                dumpFile();
                Serial.println(FILE_ACK);
                return;
            }
            case 'c':{ // Solicitud para borrar archivo
                //arg = Serial.readStringUntil('\n'); // Leer argumento
                // Imprimir por lcd
                lcdPrint(0, 0, "BORRANDO ARCHIVO", true);
                lcdPrint(0, 1, arg.c_str(), false);
                Serial.println(DEL_ACK);
                return;
            }
            case 'e':{ // Solicitud para ajustar hora
                //arg = Serial.readStringUntil('\n'); // Leer argumento
                // Formato de arg debe ser: aaaammddhhmmss

                lcdPrint(0, 0, arg.c_str(), true);
                
                int date[6];
                date[0] = arg.substring(0,4).toInt();   // year
                date[1] = arg.substring(4,6).toInt();   // month
                date[2] = arg.substring(6,8).toInt();   // day
                date[3] = arg.substring(8,10).toInt();  // hour
                date[4] = arg.substring(10,12).toInt(); // minute
                date[5] = arg.substring(12,14).toInt(); // second
                
                char currentDate[20];
                char currentTime[20];
                sprintf(currentDate, "FECHA: %02d/%02d/%04d", date[2], date[1], date[0]);
                sprintf(currentTime, "HORA: %02d:%02d", date[3], date[4]);
                
                lcdPrint(0, 2, currentDate, false);
                lcdPrint(0, 3, currentTime, false);
                return;
            }
            case 'r':{ // Solicitud de valores de pines analogicos
                Serial.println(random(0, 1023));
                Serial.println(random(0, 1023));
                Serial.println(random(0, 1023));
                Serial.println(random(0, 1023));
                Serial.println(A_READ_ACK);
                return;
            }
            case 's':{ // Solicitud de valores de contadores de pulso
                Serial.println(random(1, 1000));
                Serial.println(random(1, 1000));
                Serial.println(random(1, 1000));
                Serial.println(random(1, 1000));
                Serial.println(random(1, 1000));
                Serial.println(D_READ_ACK);
                return;
            }
            case 'z':{ // Solicitud de reinicio de la placa
                asm("jmp 0x0000"); 
                return;
            }
            default:
                return;
        }
    }
}

void loop(){}