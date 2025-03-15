mod prototype;
mod network;
mod process;

use std::sync::{atomic::AtomicBool, Arc};

use process::daemonize;
use network::start_server;

/*
    Point d'entrée, initialisation du démon

    1) Détacher le processus pour qu'il devienne un démon
    2) Congfigurer le serveur réseau pour écouter les commandes
    3) Initialiser les gestionnaires de signaux et de logs
*/

fn main() {
    let running = Arc::new(AtomicBool::new(true));

    daemonize().expect("error main() : function daemonize error.");
    start_server(running).expect("error main() : function start_server error.");
}
