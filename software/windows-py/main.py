import tkinter as tk
from tkinter import ttk
import constants
import commands
from commands import SerialPortManager
import threading

serial_port_manager = SerialPortManager()


def fill_logs_table_end():
    progress.stop()
    progress.grid_remove()
    cancel_update_list_button.config(state="disabled")
    update_log_list_button.config(state="normal")


def stop_filling_logs_table():
    serial_port_manager.set_abort_flag(True)
    progress.stop()
    progress.grid_remove()
    cancel_update_list_button.config(state="disabled")
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
    cancel_update_list_button.config(state="disabled")


def on_update_log_table():
    progress.grid()
    progress.start()
    threading.Thread(target=update_log_table).start()


def on_connect():
    port = port_combobox.get()
    if port:
        if serial_port_manager.connect(port):
            connect_button.config(state="disabled")
            disconnect_button.config(state="normal")        
            update_log_list_button.config(state="normal")    


def on_disconnect():
    if serial_port_manager.disconnect():
        connect_button.config(state="normal")
        disconnect_button.config(state="disabled")
        update_log_list_button.config(state="disabled")
        cancel_update_list_button.config(state="disabled")
        progress.stop()
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

    # Serial port selection
    label = ttk.Label(top_frame, text="Puertos disponibles")
    label.grid(row=0, column=0, padx=5)
    
    serial_ports = serial_port_manager.list_serial_ports()
    global port_combobox
    port_combobox = ttk.Combobox(top_frame, values=serial_ports)
    port_combobox.grid(row=0, column=1, padx=10, pady=10)
    default_port = serial_port_manager.find_arduino_port()
    if default_port:
        port_combobox.set(default_port)
    else:
        port_combobox.set("Seleccione un puerto")

    # Buttons
    global connect_button
    connect_button = ttk.Button(top_frame, text="Conectar", command=on_connect)
    connect_button.grid(row=0, column=2, padx=5)

    global disconnect_button
    disconnect_button = ttk.Button(top_frame, text="Desconectar", command=on_disconnect)
    disconnect_button.grid(row=0, column=3, padx=5)
    disconnect_button.config(state="disabled")

    global update_log_list_button
    update_log_list_button = ttk.Button(middle_frame, text="Actualizar lista", command=on_update_log_table)
    update_log_list_button.grid(row=0, column=1, padx=5)
    update_log_list_button.config(state="disabled")

    global cancel_update_list_button
    cancel_update_list_button = ttk.Button(middle_frame, text="Cancelar", command=stop_filling_logs_table)
    cancel_update_list_button.grid(row=0, column=2, padx=5)
    cancel_update_list_button.config(state="disabled")

    # Progress bar
    global progress
    progress = ttk.Progressbar(root, orient="horizontal", mode="indeterminate")
    progress.grid(row=2, column=0, columnspan=6, sticky="ew", padx=10, pady=10)
    progress.grid_remove()
    

    # Logs list
    table_frame = ttk.Frame(root)
    table_frame.grid(row=3, column=0, columnspan=6, sticky="ew", padx=10, pady=5)
    
    root.grid_rowconfigure(1, weight=1)
    root.grid_columnconfigure(0, weight=1)

    columns = ("#1", "#2", "#3")
    global tree
    tree = ttk.Treeview(table_frame, columns=columns, show="headings", selectmode="extended")
    tree.heading("#1", text="Registro")
    tree.heading("#2", text="Nombre de archivo")
    tree.heading("#3", text="Tamaño (bytes)")
    tree.grid(row=0, column=0, sticky="nsew")
    scrollbar = ttk.Scrollbar(table_frame, orient="vertical", command=tree.yview)
    scrollbar.grid(row=0, column=2, sticky="ns")
    tree.configure(yscroll=scrollbar.set)

    # Set the size of the window to be proportional to the screen size
    root.update()
    root.geometry("")



app = tk.Tk()
app_config = {
    "title": constants.app_name,
    "dimensions": constants.window_proportions,
    "icon": constants.icon_path,
}
app_setup(app, app_config)
app.mainloop()