from pathlib import Path

# ── Path anchors ──────────────────────────────────────────────────────────────
HERE      = Path(__file__).resolve().parent          # .../openairinterface5g/simulation/
_OAI_ROOT = HERE.parent                              # .../openairinterface5g/
_CI       = _OAI_ROOT / "ci-scripts"                # .../openairinterface5g/ci-scripts/
_COMPOSE_DIR = _CI / "yaml_files" / "4g_rfsimulator_fdd_05MHz"

# ── File paths ────────────────────────────────────────────────────────────────
CONF_DIR      = str(_CI / "conf_files")
TEMPLATE_USIM = str(_CI / "conf_files" / "lteue.usim-ci.conf")
RFSIM_CONF    = str(_CI / "conf_files" / "lteue.rfsim.conf")
UE_BINARY     = str(_OAI_ROOT / "cmake_targets" / "ran_build" / "build" / "lte-uesoftmodem")
OAI_ROOT      = str(_OAI_ROOT)
COMPOSE_DIR   = str(_COMPOSE_DIR)

# ── eNodeB ────────────────────────────────────────────────────────────────────
ENB_CONTAINER = "rfsim4g-oai-enb"
ENB_IP        = "192.168.61.20"

# ── Traffic Generator ─────────────────────────────────────────────────────────
TRF_GEN_CONTAINER = "rfsim4g-trf-gen"
TRF_GEN_IP        = "192.168.61.11"

# ── UE IP Addressing ──────────────────────────────────────────────────────────
UE_DOCKER_IP_BASE  = "192.168.61"
UE_DOCKER_IP_START = 30           # UE0 → .30, UE1 → .31, ...

UE_TUNNEL_IP_PREFIX = "12.0.0"
UE_TUNNEL_IP_START  = 2           # UE0 → .2, UE1 → .3, ...

# ── UE Identity ───────────────────────────────────────────────────────────────
FIRST_MSIN = "0100000001"
MAX_UES    = 10

# ── iperf3 ────────────────────────────────────────────────────────────────────
IPERF3_BASE_PORT  = 5201
# Remember to change this based on the test scenarios or the eNodeB bandwidth
DEFAULT_BANDWIDTH = "10M" # 10M here means 10Mbps (Throughput), not 10MHz (bandwidth)
DEFAULT_DURATION  = 300


# ── Helpers ───────────────────────────────────────────────────────────────────

def ue_docker_ip(index: int) -> str:
    return f"{UE_DOCKER_IP_BASE}.{UE_DOCKER_IP_START + index}"

def ue_tunnel_ip(index: int) -> str:
    return f"{UE_TUNNEL_IP_PREFIX}.{UE_TUNNEL_IP_START + index}"

def ue_container_name(index: int) -> str:
    return f"rfsim4g-oai-lte-ue{index}"

def ue_port(index: int) -> int:
    return IPERF3_BASE_PORT + index

def ue_usim_conf_filename(index: int) -> str:
    return f"lteue.usim-ci-ue{index}.conf"
