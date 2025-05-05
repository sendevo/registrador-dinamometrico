import tkinter as tk
from tkinter import ttk
import constants
import commands
from commands import SerialPortManager

serial_port_manager = SerialPortManager()


def fill_logs_table(tree):
    logs = serial_port_manager.get_logs_list()
    for log in logs:
        tree.insert("", "end", values=log)


def on_connect_attempt(port, tree):
    if serial_port_manager.connect(port):
        print(f"Conectado a {port}, esperando lista de registros...")
        fill_logs_table(tree)
    else:
        print(f"Error al conectar a {port}")


def app_setup(root, config):
    # Root app setup
    root.title(config.title)
    root.withdraw()
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    width_prop = config.dimensions[0]
    height_prop = config.dimensions[1]
    window_width = int(screen_width * width_prop)
    window_height = int(screen_height * height_prop)
    position_x = int((screen_width - window_width) / 2)
    position_y = int((screen_height - window_height) / 2)
    root.geometry(f"{window_width}x{window_height}+{position_x}+{position_y}")
    root.iconphoto(True, tk.PhotoImage(file = config.icon))
    root.deiconify()
    
    # Serial port selection
    label = ttk.Label(root, text="Puertos disponibles")
    label.grid(column=0, row=0, padx=10, pady=10)
    
    serial_ports = serial_port_manager.list_serial_ports()
    port_combobox = ttk.Combobox(root, values=serial_ports)
    port_combobox.grid(row=0, column=1, padx=10, pady=10)
    port_combobox.set(serial_port_manager.find_arduino_port())

    # Logs list
    table_frame = ttk.Frame(root)
    table_frame.grid(row=1, column=0, columnspan=3, padx=10, pady=10)
    table_frame.grid_rowconfigure(0, weight=1)
    table_frame.grid_columnconfigure(0, weight=1)
    columns = ("#1", "#2", "#3")
    tree = ttk.Treeview(table_frame, columns=columns, show="headings", selectmode="extended")
    tree.heading("#1", text="Registro")
    tree.heading("#2", text="Nombre de archivo")
    tree.heading("#3", text="Tamaño (bytes)")
    tree.grid(row=0, column=0, sticky="nsew")
    scrollbar = ttk.Scrollbar(table_frame, orient="vertical", command=tree.yview)
    scrollbar.grid(row=0, column=1, sticky="ns")
    tree.configure(yscroll=scrollbar.set)

    connect_button = ttk.Button(root, text="Conectar", command=lambda: on_connect_attempt(port_combobox.get(), tree))
    connect_button.grid(row=0, column=2, columnspan=2, pady=10)


app = tk.Tk()
app_config = {
    "title": constants.app_name,
    "dimensions": constants.window_proportions,
    "icon": constants.icon_path,
}
app_setup(app, app_config)
app.mainloop()