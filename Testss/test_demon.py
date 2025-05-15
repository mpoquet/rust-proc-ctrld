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
            ["cargo", "run", "--bin", "run_demon", "--", str(port)],
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
        print(f"size : {size}")
        data = client.recv(int(size))
        res_port = receive_TCPSocketListening(data,int(size))
        print(f"received port {res_port}")
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


def launch_process(IP_address, daemon, command):
    process, daemon_type, port = daemon
    assert process.poll() is None
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        print(f"size : {size}")
        data = client.recv(int(size))
        res_port = receive_TCPSocketListening(data,int(size))
        print(f"received port {res_port}")

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        print(f"size : {size}")
        data = client.recv(int(size))
        pid = receive_processlaunched(data,int(size))
        print(f"pid : {pid}")
        if pid < 0 :
            client.close()
            return -1
        else :
            client.close()
            return 1
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
        return -1
    except socket.error as e:
        print(f"Erreur de socket : {e}")
        return -1

# def launch_process(IP_address, daemon, command):
#     process, daemon_type, port = daemon
#     assert process.poll() is None
#     print(f"Testing connection for {daemon_type} daemon on port {port}")

#     try:
#         client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#         client.connect((IP_address, port))
        
#         # Envoi de la commande
#         buf = send_command_to_demon(command)
#         header = buf.size.to_bytes(4, byteorder='little')
#         client.sendall(header + buf.buffer)
#         print(f"Sent command buffer of size: {buf.size}")

#         # Réception de la réponse
#         size_bytes = client.recv(4)
#         if not size_bytes:
#             print("Failed to receive size bytes")
#             client.close()
#             return -1
            
#         size = int.from_bytes(size_bytes, byteorder='little')
#         print(f"Expected response size: {size}")

#         # Recevoir les données en plusieurs fois si nécessaire
#         data = bytearray()
#         remaining = size
#         while remaining > 0:
#             chunk = client.recv(remaining)
#             if not chunk:
#                 print("Connection closed prematurely")
#                 client.close()
#                 return -1
#             data.extend(chunk)
#             remaining -= len(chunk)
            
#         print(f"Received data size: {len(data)}")
#         print(f"First few bytes: {data[:20].hex()}")

#         # Convertir en bytes pour le traitement
#         data = bytes(data)
#         pid = receive_processlaunched(data, size)
#         print(f"Received PID: {pid}")
        
#         client.close()
#         return 1 if pid >= 0 else -1

#     except ConnectionRefusedError:
#         print(f"Connection refused at {IP_address}:{port}")
#         return -1
#     except socket.error as e:
#         print(f"Socket error: {e}")
#         return -1
    
def fail_launch_process(IP_address, daemon, command):
    process, daemon_type, port = daemon
    assert process.poll() is None
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        print(f"size : {size}")
        data = client.recv(int(size))

        size_bytes= client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        data = client.recv(int(size))
        error_code = receive_childcreationerror(data,int(size))
        print(f"error_code : {error_code}")
        if error_code < 0 :
            client.close()
            return -1
        else :
            client.close()
            return 1
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
        return -1
    except socket.error as e:
        print(f"Erreur de socket : {e}")
        return -1
    
def process_terminated(IP_address, daemon, command):
    process, daemon_type, port = daemon
    assert process.poll() is None
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        print(f"size : {size}")
        data = client.recv(int(size))

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        print(f"size : {size}")
        data = client.recv(int(size))

        size_bytes= client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        data = client.recv(int(size))
        info = receive_processterminated(data,int(size))
        print(f"error_code : {info.error_code}, pid: {info.pid}")
        if info.pid<0 or info.error_code<0:
            client.close()
            return -1
        else :
            client.close()
            return 1
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
        return -1
    except socket.error as e:
        print(f"Erreur de socket : {e}")
        return -1
    

@pytest.mark.timeout(3)
def test_daemon_connection(daemon):
    res = connect_to_daemon("127.0.0.1", daemon)
    assert res != -1, "Connexion échouée"

@pytest.mark.timeout(3)
def test_receiving_launch_process(daemon):
    # Configuration des arguments
    path = "/bin/ls"
    args = ["ls", "-l"]
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack

    # Configuration des événements à surveiller
    to_watch = []

    # Création de la commande
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
        to_watch_size=len(to_watch)
    )
    res = launch_process("127.0.0.1", daemon,command)
    assert res != -1, "Launch process failed"

@pytest.mark.timeout(3)
def test_receiving_failed_launch_process(daemon):
    # Configuration des arguments
    path = "/bin/OIHDOEJ"
    args = ["lss", "-la"]
    envp = ["badenvp"]
    flags = -1
    stack_size = 0  # 1 MB de stack

    # Configuration des événements à surveiller
    to_watch = []

    # Création de la commande
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
        to_watch_size=len(to_watch)
    )
    res = fail_launch_process("127.0.0.1", daemon,command)
    assert res != -1, "Receiving fail info failed"

@pytest.mark.timeout(3)
def test_receiving_process_terminated(daemon):
    # Configuration des arguments
    path = "/bin/ls"
    args = ["ls", "-l"]
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack

    # Configuration des événements à surveiller
    to_watch = []

    # Création de la commande
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
        to_watch_size=len(to_watch)
    )
    res = process_terminated("127.0.0.1", daemon,command)
    assert res != -1, "Receiving process_terminated failed"

@pytest.mark.timeout(3)
def test_echo(daemon):
    path = "/bin/echo"
    args = ["echo", "bonjour"]
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack
    to_watch = []
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
        to_watch_size=len(to_watch)
    )
    res = process_terminated("127.0.0.1", daemon,command)
    assert res != -1, "Receiving process_terminated failed"