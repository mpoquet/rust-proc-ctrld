from serialization import *
from test_demon import launch_process

import subprocess
import time
import os
import signal
import socket
import pandas as pd
import numpy as np
#import seaborn as sns
#import matplotlib.pyplot as plt
#from scipy import stats

RUST_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../rust"))
C_PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../c/builddir"))

nb_mesures = 1
nb_iteration = 1

temps_sans_demon = []
temps_demon_rust = []
temps_demon_c = []
temps_ssh = []

# Configuration des arguments
path = "/bin/echo"
args = ["echo", "bonjour"]
envp = []
flags = 0
stack_size = 1024 * 1024  # 1 MB de stack
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



#Mesure sans demon
for i in range(nb_mesures):
    t_start = time.perf_counter()

    for j in range(nb_iteration):
        subprocess.call(["echo", ""] , stdout=subprocess.DEVNULL)

    t_stop = time.perf_counter()

    difference = t_stop - t_start

    temps_sans_demon.append(difference)


print(f"Moyenne de temps sans demon : {np.mean(temps_sans_demon)} secondes")


#Demon rust

#Lancement demon rust
process_rust = subprocess.Popen(
            ["cargo", "run", "--bin", "run_demon", "--", str(8080)],
            cwd=RUST_PROJECT_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid
        )

demon_rust = (process_rust, "rust", 8080)

#Mesure
for i in range(nb_mesures):
    t_start = time.perf_counter()

    for j in range(nb_iteration):
        res = launch_process("127.0.0.1", demon_rust,command)

    t_stop = time.perf_counter()

    difference = t_stop - t_start

    temps_demon_rust.append(difference)

#Terminer demon rust
os.killpg(os.getpgid(process_rust.pid), signal.SIGTERM)
process_rust.wait(timeout=5)

print(f"Moyenne de temps demon rust: {np.mean(temps_demon_rust)} secondes")


#Demon c

#Lancement demon c
process_c = subprocess.Popen(
            ["./daemon", str(9090)],
            cwd=C_PROJECT_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid
        )

demon_c = (process_c, "c", 9090)

#Mesure
for i in range(nb_mesures):
    t_start = time.perf_counter()

    for j in range(nb_iteration):
        res = launch_process("127.0.0.1", demon_c,command)

    t_stop = time.perf_counter()

    difference = t_stop - t_start

    temps_demon_c.append(difference)

#Terminer demon c
os.killpg(os.getpgid(process_c.pid), signal.SIGTERM)
process_c.wait(timeout=5)

print(f"Moyenne de temps demon rust: {np.mean(temps_demon_c)} secondes")


#Mesure sshd
for i in range(nb_mesures):
    t_start = time.perf_counter()

    for j in range(nb_iteration):
        subprocess.call(["sshd", "user@localhost", "\"echo \'bonjour\' \""] , stdout=subprocess.DEVNULL)

    t_stop = time.perf_counter()

    difference = t_stop - t_start

    temps_ssh.append(difference)


print(f"Moyenne de temps sshd : {np.mean(temps_ssh)} secondes")



#Initialisation du dataframe
data = {'Sans_demon':temps_sans_demon,
        'Demon_rust':temps_demon_rust,
        'Demon_c':temps_demon_c,
        'SSH_b':temps_ssh}

df = pd.DataFrame(data)

'''
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

'''