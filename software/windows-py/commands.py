import serial
import serial.tools.list_ports
import time
import constants


""" Comandos de control del registrador:

    Comando             | Funcion                                                 | Ejemplo             | Confirmacion
    --------------------|---------------------------------------------------------|---------------------|----------------
    "a\n"               | Solicitud de lista de registros                         | "a\n"               | "%%EOL%%"
    "bLOG_xxx\n"        | Solicitud de descarga del registro número xxx.          | "bLOG_008\n"        | "##EOF##"
    "cLOG_xxx\n"        | Solicitud de eliminación del registro número xxx        | "cLOG_034\n"        | "$$EOD$$"
    "exxxxxxxxxxxxxx\n" | Actualizar la fecha y hora según formato aaaammddhhmmss | "e20150914103826\n" | (Sin confirmacion)
    "r\n"               | Solicitud de lectura de celdas de carga                 | "r\n"               | "&&EOW&&"
"""


class SerialPortManager:

    def __init__(self):
        self.serial_port = None
        self.baudrate = constants.serial_baudrate
        self.timeout = 1
        self.abort_flag = False
    

    def debug_logger(self, message, type="info"):
        if type == "info":
            print(f"INFO: {message}")
        elif type == "error":
            print(f"ERROR: {message}")
        elif type == "debug":
            print(f"DEBUG: {message}")
        else:
            print(f"UNKNOWN TYPE: {type} - {message}")


    def set_abort_flag(self, flag):
        self.abort_flag = flag
        if flag:
            self.debug_logger("Abortando operación actual...")


    def find_arduino_port(self):
        ports = serial.tools.list_ports.comports()
        for port in ports:
            desc = port.description.lower()
            vid = f"{port.vid:04X}" if port.vid else None
            pid = f"{port.pid:04X}" if port.pid else None
            self.debug_logger(f"Port: {port.device}, VID: {vid}, PID: {pid}")
            if "arduino" in desc or "ch340" in desc:
                return port.device  
        self.debug_logger("Arduino no encontrado, buscando por ping...")
        for port in ports:
            try:
                with serial.Serial(port.device, baudrate=self.baudrate, timeout=self.timeout) as ser:
                    time.sleep(2)
                    ser.reset_input_buffer()
                    ser.write(b"a\n")
                    time.sleep(0.1)
                    response = ser.readline().decode(errors="ignore").strip()
                    self.debug_logger(f"Trying {port.device}: '{response}'")
                    if response:
                        return port.device
                    else:
                        self.debug_logger(f"Sin respuesta de {port.device}, intentando otro puerto")
            except (serial.SerialException, OSError) as e:
                continue
        return None


    def list_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]


    def connect(self, port):
        try:
            self.serial_port = serial.Serial(port, self.baudrate, timeout=self.timeout)
            if self.serial_port.is_open:
                self.debug_logger(f"Conectado a {port} a {self.baudrate} bps", "info")
                return True
            else:
                self.debug_logger(f"Puerto {port} cerrado", "info")
                return False
        except serial.SerialException as e:
            self.debug_logger(f"No se pudo conectar al puerto {port}: {e}", "error")
            return False


    def disconnect(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.serial_port = None
            self.debug_logger("Desconectado del puerto serie", "info")
            return True
        else:
            self.debug_logger("No hay conexión activa")
        return False


    def get_logs_list(self):
        self.debug_logger("Solicitando lista de registros...")
        time.sleep(2)
        logs = []
        if self.serial_port and self.serial_port.is_open:
            try:
                self.serial_port.write(b"a\n")
                max_attempts = 5
                attempts = 0
                while True and not self.abort_flag:
                    log = self.serial_port.readline().decode('utf-8').strip()
                    if log == "":
                        self.debug_logger("Respuesta vacía, esperando...")
                        attempts += 1
                        if attempts >= max_attempts:
                            self.debug_logger("Tiempo de espera agotado al recibir la lista de registros", "info")
                            break
                    else:
                        self.debug_logger(f"Registro recibido: {log}")
                        if log == "%%EOL%%":
                            break
                        else:
                            logs.append(log)
                        attempts = 0
                self.debug_logger("Descarga de listado completada")
            except serial.SerialTimeoutException:
                self.debug_logger("Tiempo de espera de lista de registros agotado", "error")
            except serial.SerialException as e:
                self.debug_logger(f"Error al recibir la lista de registros: {e}", "error")
        else:
            self.debug_logger("No hay conexión activa")
        return logs


    def download_log(self, log_number):
        if not self.serial_port or not self.serial_port.is_open:
            self.debug_logger("No hay conexión activa")
            return []
        cmd = f"bLOG_{log_number:03d}\n"
        self.debug_logger(f"Enviando: {cmd.strip()}")
        self.serial_port.write(cmd.encode())
        log_data = []
        while True:
            line = self.serial_port.readline().decode("utf-8").strip()
            if line == "##EOF##":
                break
            log_data.append(line)
            self.debug_logger(f"Recibido: {line}")
        
        self.debug_logger(f"Registro {log_number} descargado")
        return log_data


    def delete_log(self, log_number):
        if not self.serial_port or not self.serial_port.is_open:
            self.debug_logger("No hay conexión activa")
            return False
        cmd = f"cLOG_{log_number:03d}\n"
        self.debug_logger(f"Enviando: {cmd.strip()}")
        self.serial_port.write(cmd.encode())
        response = self.serial_port.readline().decode("utf-8").strip()
        if response == "$$EOD$$":
            self.debug_logger(f"Registro {log_number} eliminado correctamente", "info")
            return True
        else:
            self.debug_logger(f"Respuesta inesperada: {response}", "error")
            return False
    

    def update_datetime(self, datetime_str):
        if not self.serial_port or not self.serial_port.is_open:
            self.debug_logger("No hay conexión activa")
            return
        if len(datetime_str) != 14:
            self.debug_logger("Formato inválido. Use: AAAAMMDDHHMMSS", "error")
            return
        cmd = f"e{datetime_str}\n"
        self.debug_logger(f"Actualizando reloj: {cmd.strip()}", "info")
        self.serial_port.write(cmd.encode())
        

    def read_load_cells(self):
        if not self.serial_port or not self.serial_port.is_open:
            self.debug_logger("No hay conexión activa")
            return []

        self.debug_logger("Solicitando lectura de celdas de carga...", "info")
        self.serial_port.write(b"r\n")
        readings = []

        while True:
            line = self.serial_port.readline().decode("utf-8").strip()
            if line == "&&EOW&&":
                break
            readings.append(int(line))
            self.debug_logger(f"Celda: {line}")
        
        self.debug_logger("Lectura finalizada")
        return readings
