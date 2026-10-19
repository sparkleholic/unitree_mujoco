#!/usr/bin/env python3
"""CLI regressions: invalid input must not silently start a controller."""
import subprocess
import sys
from pathlib import Path
binary = Path(sys.argv[1])
for args in (["--api", "invalid"], ["--duration", "nan"], ["--domain", "-1"], ["--action", "invalid"]):
    p = subprocess.run([str(binary), *args], capture_output=True, timeout=5)
    assert p.returncode == 2, (args, p.returncode)
p = subprocess.run([str(binary), "--help"], capture_output=True, text=True, timeout=5)
assert p.returncode == 0 and "lowcmd|sport" in p.stdout
p = subprocess.run([str(binary), "lo", "--domain", "76", "--duration", "0.05"],
                   input="\n", capture_output=True, text=True, timeout=5)
assert p.returncode == 0 and "Press enter to start" in p.stdout
print("PASS invalid CLI, help and interactive positional-network compatibility")
