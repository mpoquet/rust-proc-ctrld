import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from scipy import stats
import pyexcel



# Configuration pour un style plus propre
sns.set(style="whitegrid")
plt.rcParams.update({'font.size': 12})

# Charger les données depuis le fichier ODS
ods_file_path = '/content/sample_data/performance_data.ods' # Remplacez par le chemin réel de votre fichier ODS

try:
    # Lire le fichier ODS
    book = pyexcel.get_book(file_name=ods_file_path)

    # Supposons que les données sont dans la première feuille (index 0)
    # Vous pouvez changer l'index si vos données sont dans une autre feuille
    sheet = book[0]

    # Convertir la feuille en liste de listes, en retirant l'en-tête pour le moment
    data = sheet.array

    # Utiliser la première ligne comme en-têtes
    headers = data[0]
    data_rows = data[1:]

    # Créer le DataFrame
    df = pd.DataFrame(data_rows, columns=headers)

     # Convertir la colonne temps_performance en numérique si nécessaire
    # Assurez-vous que le nom de la colonne est correct
    df['temps_performance'] = pd.to_numeric(df['temps_performance'], errors='coerce')

    # Supprimer les lignes où la conversion numérique a échoué (si il y en a)
    df.dropna(subset=['temps_performance'], inplace=True)


except FileNotFoundError:
    print(f"Erreur : Le fichier ODS '{ods_file_path}' est introuvable.")
    # Vous pourriez vouloir sortir ici ou charger des données par défaut
    exit()
except ImportError:
    print("Erreur : La bibliothèque 'pyexcel-ods3' (ou similaire) n'est pas installée.")
    print("Veuillez l'installer en exécutant : !pip install pyexcel-ods3")
    exit()
except Exception as e:
    print(f"Une erreur est survenue lors de la lecture du fichier ODS : {e}")
    exit()

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
