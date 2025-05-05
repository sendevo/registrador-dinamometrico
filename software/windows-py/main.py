import tkinter as tk
from tkinter import ttk
import constants
import commands

def main_window_setup(root, cts):
    root.title(cts.app_name)
    root.withdraw()
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    width_prop = cts.window_proportions[0]
    height_prop = cts.window_proportions[1]
    window_width = int(screen_width * width_prop)
    window_height = int(screen_height * height_prop)
    position_x = int((screen_width - window_width) / 2)
    position_y = int((screen_height - window_height) / 2)
    root.geometry(f"{window_width}x{window_height}+{position_x}+{position_y}")
    root.iconphoto(True, tk.PhotoImage(file = cts.icon_path))
    root.deiconify()  # Show window

def serial_port_selector(root, serial_ports, default_serial_port, callback):
    ttk.Label(root, text="Puertos disponibles").grid(column=0, row=0, padx=10, pady=10)
    port_combobox = ttk.Combobox(root, values=serial_ports)
    port_combobox.set(default_serial_port)  # Set default value
    port_combobox.grid(row=0, column=1, padx=10, pady=10)
    connect_button = ttk.Button(root, text="Conectar", command=lambda: callback(port_combobox.get()))
    connect_button.grid(row=1, column=0, columnspan=2, pady=10)

serial_port_manager = commands.SerialPortManager()

window = tk.Tk()
main_window_setup(window, constants)
serial_ports = serial_port_manager.list_serial_ports()
default_serial_port = serial_port_manager.find_arduino_port()
serial_port_selector(window, serial_ports, default_serial_port, serial_port_manager.connect)

window.mainloop()