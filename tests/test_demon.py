import subprocess
import time
import os
import signal
import pytest

@pytest.fixture
def daemon_proc():
    proc = subprocess.Popen(
        ["cargo", "run", "--bin", "run_demon"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    time.sleep(1)
    yield proc


def test_daemon_launch(daemon_process):
    stdout = daemon_process.stdout.readline()
    assert "Démon lancé" in stdout