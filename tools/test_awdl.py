#!/usr/bin/env python3
"""Run host AWDL protocol checks under memory/undefined-behavior sanitizers."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="forbiddenitlwm-tests-") as directory:
    for name in ("awdl_requested_configuration", "awdl_protocol", "awdl_frame_builder", "awdl_sync_prefix", "awdl_local_schedule", "awdl_probe_policy", "awdl_data_payload", "awdl_data_frame", "awdl_observation", "awdl_regulatory", "awdl_tx_burst", "awdl_peer_schedule", "btleadv_hci"):
        output = Path(directory) / name
        subprocess.run([
            "xcrun", "clang++", "-std=c++14",
            "-D__IO80211_TARGET=__MAC_26_0", "-I" + str(root / "include"), "-I" + str(root / "BTLEAdvFixup"),
            "-fsanitize=address,undefined", str(root / "tests" / (name + ".cpp")),
            "-o", str(output),
        ], check=True)
        subprocess.run([str(output)], check=True)
subprocess.run(["python3", str(root / "tests/monitor_tx_ownership.py")], check=True)
subprocess.run(["python3", str(root / "tests/monitor_channel.py")], check=True)
subprocess.run(["python3", str(root / "tests/monitor_probe.py")], check=True)
subprocess.run(["python3", str(root / "tests/btleadv_patterns.py")], check=True)
subprocess.run(["python3", str(root / "tests/awdl_snapshot.py")], check=True)
subprocess.run(["python3", str(root / "tests/awdl_bridge.py")], check=True)
subprocess.run(["python3", str(root / "tests/awdl_pool_lease.py")], check=True)
subprocess.run(["python3", str(root / "tests/awdl_rx_copy.py")], check=True)
