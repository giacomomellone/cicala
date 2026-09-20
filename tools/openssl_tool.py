"""Find an OpenSSL executable with Ed25519 support on macOS and Linux."""

import os
import shutil
import subprocess
from pathlib import Path


def executable() -> str:
    configured = os.environ.get("OPENSSL")
    candidates = (
        [configured]
        if configured
        else [
            shutil.which("openssl"),
            "/opt/homebrew/opt/openssl@3/bin/openssl",
            "/usr/local/opt/openssl@3/bin/openssl",
        ]
    )
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            version = subprocess.check_output([candidate, "version"], text=True)
            if version.startswith("OpenSSL "):
                return candidate
    raise RuntimeError(
        "Install OpenSSL 3 (brew install openssl@3 on macOS), or set OPENSSL to its executable"
    )
