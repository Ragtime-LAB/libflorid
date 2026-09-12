import argparse
import tkinter as tk
from .app import UnderwaterApp


def main():
    parser = argparse.ArgumentParser(description="Underwater USB 控制台")
    parser.add_argument("--smoke", action="store_true", help="构建窗口并退出，不扫描或连接 USB")
    args = parser.parse_args()
    root = tk.Tk()
    app = UnderwaterApp(root, auto_scan=not args.smoke)
    if args.smoke:
        root.after(250, app.close)
    root.mainloop()


if __name__ == "__main__":
    main()
