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
        res = client.recv(int(size))

        print(receive_message_from_demon(res,size))

        while(receive_message_from_demon(res,size)!=Event.PROCESS_LAUNCHED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        data = receive_processlaunched(res,int(size))
        print(f"pid : {data}")
        if data < 0 :
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
    
def execve_executed(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.EXECVE_TERMINATED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        data = receive_execveterminated(res,int(size))

        print(f"pid : {data.pid}, command : {data.command_name}, succes : {data.success}")
        if data.success==True and data.pid >0:
            print("Dans la 1ere branche")
            client.close()
            return 1
        else :
            print("Dans la 2eme branche")
            client.close()
            return -1
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
        return -1
    except socket.error as e:
        print(f"Erreur de socket : {e}")
        return -1

def fail_launch_process(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.EXECVE_TERMINATED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        data = receive_execveterminated(res,int(size))

        print(f"pid : {data.pid}, command : {data.command_name}, succes : {data.success}")
        if data.success==False and data.pid >0:
            client.close()
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
    
def process_terminated(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.PROCESS_TERMINATED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        info = receive_processterminated(res,int(size))
        print(f"error_code : {info.error_code}, pid: {info.pid}")
        if info.pid>=0 and info.error_code==0:
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
    
def inotify_watchlist_updated(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.INOTIFY_WATCH_LIST_UPDATED):
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        path = receive_inotifywatchlistupdated(res,int(size))
        print(f"path : {path}")
        if path!=None:
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
    
def inotify_event(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.INOTIFY_PATH_UPDATED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        info = receive_inotifypathupdated(res,int(size))
        print(f"path : {info.path}")
        if info.path!=None:
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
    
def response_time(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))

        #receiving conection successfull
        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        print(f"size : {size}")
        data = client.recv(int(size))

        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        start = time.perf_counter()

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.PROCESS_TERMINATED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        info = receive_processterminated(res,int(size))

        end = time.perf_counter()
        print(f"Time elapsed : {end-start:.4f}")

        print(f"error_code : {info.error_code}, pid: {info.pid}")
        if info.pid>0 and info.error_code==0 and end-start<3.1:
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
    
def watching_socket(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.SOCKET_WATCHED):
            print(receive_message_from_demon(res,size))
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        port_res = receive_socketwatched(res,size)

        client.close()

        if port_res > 0:
            return 1
        else:
            return -1

    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
    except socket.error as e:
        print(f"Erreur de socket : {e}")


def socket_listening(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))
        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.SOCKET_WATCH_TERMINATED):
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        info = receive_socketwatchterminated(res,int(size))

        print(f"dest port : {info.port}")

        if info.port > 0:
            client.close()
            return 1
        else :
            client.close()
            return -1
        
    except ConnectionRefusedError:
        print(f"Échec de la connexion : Le serveur à {IP_address}:{port} a refusé la connexion.")
        print(f"Assurez-vous que le serveur {daemon_type} est lancé et écoute sur le bon port.")
    except socket.error as e:
        print(f"Erreur de socket : {e}")

def kill_process(IP_address, daemon, command):
    process, daemon_type, port = daemon
    print(f"Testing connection for {daemon_type} daemon on port {port}")

    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect((IP_address, port))

        buf=send_command_to_demon(command)
        header = buf.size.to_bytes(4, byteorder='little')
        client.sendall(header + buf.buffer)

        start = time.perf_counter()

        size_bytes = client.recv(4)
        size = int.from_bytes(size_bytes, byteorder='little')
        res = client.recv(int(size))

        while(receive_message_from_demon(res,size)!=Event.PROCESS_TERMINATED):
            if(receive_message_from_demon(res,size)!=Event.PROCESS_LAUNCHED):
                size_bytes = client.recv(4)
                size = int.from_bytes(size_bytes, byteorder='little')
                res = client.recv(int(size))
                pid=receive_processlaunched(res,size)
                print(pid)
                # buf=send_kill_to_demon(pid)
                # header = buf.size.to_bytes(4, byteorder='little')
                # client.sendall(header + buf.buffer)
            size_bytes = client.recv(4)
            size = int.from_bytes(size_bytes, byteorder='little')
            res = client.recv(int(size))

        info = receive_processterminated(res,int(size))

        end = time.perf_counter()
        print(f"Time elapsed : {end-start:.4f}")

        print(f"error_code : {info.error_code}, pid: {info.pid}")
        if info.pid>0 and info.error_code==0 and end-start<2.9:
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
    )
    res = launch_process("127.0.0.1", daemon,command)
    assert res != -1, "Launch process failed"

@pytest.mark.timeout(3)
def test_receiving_failed_launch_process(daemon):
    # Configuration des arguments
    path = "/bin/OIHDOEJ"
    args = ["lss", "-la"]
    envp = ["badenvp"]
    flags = 0
    stack_size = 1024*1024

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
    )
    res = process_terminated("127.0.0.1", daemon,command)
    assert res != -1, "Receiving process_terminated failed"

@pytest.mark.timeout(6)
def test_response_time(daemon):
    path = "/bin/sleep"
    args = ["sleep", "3"]
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
    )
    res = response_time("127.0.0.1", daemon,command)
    assert res != -1, "Response time is not good"

@pytest.mark.timeout(3)
def test_execve_terminated(daemon):
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
    )
    res = execve_executed("127.0.0.1", daemon,command)
    assert res != -1, "Receiving execve_termiated with success failed"

@pytest.mark.timeout(3)
def test_socket_listening(daemon):
    path = ""
    args = []
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack

    process, daemon_type, port = daemon

    scoket_evt = TCPSocket(destport=port)

    surveillance = Surveillance(
        event=SurveillanceEventType.WATCH_SOCKET,
        ptr_event=scoket_evt
    )
    to_watch = [surveillance]
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
    )

    res = socket_listening("127.0.0.1", daemon,command)
    assert res != -1, "Socket listening failed"

@pytest.mark.timeout(3)
def test_watching_socket(daemon):
    path = ""
    args = []
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack

    process, daemon_type, port = daemon

    scoket_evt = TCPSocket(destport=port)

    surveillance = Surveillance(
        event=SurveillanceEventType.WATCH_SOCKET,
        ptr_event=scoket_evt
    )
    to_watch = [surveillance]
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
    )

    res = watching_socket("127.0.0.1", daemon,command)
    assert res != -1, "Reception of watching socket failed"



@pytest.mark.timeout(3)
def test_inotify_result(daemon):
    path = "/bin/echo"
    args = ["echo", "bonjour"]
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack

    # Création des paramètres inotify
    inotify_params = InotifyParameters(
        root_paths="./test.txt",
        mask=IN_MODIFY | IN_CREATE,
        size=4096
    )

    # Création de l'événement de surveillance pour ces paramètres
    surveillance_inotify = Surveillance(
        event=SurveillanceEventType.INOTIFY,
        ptr_event=inotify_params
    )

    to_watch = [surveillance_inotify]
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
    )
    res = inotify_watchlist_updated("127.0.0.1", daemon,command)
    assert res != -1, "Inotify watch list not updated"

IN_MODIFY = 0x00000002
IN_CREATE = 0x00000100
IN_ACCESS = 0x00000001

@pytest.mark.timeout(3)
def test_inotify_event(daemon):
    path = "/bin/echo"
    args = ["echo", "bonjour"]
    envp = []
    flags = 0
    stack_size = 1024 * 1024  # 1 MB de stack

    # Création des paramètres inotify
    inotify_params = InotifyParameters(
        root_paths="/bin/",
        mask=IN_MODIFY | IN_CREATE | IN_ACCESS,
        size=128
    )

    # Création de l'événement de surveillance pour ces paramètres
    surveillance_inotify = Surveillance(
        event=SurveillanceEventType.INOTIFY,
        ptr_event=inotify_params
    )

    to_watch = [surveillance_inotify]
    command = Command(
        path=path,
        args=args,
        envp=envp,
        flags=flags,
        stack_size=stack_size,
        to_watch=to_watch,
    )
    res = inotify_event("127.0.0.1", daemon,command)
    assert res != -1, "Inotify path not updated"

# @pytest.mark.timeout(3)
# def test_kill_process(daemon):
#     path = "/bin/sleep"
#     args = ["sleep", "3"]
#     envp = []
#     flags = 0
#     stack_size = 1024 * 1024  # 1 MB de stack
#     to_watch = []
#     command = Command(
#         path=path,
#         args=args,
#         envp=envp,
#         flags=flags,
#         stack_size=stack_size,
#         to_watch=to_watch,
#     )

#     res = kill_process("127.0.0.1", daemon,command)
#     assert res != -1, "Response time is not good"