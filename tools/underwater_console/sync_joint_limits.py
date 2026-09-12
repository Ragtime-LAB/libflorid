"""Export the two firmware URDFs' joint limits for the standalone console."""
import argparse
import json
from pathlib import Path
import xml.etree.ElementTree as ET


def export(models):
    axes = []
    for arm in ("master", "slave"):
        tree = ET.parse(models / f"{arm}.urdf").getroot()
        for i in range(1, 5):
            name = f"joint{i}"
            limits = tree.find(f"joint[@name='{name}']/limit")
            axes.append(dict(arm=arm, joint=name, lower=float(limits.get("lower")),
                             upper=float(limits.get("upper")), source=f"{arm}.urdf"))
        axes.append(dict(arm=arm, joint="gripper", lower=-3.14, upper=3.14,
                         source="User-specified motor range; gripper is not in the URDF"))
    return {"units": "rad", "coordinates": "motor", "axes": axes}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("models", type=Path, help="Firmware underwater_teleop/models directory")
    args = parser.parse_args()
    output = Path(__file__).parent / "underwater_console/joint_limits.json"
    output.write_text(json.dumps(export(args.models), indent=2) + "\n")
    print(output)
