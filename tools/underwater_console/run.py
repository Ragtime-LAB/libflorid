"""Launch from a source checkout; prefer this worktree's locally built SDK."""
from pathlib import Path
import sys

repo = Path(__file__).resolve().parents[2]
local_extension = repo / "build-underwater-sdk" / "pyflorid"
if local_extension.is_dir():
    sys.path.insert(0, str(repo / "pyflorid"))
    sys.path.insert(0, str(local_extension))

from underwater_console.__main__ import main

if __name__ == "__main__":
    main()
