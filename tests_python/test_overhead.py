from serialization import *

import subprocess
import time
import os
import signal
import pytest
import socket
import numpy as np

RUST_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../rust"))
C_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../c/builddir"))

DAEMON_PORTS = {
    "rust": 8080,
    "c": 9090
}

nb_mesures = 30
nb_iteration = 30

temps_sans_demon = []
temps_demon_rust = []
temps_demon_c = []

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



def boucle_launch_process(IP_address, daemon, command):
    process, daemon_type, port = daemon
    assert process.poll() is None
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        #connexion avec le demon
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')

        #receiving conection successfull
        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        data = client.recv(int(size))
        res_port = receive_TCPSocketListening(data,int(size))
        #Boucle de i mesures
        for i in range(nb_mesures):

            #Lancement chronomètre
            t_start = time.perf_counter()

            #Boucle de j itérations
            for j in range(nb_iteration):
                client.sendall(header + buf.buffer)

                #receiving launch process
                size_bytes = client.recv(4)
                size = int.from_bytes(size_bytes, byteorder='little')
                data = client.recv(int(size))
                pid = receive_processlaunched(data,int(size))

                #receiving execve terminated
                size_bytes = client.recv(4)
                size = int.from_bytes(size_bytes, byteorder='little')
                print(f"size : {size}")
                data = client.recv(int(size))
                info_execve= receive_execveterminated(data,int(size))

                #receiving process terminated
                size_bytes= client.recv(4)
                size = int.from_bytes(size_bytes, byteorder='little')
                data = client.recv(int(size))
                info = receive_processterminated(data,int(size))
            
            #Arrêt chronomètre et enregistrement temps de j itérations
            t_stop = time.perf_counter()

            difference = t_stop - t_start

            match daemon_type:
                case "rust":
                    temps_demon_rust.append(difference)
                case "c":
                    temps_demon_c.append(difference)
                case _ : print("non couvert")

    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
        return -1
    except socket.error as e:
        print(f"Erreur de socket : {e}")
        return -1



#Lancement des test
@pytest.mark.timeout(10)
def test_sans_demon():
    #Boucle de i mesures
    for i in range(nb_mesures):
        #Lancement chronomètre
        t_start = time.perf_counter()

        #Boucle de j itérations
        for j in range(nb_iteration):
            subprocess.call(["echo", ""] , stdout=subprocess.DEVNULL)

        #Arrêt chronomètre et enregistrement temps de j itérations
        t_stop = time.perf_counter()

        difference = t_stop - t_start

        temps_sans_demon.append(difference)



@pytest.mark.timeout(10)
def test_avec_demon(daemon):
    process, daemon_type, port = daemon

    # Configuration des arguments
    path = "/bin/echo"
    args = ["echo", "bonjour"]
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
    )

    boucle_launch_process("127.0.0.1", daemon,command)




@pytest.mark.timeout(10)
def test_traitement_donne():
    print("\n")
    print(f"Moyenne de temps sans demon : {np.mean(temps_sans_demon)} secondes")
    print(f"Moyenne de temps avec demon rust: {np.mean(temps_demon_rust)} secondes")
    print(f"Moyenne de temps avec demon c : {np.mean(temps_demon_c)} secondes")
    print(f"{len(temps_sans_demon)} {len(temps_demon_rust)} {len(temps_demon_c)}")