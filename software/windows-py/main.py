import tkinter as tk
from tkinter import ttk
import constants

root = tk.Tk()
root.title(constants.app_name)
screen_width = root.winfo_screenwidth()
screen_height = root.winfo_screenheight()
width_prop = constants.window_proportions[0]
height_prop = constants.window_proportions[1]
root.geometry("{}x{}+{}+{}".format(
    int(screen_width * width_prop/2),
    int(screen_height * height_prop/2),
    int(screen_width * (1 - width_prop) / 2),
    int(screen_height * (1 - height_prop) / 2)
))

root.iconphoto(True, tk.PhotoImage(file = constants.icon_path))


root.mainloop()
