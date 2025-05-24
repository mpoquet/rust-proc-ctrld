import pandas as pd
import numpy as np
import seaborn as sns
import matplotlib.pyplot as plt
from scipy import stats

# 1. Lecture du fichier CSV
# IMPORTANT: Make sure your 'donnees.csv' file ONLY contains the numeric data,
# with commas as delimiters, and NO header row like ", Sans demon,Demon rust,Demon c"
column_names = ["Sans_demon", "Demon_rust", "Demon_c"]
df_raw = pd.read_csv('donnees.csv', header=None, names=column_names)

# 2. Transformation du DataFrame du format "wide" au format "long"
df = pd.melt(df_raw, var_name='variante', value_name='temps_performance')

# Ensure the 'temps_performance' column is numeric
# This is a crucial step to handle potential mixed types after reading
df['temps_performance'] = pd.to_numeric(df['temps_performance'], errors='coerce')

# Drop any rows where 'temps_performance' could not be converted (i.e., became NaN)
df.dropna(subset=['temps_performance'], inplace=True)


# Afficher les premières lignes pour vérifier les données après transformation
print("Aperçu des données transformées :")
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
plt.ylabel("Temps de performance (s)", fontsize=14, labelpad=10)
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Annoter les moyennes sur les barres
for i, variante_name in enumerate(df['variante'].unique()):
    mean_val = stats_df.loc[variante_name, 'mean']
    # Adjust annotation position dynamically based on max value in the current bar
    # This ensures the text is always above the bar and visible
    y_pos = mean_val + (df['temps_performance'].max() * 0.01) # A small offset above the bar
    plt.text(
        i,
        y_pos,
        f"{mean_val:.6f}", # Display more decimal places for precision
        ha='center',
        fontsize=10,
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
print("\nStatistiques détaillées par variante :")
for variante in df['variante'].unique():
    subset = df[df['variante'] == variante]['temps_performance']
    print(f"\n{variante}:")
    print(f"  Moyenne: {subset.mean():.6f} ms") # More decimal places for precision
    print(f"  Écart-type: {subset.std():.6f} ms")
    if variante in stats_df.index: # Check if variant exists in stats_df before accessing
        ci_lower = subset.mean() - stats_df.loc[variante, 'ci_95']
        ci_upper = subset.mean() + stats_df.loc[variante, 'ci_95']
        print(f"  Intervalle de confiance à 95%: [{ci_lower:.6f}, {ci_upper:.6f}]")
    print(f"  Min: {subset.min():.6f} ms")
    print(f"  Max: {subset.max():.6f} ms")