import serial
import serial.tools.list_ports

class SerialPortManager:

    def __init__(self):
        self.serial_port = None
        self.baudrate = 19200
        self.timeout = 1
        self.serial_port = None
        
    def find_arduino_port(self):
        ports = serial.tools.list_ports.comports()
        for port in ports:
            desc = port.description.lower()
            if "arduino" in desc or "ch340" in desc:
                return port.device  
        print("Arduino no encontrado, usando el primer puerto disponible")
        return ports[0].device if ports else None
            
    def list_serial_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def connect(self, port):
        try:
            self.serial_port = serial.Serial(port, self.baudrate, timeout=self.timeout)
            if self.serial_port.is_open:
                print(f"Conectado a {port} a {self.baudrate} bps")
            else:
                print(f"Puerto {port} cerrado")
                return False
            return True
        except serial.SerialException as e:
            print(f"Error connecting to {port}: {e}")
            return False

    def disconnect(self):
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
            self.serial_port = None
            print("Desconectado del puerto serie")
        else:
            print("No hay conexión activa")
        return False

    """ Comandos de control
        Comando             | Funcion                                                 | Ejemplo             | Confirmacion
        --------------------|---------------------------------------------------------|---------------------|----------------
        "a\n"               | Solicitud de lista de registros                         | "a\n"               | "%%EOL%%"
        "bLOG_xxx\n"        | Solicitud de descarga del registro número xxx.          | "bLOG_008\n"        | "##EOF##"
        "cLOG_xxx\n"        | Solicitud de eliminación del registro número xxx        | "cLOG_034\n"        | "$$EOD$$"
        "exxxxxxxxxxxxxx\n" | Actualizar la fecha y hora según formato aaaammddhhmmss | "e20150914103826\n" | (Sin confirmacion)
        "r\n"               | Solicitud de lectura de celdas de carga                 | "r\n"               | "&&EOW&&"
    """

    def get_logs_list(self):
        print("Solicitando lista de registros...")

    def download_log(self, log_number):
        print(f"Solicitando descarga del registro número {log_number}...")

    def delete_log(self, log_number):
        print(f"Solicitando eliminación del registro número {log_number}...")
    
    def update_datetime(self, datetime_str):
        print(f"Actualizando fecha y hora a {datetime_str}...")
        
    def read_load_cells(self):
        print("Solicitando lectura de celdas de carga...")
