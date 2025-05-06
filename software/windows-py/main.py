import tkinter as tk
from tkinter import ttk
import constants
import commands
from commands import SerialPortManager
import threading

serial_port_manager = SerialPortManager()


def stop_filling_logs_table():
    serial_port_manager.set_abort_flag(True)
    progress.stop()
    cancel_button.config(state="disabled")
    update_log_list_button.config(state="normal")


def fill_logs_table():
    logs = serial_port_manager.get_logs_list()
    if not logs:
        return
    for log in logs:
        tree.insert("", "end", values=log)
    root.after(0, fill_logs_table_end)
    serial_port_manager.set_abort_flag(False)
    update_log_list_button.config(state="normal")


def on_connect_attempt():
    port = port_combobox.get()
    if port:
        if serial_port_manager.connect(port):
            connect_button.config(state="disabled")
            progress.start()
            cancel_button.config(state="normal")
            global fill_logs_thread
            fill_logs_thread = threading.Thread(target=fill_logs_table)
            fill_logs_thread.start()


def app_setup(root, config):
    # Root app setup
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
    connect_button = ttk.Button(top_frame, text="Conectar", command=on_connect_attempt)
    connect_button.grid(row=0, column=2, padx=5)

    global cancel_button
    cancel_button = ttk.Button(top_frame, text="Cancelar", command=stop_filling_logs_table)
    cancel_button.grid(row=0, column=3, padx=5)
    cancel_button.config(state="disabled")

    global update_log_list_button
    update_log_list_button = ttk.Button(top_frame, text="Actualizar lista de registros", command=fill_logs_table)
    update_log_list_button.grid(row=0, column=4, padx=5)
    update_log_list_button.config(state="disabled")

    # Logs list
    table_frame = ttk.Frame(root)
    table_frame.grid(row=1, column=0, columnspan=6, sticky="ew", padx=10, pady=5)
    
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

    
    # Progress bar
    global progress
    progress = ttk.Progressbar(root, orient="horizontal", mode="indeterminate")
    progress.grid(row=2, column=0, columnspan=6, sticky="ew", padx=10, pady=10)

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