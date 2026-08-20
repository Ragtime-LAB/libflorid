"""Entry point:  python -m gravity_tuner  (run from this directory)."""

import tkinter as tk

from .app import GravityTunerApp


def main() -> None:
    s_root = tk.Tk()
    s_app = GravityTunerApp(s_root)
    s_root.protocol("WM_DELETE_WINDOW", s_app.on_close)
    s_root.mainloop()


if __name__ == "__main__":
    main()