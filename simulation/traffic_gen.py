#!/usr/bin/env python3
"""
traffic_gen.py — OAI 4G RFSim Traffic Generator
================================================
Starts iperf3 servers on trf_gen and clients on each UE container,
then collects and prints results.

Usage:
    uv run traffic_gen.py --ues 2
    uv run traffic_gen.py --ues 3 --bandwidth 8M --duration 120
    uv run traffic_gen.py --ues 2 --dry-run     ← prints commands without running
"""

import argparse
import subprocess
import sys
import time
from dataclasses import dataclass

import config


def get_tunnel_ip(container: str) -> str | None:
    """
    Dynamically discover the tunnel IP assigned to this UE container.
    Looks for any 'oaitun_ue' interface and returns its IPv4 address.
    We cannot hardcode these — the SPGW assigns them by attach order.
    """
    output = docker_exec(container, ["ip", "addr", "show"], detach=False)
    # simpler approach: just grep for the oaitun interface block
    for i, line in enumerate(output.splitlines()):
        if "oaitun_ue" in line:
            # next lines contain the inet address
            for sub in output.splitlines()[i:i+5]:
                sub = sub.strip()
                if sub.startswith("inet ") and not sub.startswith("inet6"):
                    return sub.split()[1].split("/")[0]
    return None


# ─── Data Model ───────────────────────────────────────────────────────────────

@dataclass
class UE:
    """
    Represents one simulated UE and all the addresses it needs.
    A dataclass is just a clean way to group related values together —
    think of it as a named record/struct.
    """
    index:        int
    container:    str
    docker_ip:    str   # IP on the Docker bridge network (192.168.61.x)
    tunnel_ip:    str   # IP assigned through the LTE tunnel by SPGW (12.0.0.x)
    port:         int   # iperf3 port dedicated to this UE

    @classmethod
    def from_index(cls, index: int) -> "UE":
        """Build a UE object from just its index — all other fields are derived."""
        return cls(
            index=index,
            container=config.ue_container_name(index),
            docker_ip=config.ue_docker_ip(index),
            tunnel_ip=config.ue_tunnel_ip(index),
            port=config.ue_port(index),
        )

    def __str__(self) -> str:
        return (
            f"UE{self.index} | container={self.container} "
            f"| tunnel={self.tunnel_ip} | port={self.port}"
        )


# ─── Docker Execution Helpers ─────────────────────────────────────────────────

def docker_exec(container: str, cmd: list[str], detach: bool = False, dry_run: bool = False) -> str:
    """
    Run a command inside a Docker container.

    container : name of the Docker container
    cmd       : the command to run, as a list of strings (safer than a raw string)
    detach    : if True, use -d flag so the command runs in the background
    dry_run   : if True, just print the command instead of running it

    Returns the captured stdout (empty string if detached or dry_run).

    KEY CONCEPT — why a list and not a string?
      subprocess with a list avoids shell injection issues and handles
      spaces in arguments correctly. Compare:
        ✗  "iperf3 -B 12.0.0.2 -p 5201"   ← a string, shell must parse it
        ✓  ["iperf3", "-B", "12.0.0.2", "-p", "5201"]  ← already parsed
    """
    flags = ["-d"] if detach else ["-it"]
    full_cmd = ["docker", "exec"] + flags + [container] + cmd

    if dry_run:
        print(f"  [DRY RUN] {' '.join(full_cmd)}")
        return ""

    result = subprocess.run(
        full_cmd,
        capture_output=True,
        text=True,
    )

    if result.returncode != 0 and not detach:
        # Detached commands sometimes return non-zero even on success;
        # only raise for foreground commands where we expect output.
        print(
            f"  ✗ Command failed (exit {result.returncode}): {' '.join(full_cmd)}")
        print(f"    stderr: {result.stderr.strip()}")

    return result.stdout.strip()


def check_container_running(container: str) -> bool:
    """Return True if the container exists and is running."""
    result = subprocess.run(
        ["docker", "inspect", "-f", "{{.State.Running}}", container],
        capture_output=True, text=True,
    )
    return result.stdout.strip() == "true"


# ─── Traffic Steps ────────────────────────────────────────────────────────────

def start_servers(ues: list[UE], dry_run: bool):
    """
    Start one iperf3 server per UE on the trf_gen container.

    Each server listens on a unique port so they don't conflict.
    -s = server mode
    -B = bind to trf_gen's IP (so responses go out the right interface)
    -D = daemon mode (runs in background inside the container)
    """
    print(f"\n{'─'*50}")
    print(f"  [1/3] Starting iperf3 servers on {config.TRF_GEN_CONTAINER}")
    print(f"{'─'*50}")

    if not dry_run and not check_container_running(config.TRF_GEN_CONTAINER):
        print(
            f"  ✗ Container '{config.TRF_GEN_CONTAINER}' is not running. Start the network first.")
        sys.exit(1)

    for ue in ues:
        print(f"  → Server for UE{ue.index} on port {ue.port} ...", end=" ")
        docker_exec(
            config.TRF_GEN_CONTAINER,
            ["iperf3", "-s", "-B", config.TRF_GEN_IP, "-p", str(ue.port)],
            detach=True,   # -D (daemon) is an iperf3 flag, not a Docker flag
            dry_run=dry_run,
        )
        if not dry_run:
            time.sleep(2)   # give it a moment to bind
            # Verify it's actually listening before saying ✓
            check = docker_exec(
                config.TRF_GEN_CONTAINER,
                ["pgrep", "-a", "iperf3"],
                detach=False,
                dry_run=False,
            )
            if check:
                print("✓")
            else:
                print("✗ server not listening — check trf_gen container")
                sys.exit(1)


def start_clients(ues: list[UE], bandwidth: str, duration: int, dry_run: bool):
    """
    Start one iperf3 client per UE container.

    -c  = connect to trf_gen IP (the server)
    -B  = bind source IP to the UE's TUNNEL IP (12.0.0.x)
          ↑ This is the critical flag — without it, traffic bypasses the LTE tunnel
    -u  = UDP (realistic mobile data behaviour)
    -b  = target bandwidth (application data rate, not radio bandwidth)
    -t  = duration in seconds
    -R  = Reverse: server → client (simulates downlink: network sending to UE)
    -p  = port matching the dedicated server
    --logfile = save results inside the container for later collection
    """
    print(f"\n{'─'*50}")
    print(f"  [2/3] Starting iperf3 clients")
    print(f"{'─'*50}")

    for ue in ues:
        if not dry_run and not check_container_running(ue.container):
            print(f"  ✗ Container '{ue.container}' is not running — skipping.")
            continue
        # Discover the actual tunnel IP at runtime
        actual_tunnel_ip = get_tunnel_ip(
            ue.container) if not dry_run else ue.tunnel_ip
        if not dry_run and not actual_tunnel_ip:
            print(
                f"  ✗ No tunnel interface found on {ue.container} — is it attached?")
            continue

        print(
            f"  → UE{ue.index} (actual tunnel: {actual_tunnel_ip}) → {config.TRF_GEN_IP}:{ue.port} ...", end=" ")

        log_path = f"/tmp/iperf3_ue{ue.index}.txt"

        docker_exec(
            ue.container,
            [
                "iperf3",
                "-c", config.TRF_GEN_IP,
                "-B", actual_tunnel_ip,
                "-u",
                "-b", bandwidth,
                "-t", str(duration),
                "-R",
                "-p", str(ue.port),
                "--logfile", log_path,
            ],
            detach=True,
            dry_run=dry_run,
        )
        if not dry_run:
            print("✓")


def wait_and_collect(ues: list[UE], duration: int, dry_run: bool):
    """Wait for tests to finish, then read result files from each UE container."""
    wait_time = duration + 5   # small buffer after test ends

    print(f"\n{'─'*50}")
    print(f"  [3/3] Waiting {wait_time}s for tests to complete...")
    print(f"{'─'*50}")

    if dry_run:
        print("  [DRY RUN] Would wait here.")
        return

    # Show a simple countdown every 30 seconds so you know it's alive
    elapsed = 0
    interval = 30
    while elapsed < wait_time:
        remaining = wait_time - elapsed
        print(f"  ⏱  {remaining}s remaining...", flush=True)
        sleep_for = min(interval, remaining)
        time.sleep(sleep_for)
        elapsed += sleep_for

    # ── Collect results ───────────────────────────────────────────────────────
    print(f"\n{'═'*55}")
    print(f"  RESULTS")
    print(f"{'═'*55}")

    for ue in ues:
        log_path = f"/tmp/iperf3_ue{ue.index}.txt"
        print(f"\n  ── UE{ue.index} | {ue.container} ({ue.tunnel_ip}) ──")

        output = docker_exec(ue.container, ["cat", log_path], dry_run=False)

        if output:
            # Highlight the summary line (iperf3 ends with a line containing "sender" or "receiver")
            for line in output.splitlines():
                marker = "→ " if (
                    "sender" in line or "receiver" in line) else "  "
                print(f"  {marker}{line}")
        else:
            print(
                "  (no result file — container may have stopped or tunnel did not connect)")

    print(f"\n{'═'*55}\n")


# ─── Main ─────────────────────────────────────────────────────────────────────

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="OAI 4G RFSim — iperf3 traffic generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  uv run traffic_gen.py --ues 2
  uv run traffic_gen.py --ues 4 --bandwidth 8M --duration 60
  uv run traffic_gen.py --ues 2 --dry-run
        """,
    )
    parser.add_argument("--ues",       type=int, required=True,
                        help="Number of UEs (must match running containers)")
    parser.add_argument("--bandwidth", type=str, default=config.DEFAULT_BANDWIDTH,
                        help=f"iperf3 bandwidth per UE (default: {config.DEFAULT_BANDWIDTH}). "
                        "Keep under ~12M for 5 MHz LTE.")
    parser.add_argument("--duration",  type=int, default=config.DEFAULT_DURATION,
                        help=f"Test duration in seconds (default: {config.DEFAULT_DURATION})")
    parser.add_argument("--dry-run",   action="store_true",
                        help="Print all docker commands without executing them")
    return parser.parse_args()


def main():
    args = parse_args()

    if args.ues < 1 or args.ues > config.MAX_UES:
        print(f"ERROR: --ues must be between 1 and {config.MAX_UES}")
        sys.exit(1)

    ues = [UE.from_index(i) for i in range(args.ues)]

    tag = " [DRY RUN]" if args.dry_run else ""
    print(f"""
╔══════════════════════════════════════════════╗
║   OAI 4G RFSim — Traffic Generator{tag:<10}║
╠══════════════════════════════════════════════╣
║  UEs        : {args.ues:<30} ║
║  Bandwidth  : {args.bandwidth:<30} ║
║  Duration   : {str(args.duration) + 's':<30} ║
╚══════════════════════════════════════════════╝""")

    for ue in ues:
        print(f"  {ue}")

    start_servers(ues, dry_run=args.dry_run)
    start_clients(ues, bandwidth=args.bandwidth,
                  duration=args.duration, dry_run=args.dry_run)
    wait_and_collect(ues, duration=args.duration, dry_run=args.dry_run)


if __name__ == "__main__":
    main()
