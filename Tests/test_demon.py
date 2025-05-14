from serialization import *

import subprocess
import time
import os
import signal
import pytest
import socket

RUST_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../rust"))
C_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../c/builddir"))

DAEMON_PORTS = {
    "rust": 8080,
    "c": 9090
}

@pytest.fixture(params=["rust", "c"])
def daemon(request):
    daemon_type = request.param
    port = DAEMON_PORTS[daemon_type]

    if daemon_type == "rust":
        process = subprocess.Popen(
            ["cargo", "run", "--bin", "run_demon"],
            cwd=RUST_PROJECT_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid
        )
    elif daemon_type == "c":
        process = subprocess.Popen(
            ["./daemon", str(port)],
            cwd=C_PROJECT_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid
        )

    time.sleep(1)
    yield process, daemon_type, port
    os.killpg(os.getpgid(process.pid), signal.SIGTERM)
    process.wait(timeout=5)


def connect_to_daemon(IP_address, daemon):
    process, daemon_type, port = daemon

    # Vérifie que le processus est toujours en cours d'exécution
    assert process.poll() is None

    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        print(f"Connexion vers {IP_address}:{port} réussie.")
        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        data = client.recv(int(size))
        res_port = receive_TCPSocketListening(data,int(size))
        if res_port == port :
            client.close()
            return 1
        else :
            client.close()
            return -1
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
        return -1
    except socket.error as e:
        print(f"Erreur de socket : {e}")
        return -1
    
def test_daemon_connection(daemon):
    res = connect_to_daemon("127.0.0.1", daemon)
    assert res != -1, "Connexion échouée"




# def test_echo_bonjour(daemon_process):
#     client = connect_to_daemon(HOST, PORT)
#     if client is None:
#         pytest.fail("Échec de la connexion au démon.")

#     # Envoi via la sérialisation
#     buf = serialisation("echo", "bonjour", "", 0, 0)
#     client.sendall(buf)

#     # Réception de la réponse
#     response = client.recv(1024)
#     if not response:
#         pytest.fail("Aucune réponse reçue du démon.")
#     handler(response)
