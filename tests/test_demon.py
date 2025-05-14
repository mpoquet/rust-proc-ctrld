from serialisation_python import serialisation
from deserialisation import handler

import subprocess
import time
import os
import signal
import pytest
import socket

# Chemin vers le dossier contenant le projet Rust
RUST_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../rust"))

HOST = "127.0.0.1"
PORT = 8080

@pytest.fixture
def daemon_process():
    process = subprocess.Popen(
        ["cargo", "run", "--bin", "run_demon"],
        cwd=RUST_PROJECT_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        preexec_fn=os.setsid
    )

    time.sleep(1)
    yield process
    os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    process.wait(timeout=5)

def launch_C_daemon(port):
    proc = subprocess.Popen(
        ["../c/builddir/daemon", str(port)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    time.sleep(1)
    yield proc

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


def test_echo_bonjour(daemon_process):
    client = connect_to_daemon(HOST, PORT)
    if client is None:
        pytest.fail("Échec de la connexion au démon.")

    # Envoi via la sérialisation
    buf = serialisation("echo", "bonjour", "", 0, 0)
    client.sendall(buf)

    # Réception de la réponse
    response = client.recv(1024)
    if not response:
        pytest.fail("Aucune réponse reçue du démon.")
    assert handler(response) in "test\n"
    client.close()  
