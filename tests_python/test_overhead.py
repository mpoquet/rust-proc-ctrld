from serialization import *

import subprocess
import time
import os
import signal
import pytest
import socket
import numpy as np
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
from scipy import stats

RUST_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../rust"))
C_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../c/builddir"))

DAEMON_PORTS = {
    "rust": 8080,
    "c": 9090
}

nb_mesures = 2
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
@pytest.mark.timeout(5)
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



@pytest.mark.timeout(5)
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




@pytest.mark.timeout(5)
def test_traitement_donne():
    print("\n")
    print(f"Moyenne de temps sans demon : {np.mean(temps_sans_demon)} secondes")
    print(f"Moyenne de temps avec demon rust: {np.mean(temps_demon_rust)} secondes")
    print(f"Moyenne de temps avec demon c : {np.mean(temps_demon_c)} secondes")
    print(f"{len(temps_sans_demon)} {len(temps_demon_rust)} {len(temps_demon_c)}")

    #Initialisation du dataframe
    data = {'Sans_demon':temps_sans_demon,
            'Demon_rust':temps_demon_rust,
            'Demon_c':temps_demon_c}

    df = pd.DataFrame(data)


    # Afficher les premières lignes pour vérifier les données
    print("Aperçu des données :")
    print(df.head())

    # Statistiques descriptives par variante
    stats_df = df.groupby('variante')['temps_performance'].agg(['mean', 'std', 'count'])
    stats_df['se'] = stats_df['std'] / np.sqrt(stats_df['count'])
    stats_df['ci_95'] = stats_df['se'] * stats.t.ppf(0.975, stats_df['count'] - 1)
    print("\nStatistiques par variante :")
    print(stats_df)

    # Création de la figure et des axes
    plt.figure(figsize=(12, 8))

    # Barplot avec intervalle de confiance à 95%
    barplot = sns.barplot(
        x="variante", 
        y="temps_performance", 
        data=df, 
        estimator=np.mean, 
        errorbar=("ci", 95),
        alpha=0.8,
        palette="viridis"
    )

    # Ajout des points individuels (scatterplot)
    sns.stripplot(
        x="variante", 
        y="temps_performance", 
        data=df,
        jitter=True, 
        size=5, 
        alpha=0.3,
        palette="viridis",
        edgecolor='gray',
        linewidth=0.5
    )

    # Personnalisation du graphique
    plt.title("Temps de performance par variante", fontsize=16, pad=20)
    plt.xlabel("Variante", fontsize=14, labelpad=10)
    plt.ylabel("Temps de performance (ms)", fontsize=14, labelpad=10)
    plt.grid(axis='y', linestyle='--', alpha=0.7)

    # Annoter les moyennes sur les barres
    for i, (index, row) in enumerate(stats_df.iterrows()):
        plt.text(
            i, 
            row['mean'] + 2, 
            f"{row['mean']:.1f}",
            ha='center', 
            fontsize=12, 
            fontweight='bold'
        )

    # Ajustement des limites de l'axe y pour une meilleure visualisation
    y_min = df['temps_performance'].min() * 0.9
    y_max = df['temps_performance'].max() * 1.1
    plt.ylim(y_min, y_max)

    # Ajout d'une légende
    plt.legend(["Moyenne avec IC à 95%", "Observations individuelles"], loc='upper right')

    # Ajout d'une note
    plt.figtext(
        0.5, 
        0.01, 
        "Note: Les barres représentent les moyennes avec intervalle de confiance à 95%. Les points représentent les observations individuelles.",
        ha="center", 
        fontsize=10, 
        style='italic'
    )

    plt.tight_layout()
    plt.savefig('performance_comparison.png', dpi=300)
    plt.show()

    # Afficher les statistiques détaillées par variante sous forme de tableau
    print("\nStatistiques détaillées par variante:")
    for variante in df['variante'].unique():
        subset = df[df['variante'] == variante]['temps_performance']
        print(f"\n{variante}:")
        print(f"  Moyenne: {subset.mean():.2f} ms")
        print(f"  Écart-type: {subset.std():.2f} ms")
        print(f"  Intervalle de confiance à 95%: [{subset.mean() - stats_df.loc[variante, 'ci_95']:.2f}, {subset.mean() + stats_df.loc[variante, 'ci_95']:.2f}]")
        print(f"  Min: {subset.min():.2f} ms")
        print(f"  Max: {subset.max():.2f} ms")
