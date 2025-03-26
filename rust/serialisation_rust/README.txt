Ce prototype est un test pour sérialiser et désérialiser une erreur typée (code i16, contexte string) depuis un fichier.

Le fonctionnement est le suivant :
-L'utilisateur renseigne le nom du fichier, le code d'erreur et le contexte
-Le programme sérialise l'erreur
-La sérialisation est écrite dans un fichier nouvellement crée
-Le programme lit le contenue du fichier
-Le programme désérialise l'erreur et l'affiche

La commande pour l'executer est : 
cargo run [nom_du_fichier] [code] [contexte]

avec :
nom_du_fichier : fichier qui va être crée pour contenir l'erreur sérialisée
code : code de l'erreur
contexte : contexte de l'erreur d'execution (doit tenir en une chaine de caractères)

exemple:
cargo run fichier_test 52 Je_Suis_Un_Contexte
