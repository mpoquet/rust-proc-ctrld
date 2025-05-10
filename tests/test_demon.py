import subprocess
import time
import os
import signal
import pytest
import socket

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

def launch_C_daemon(port):
    proc = subprocess.Popen(
        ["../c/bin/daemon", str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    time.sleep(1)
    yield proc


def test_daemon_launch(daemon_process):
    stdout = daemon_process.stdout.readline()
    assert "Démon lancé" in stdout

def connect_to_daemon(IP_adress, port):
    try :
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_adress, port))
        print('Connexion vers ' + IP_adress + ':' + str(port) + ' reussie.')
        return client
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_adress}:{port} a refusé la connexion.")
        print("Assurez-vous que le serveur C est lancé et écoute sur le bon port et la bonne interface.")
    except socket.error as e:
        print(f"Erreur de socket : {e}")