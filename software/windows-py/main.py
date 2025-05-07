import tkinter as tk
from tkinter import ttk, font, filedialog, simpledialog, messagebox
from tkcalendar import Calendar, DateEntry
from datetime import datetime
import threading
import constants
import commands
from commands import SerialPortManager


serial_port_manager = SerialPortManager()


def find_arduino_port():
    progress.grid()
    status_label.config(text="Buscando puerto del registrador...")
    connect_button.config(state="disabled")
    default_port = serial_port_manager.find_arduino_port()
    if default_port:
        port_combobox.set(default_port)
        status_label.config(text="Puerto encontrado, listo para conectar.")
    else:
        port_combobox.set("Seleccione un puerto")
        status_label.config(text="El puerto del registrador parece no estar disponible.")
    progress.grid_remove()
    connect_button.config(state="normal")


def fill_logs_table_end():
    serial_port_manager.set_abort_flag(False)
    progress.grid_remove()
    update_log_list_button.config(state="normal")
    download_button.config(state="normal")
    delete_button.config(state="normal")
    status_label.config(text="Lista de registros actualizada.")


def stop_filling_logs_table():
    serial_port_manager.set_abort_flag(True)
    progress.grid_remove()
    update_log_list_button.config(state="normal")
    status_label.config(text="Carga de registros interrumpida.")


def update_log_table():
    serial_port_manager.set_abort_flag(False)
    progress.grid()
    status_label.config(text="Actualizando lista de registros...")
    update_log_list_button.config(state="disabled")
    table.delete(*table.get_children())
    logs = serial_port_manager.get_logs_list()
    if not logs:
        status_label.config(text="No se encontraron registros.")
        return
    for log in logs:
        table.insert("", "end", values=log)
    root.after(0, fill_logs_table_end)


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
            datetime_button.config(state="normal")
            progress.grid_remove()
            status_label.config(text="Registrador conectado, descargando registros...")
            on_update_log_table()
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
        download_button.config(state="disabled")
        delete_button.config(state="disabled")
        datetime_button.config(state="disabled")
        progress.grid_remove()
        status_label.config(text="Puerto serie desconectado.")
        table.delete(*table.get_children())
    else:
        print("Error al desconectar")


def get_selected_items():
    selected_items = table.selection()
    if not selected_items:
        status_label.config(text="Seleccione al menos un registro.")
        return
    selected_logs = [table.item(item_id)["values"][0] for item_id in selected_items]
    return selected_logs


def download_selected():
    selected_logs = get_selected_items()
    if not selected_logs:
        return
    content = serial_port_manager.download_log(selected_logs[0])
    file_path = filedialog.asksaveasfilename(defaultextension=".txt", filetypes=[("Text files", "*.txt")])
    if file_path:
        try:
            with open(file_path, 'w') as f:
                for item in content:
                    f.write(f"{item}\n")
            status_label.config(text=f"Guardado en {file_path}")
        except Exception as e:
            status_label.config(text=f"Error al guardar el archivo: {e}")
    else:
        status_label.config(text=f"Debe seleccionar un archivo para guardar.")


def delete_selected():
    selected_logs = get_selected_items()
    if not selected_logs:
        return
    response = messagebox.askyesno("Confirmar", f"¿Desea eliminar el registro {selected_logs[0]}?")
    if response:
        result = serial_port_manager.delete_log(selected_logs[0])
        if result:
            status_label.config(text="Registro eliminado.")
            table.delete(*table.get_children())
            on_update_log_table()
        else:
            status_label.config(text="Error al eliminar el registro.")
    else:
        status_label.config(text="Eliminación cancelada.")


def set_datetime():
    selected_date = cal.get_date()
    selected_time = time_entry.get()
    combined_datetime_str = selected_date + " " + selected_time  # "YYYY-MM-DD HH:MM"
    status_label.config(text=f"Configurando fecha y hora: {combined_datetime_str}")
    datetime_obj = datetime.strptime(combined_datetime_str, "%Y-%m-%d %H:%M")
    formatted_datetime = datetime_obj.strftime("%Y%m%d%H%M%S")
    print(f"Configure date time: {formatted_datetime}")
    datetime_window.destroy()


def prompt_datetime():
    global datetime_window
    datetime_window = tk.Toplevel(root)
    datetime_window.title("Seleccionar fecha y hora")

    cal_label = ttk.Label(datetime_window, text="Indicar fecha:")
    cal_label.pack(padx=10, pady=5)
    global cal
    cal = Calendar(datetime_window, selectmode="day", date_pattern="yyyy-mm-dd")
    cal.pack(padx=10, pady=5)

    time_label = ttk.Label(datetime_window, text="Ingresar hora (HH:MM):")
    time_label.pack(padx=10, pady=5)
    global time_entry
    time_entry = ttk.Entry(datetime_window)
    time_entry.pack(padx=10, pady=5)

    submit_button = ttk.Button(datetime_window, text="Submit", command=set_datetime)
    submit_button.pack(padx=10, pady=10)


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
    serial_port_frame = ttk.Frame(root)
    serial_port_frame.grid(row=0, column=0, sticky="ew", padx=10, pady=5)

    table_frame = ttk.Frame(root)
    table_frame.grid(row=1, column=0, sticky="ew", padx=10, pady=5)

    actions_frame = ttk.Frame(root)
    actions_frame.grid(row=2, column=0, sticky="ew", padx=10, pady=5)
    
    feedback_frame = ttk.Frame(root)
    feedback_frame.grid(row=3, column=0, sticky="ew", padx=10, pady=5)

    # User feedback
    global status_label
    status_label_font = font.Font(family="Helvetica", size=14, slant="italic")
    status_label = ttk.Label(feedback_frame, font=status_label_font)
    status_label.grid(row=0, column=0, padx=5)
    global progress
    progress = ttk.Progressbar(feedback_frame, orient="horizontal", mode="indeterminate")
    progress.grid(row=0, column=1, sticky="w", padx=10, pady=10)
    progress.start() # Always running

    # Serial port selection combobox and connection controls
    label = ttk.Label(serial_port_frame, text="Puertos disponibles")
    label.grid(row=0, column=0, padx=5)
    
    global connect_button
    connect_button = ttk.Button(serial_port_frame, text="Conectar", command=on_connect)
    connect_button.grid(row=0, column=2, padx=5)

    global disconnect_button
    disconnect_button = ttk.Button(serial_port_frame, text="Desconectar", command=on_disconnect)
    disconnect_button.grid(row=0, column=3, padx=5)
    disconnect_button.config(state="disabled")

    # List available serial ports
    serial_ports = serial_port_manager.list_serial_ports()
    global port_combobox
    port_combobox = ttk.Combobox(serial_port_frame, values=serial_ports)
    port_combobox.grid(row=0, column=1, padx=10, pady=10)
    threading.Thread(target=find_arduino_port).start()

    # Query logs from datalogger
    global update_log_list_button
    update_log_list_button = ttk.Button(table_frame, text="Actualizar lista", command=on_update_log_table)
    update_log_list_button.grid(row=0, column=0, padx=5, pady=10, sticky="w")
    update_log_list_button.config(state="disabled")

    global table
    table = ttk.Treeview(table_frame, columns=("#1", "#2", "#3"), show="headings", selectmode="browse")
    table.heading("#1", text="Registro")
    table.heading("#2", text="Nombre de archivo")
    table.heading("#3", text="Tamaño (bytes)")
    table.grid(row=1, column=0, sticky="nsew")
    scrollbar = ttk.Scrollbar(table_frame, orient="vertical", command=table.yview)
    scrollbar.grid(row=1, column=2, sticky="ns")
    table.configure(yscroll=scrollbar.set)

    actions_label_font = font.Font(family="Helvetica", size=14)
    ttk.Label(actions_frame, text="Acciones", font=actions_label_font).grid(row=0, column=0)

    global download_button
    download_button = ttk.Button(actions_frame, text="Descargar", command=download_selected)
    download_button.grid(row=1, column=0, padx=5, pady=10)
    download_button.config(state="disabled")

    global delete_button
    delete_button = ttk.Button(actions_frame, text="Eliminar", command=delete_selected)
    delete_button.grid(row=1, column=1, padx=5, pady=10)
    delete_button.config(state="disabled")

    global datetime_button
    datetime_button = ttk.Button(actions_frame, text="Configurar fecha y hora", command=prompt_datetime)
    datetime_button.grid(row=1, column=2, padx=5, pady=10)
    datetime_button.config(state="disabled")

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