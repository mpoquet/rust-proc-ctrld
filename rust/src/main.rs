use prototype::*;

mod prototype;
mod network;
mod process;

/*
    Point d'entrée, initialisation du démon

    1) Détacher le processus pour qu'il devienne un démon
    2) Congfigurer le serveur réseau pour écouter les commandes
    3) Initialiser les gestionnaires de signaux et de logs
*/

fn main() {
    prototype();
}
