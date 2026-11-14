"""Desktop startup, diagnostics and error reporting; never opens a device on startup."""
import argparse
import json
import logging
from logging.handlers import RotatingFileHandler
import os
from pathlib import Path
import platform
import sys


def build_info():
    s_path = Path(__file__).with_name("build-info.json")
    if s_path.is_file():
        return json.loads(s_path.read_text(encoding="utf-8"))
    return {"version": "development", "sdk_version": "source", "commit": "unknown"}


def configure_logging():
    s_base = Path(os.environ.get("LOCALAPPDATA", Path.home() / ".local" / "state"))
    s_path = s_base / "UnderwaterSingle" / "logs" / "desktop.log"
    s_path.parent.mkdir(parents=True, exist_ok=True)
    s_handler = RotatingFileHandler(s_path, maxBytes=2_000_000, backupCount=2, encoding="utf-8")
    logging.basicConfig(level=logging.INFO, handlers=[s_handler],
                        format="%(asctime)s %(levelname)s %(message)s", force=True)
    return s_path


def load_sdk():
    if not getattr(sys, "frozen", False):
        s_repo = Path(__file__).resolve().parents[2]
        for s_build in (s_repo / "build", s_repo / "build-underwater-sdk"):
            s_extension = s_build / "pyflorid"
            if s_extension.is_dir():
                sys.path[:0] = [str(s_extension), str(s_repo / "pyflorid")]
                break
    import pyflorid
    return pyflorid


def main(s_argv=None):
    s_parser = argparse.ArgumentParser(description="Underwater Single desktop console")
    s_action = s_parser.add_mutually_exclusive_group()
    s_action.add_argument("--smoke", action="store_true", help="Create the UI and exit without scanning devices")
    s_action.add_argument("--self-test", action="store_true", help="Check embedded dependencies without accessing hardware")
    s_action.add_argument("--version", action="store_true", help="Show application build information")
    s_parser.add_argument("--report", type=Path, help="Write a diagnostic JSON report (also in windowed builds)")
    s_args = s_parser.parse_args(s_argv)
    s_result = {"ok": False}
    s_root = None
    s_log_path = None
    s_errors = []
    try:
        s_log_path = configure_logging()
        s_result.update(build_info(), python=platform.python_version(),
                        frozen=bool(getattr(sys, "frozen", False)), log=str(s_log_path))
        logging.info("Starting %s", s_result)
        if s_args.version:
            s_result["ok"] = True
            if sys.stdout is not None:
                print(json.dumps(s_result, ensure_ascii=False))
        else:
            s_sdk = load_sdk()
            import tkinter as tk
            s_root = tk.Tk()

            def s_callback_error(s_type, s_value, s_traceback):
                logging.error("Tk callback failed", exc_info=(s_type, s_value, s_traceback))
                s_errors.append(str(s_value))
                if not s_args.smoke:
                    from tkinter import messagebox
                    messagebox.showerror("Underwater Single", f"{s_value}\n\n日志：{s_log_path}", parent=s_root)

            s_root.report_callback_exception = s_callback_error
            if s_args.self_test:
                s_root.withdraw()
                # Import the actual Windows backend, without enumerating/opening a port.
                import serial
                from serial.tools import list_ports_windows
                from pyflorid import _pyflorid as s_native
                if not callable(s_sdk.UnderwaterSingle.connect) or not hasattr(s_native, "_SingleUsb"):
                    raise RuntimeError("The bundled SDK does not provide the Single USB API")
                if not callable(list_ports_windows.comports):
                    raise RuntimeError("Windows serial enumeration backend is missing")
                if getattr(sys, "frozen", False) and not Path(s_native.__file__).resolve().is_relative_to(Path(sys._MEIPASS).resolve()):
                    raise RuntimeError("Native SDK was loaded from outside the executable bundle")
                s_result.update(native_extension=str(s_native.__file__), pyserial=serial.VERSION,
                                tcl=s_root.tk.call("info", "patchlevel"),
                                tk=s_root.tk.call("package", "require", "Tk"))
                s_root.destroy()
                s_root = None
            else:
                from console import Console
                s_app = Console(s_root, s_sdk)
                s_root.title(f"Underwater Single · 单臂控制台 · {s_result['version']}")
                if s_args.smoke:
                    s_root.after(1200, s_app.close)
                s_root.mainloop()
                s_root = None
            s_result.update(ok=not s_errors, callback_errors=s_errors)
    except Exception as s_error:
        logging.exception("Desktop startup or diagnostics failed")
        s_result["error"] = str(s_error)
        if not (s_args.smoke or s_args.self_test or s_args.version):
            try:
                from tkinter import messagebox
                messagebox.showerror("Underwater Single", f"启动失败：{s_error}\n\n日志：{s_log_path}")
            except Exception:
                pass
    finally:
        if s_root is not None:
            s_root.destroy()
        if s_args.report:
            s_args.report.parent.mkdir(parents=True, exist_ok=True)
            s_args.report.write_text(json.dumps(s_result, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0 if s_result["ok"] else 1
