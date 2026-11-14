"""Launch the single-arm console, preferring this worktree's SDK build."""
from pathlib import Path
import sys
import tkinter as tk
root_dir = Path(__file__).resolve().parents[2]
for build in (root_dir / "build", root_dir / "build-underwater-sdk"):
    extension = build / "pyflorid"
    if extension.is_dir():
        sys.path[:0] = [str(extension), str(root_dir / "pyflorid")]
        break
import pyflorid
from console import Console

def main():
    root = tk.Tk()
    app = Console(root, pyflorid)
    if "--smoke" in sys.argv:
        root.after(1200, app.close)
    root.mainloop()

if __name__ == "__main__":
    main()
