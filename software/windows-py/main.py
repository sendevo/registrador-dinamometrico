import tkinter as tk
from tkinter import ttk
import constants
import commands
from commands import SerialPortManager
import threading


serial_port_manager = SerialPortManager()


def find_arduino_port():
    connect_button.config(state="disabled")
    default_port = serial_port_manager.find_arduino_port()
    if default_port:
        port_combobox.set(default_port)
        status_label.config(text="Puerto encontrado, listo para conectar.")
    else:
        port_combobox.set("Seleccione un puerto")
    progress.grid_remove()
    status_label.config(text="El puerto del registrador parece no estar disponible.")
    connect_button.config(state="normal")


def fill_logs_table_end():
    progress.grid_remove()
    update_log_list_button.config(state="normal")


def stop_filling_logs_table():
    serial_port_manager.set_abort_flag(True)
    progress.grid_remove()
    update_log_list_button.config(state="normal")


def update_log_table():
    serial_port_manager.set_abort_flag(False)
    tree.delete(*tree.get_children())
    logs = serial_port_manager.get_logs_list()
    if not logs:
        return
    for log in logs:
        tree.insert("", "end", values=log)
    root.after(0, fill_logs_table_end)
    serial_port_manager.set_abort_flag(False)
    update_log_list_button.config(state="normal")


def on_update_log_table():
    progress.grid()
    threading.Thread(target=update_log_table).start()


def on_connect():
    port = port_combobox.get()
    if port:
        progress.grid()
        status_label.config(text="Conectando...")
        if serial_port_manager.connect(port):
            connect_button.config(state="disabled")
            disconnect_button.config(state="normal")        
            update_log_list_button.config(state="normal")    
            progress.grid_remove()
            status_label.config(text="Conectado al puerto serie, actualice lista de registros.")
        else:
            progress.grid_remove()
            status_label.config(text="Error al conectar al puerto serie.")
    else:
        status_label.config(text="Seleccione un puerto serie.")
    


def on_disconnect():
    if serial_port_manager.disconnect():
        connect_button.config(state="normal")
        disconnect_button.config(state="disabled")
        update_log_list_button.config(state="disabled")
        progress.grid_remove()
        status_label.config(text="Puerto serie desconectado.")
        tree.delete(*tree.get_children())
    else:
        print("Error al desconectar")


def app_setup(app, config):
    # Root app setup
    global root
    root = app

    root.title(config["title"])
    root.withdraw()
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    width_prop = config["dimensions"][0]
    height_prop = config["dimensions"][1]
    window_width = int(screen_width * width_prop)
    window_height = int(screen_height * height_prop)
    position_x = int((screen_width - window_width) / 2)
    position_y = int((screen_height - window_height) / 2)
    root.geometry(f"{window_width}x{window_height}+{position_x}+{position_y}")
    root.iconphoto(True, tk.PhotoImage(file = config["icon"]))
    root.deiconify()
    
    # Configure grid
    top_frame = ttk.Frame(root)
    top_frame.grid(row=0, column=0, sticky="ew", padx=10, pady=5)
    middle_frame = ttk.Frame(root)
    middle_frame.grid(row=1, column=0, sticky="ew", padx=10, pady=5)
    bottom_frame = ttk.Frame(root)
    bottom_frame.grid(row=2, column=0, sticky="ew", padx=10, pady=5)

    # User feedback
    global status_label
    status_label = ttk.Label(bottom_frame, text="Buscando puertos disponibles...")
    status_label.grid(row=0, column=0, padx=5)
    global progress
    progress = ttk.Progressbar(bottom_frame, orient="horizontal", mode="indeterminate")
    progress.grid(row=1, column=0, columnspan=6, sticky="ew", padx=10, pady=10)

    # Serial port selection combobox and connection controls
    label = ttk.Label(top_frame, text="Puertos disponibles")
    label.grid(row=0, column=0, padx=5)
    
    global connect_button
    connect_button = ttk.Button(top_frame, text="Conectar", command=on_connect)
    connect_button.grid(row=0, column=2, padx=5)

    global disconnect_button
    disconnect_button = ttk.Button(top_frame, text="Desconectar", command=on_disconnect)
    disconnect_button.grid(row=0, column=3, padx=5)
    disconnect_button.config(state="disabled")

    # List available serial ports
    serial_ports = serial_port_manager.list_serial_ports()
    global port_combobox
    port_combobox = ttk.Combobox(top_frame, values=serial_ports)
    port_combobox.grid(row=0, column=1, padx=10, pady=10)
    progress.start()
    threading.Thread(target=find_arduino_port).start()

    # Query logs from datalogger
    global update_log_list_button
    update_log_list_button = ttk.Button(middle_frame, text="Actualizar lista", command=on_update_log_table)
    update_log_list_button.grid(row=0, column=0, padx=5, pady=10)
    update_log_list_button.config(state="disabled")

    columns = ("#1", "#2", "#3")
    global tree
    tree = ttk.Treeview(middle_frame, columns=columns, show="headings", selectmode="extended")
    tree.heading("#1", text="Registro")
    tree.heading("#2", text="Nombre de archivo")
    tree.heading("#3", text="Tamaño (bytes)")
    tree.grid(row=1, column=0, sticky="nsew")
    scrollbar = ttk.Scrollbar(middle_frame, orient="vertical", command=tree.yview)
    scrollbar.grid(row=1, column=2, sticky="ns")
    tree.configure(yscroll=scrollbar.set)

    # Set the size of the window to be proportional to the screen size
    root.update()
    root.geometry("")

if __name__ == "__main__":     
    app = tk.Tk()
    app_config = {
        "title": constants.app_name,
        "dimensions": constants.window_proportions,
        "icon": constants.icon_path,
    }
    app_setup(app, app_config)
    app.mainloop()