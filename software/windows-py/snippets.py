# list serial ports
import serial.tools.list_ports

ports = serial.tools.list_ports.comports()
for port in ports:
    print(f"{port.device} - {port.description}")


# find arduino port
arduino_port = None
for port in ports:
    descs = port.description.lower()
    if "arduino" in descs or "ch340" in descs:
        arduino_port = port.device
        break
if arduino_port is None:
    print("Arduino not found")
else:
    print(f"Arduino found on port {arduino_port}")


# connect to arduino
import serial
arduino = serial.Serial(arduino_port, 19600, timeout=1)


# read data from arduino
def read_arduino_data(arduino):
    data = arduino.readline().decode("utf-8").strip()
    if data:
        print(f"Received: {data}")
    else:
        print("No data received")

# write data to arduino
data = "Hello Arduino"
def write_arduino_data(arduino, data):
    arduino.write(data.encode("utf-8"))
    arduino.flush()
    print(f"Sent: {data}")
    response = read_arduino_data(arduino)
    print(f"Received: {response}")


# close arduino connection
def close_arduino(arduino):
    arduino.close()
    print("Arduino connection closed")



# show a select dialog to choose a serial port
serial_ports = ['COM1', 'COM2', 'COM3']  # This should be dynamically generated
default_serial_port = 'COM2'
ttk.Label(root, text="Hello World").grid(column=0, row=0, padx=10, pady=10)
port_combobox = ttk.Combobox(root, values=serial_ports)
port_combobox.set(default_serial_port)  # Set default value
port_combobox.grid(row=0, column=1, padx=10, pady=10)
connect_button = ttk.Button(root, text="Connect", command=lambda: print(f"Connecting to {port_combobox.get()}"))
connect_button.grid(row=1, column=0, columnspan=2, pady=10)