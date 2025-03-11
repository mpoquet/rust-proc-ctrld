use std::net::TcpListener;

use nix::libc::{STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
use nix::unistd::ForkResult;
use nix::unistd::{fork, setsid, chdir, close};
use prototype::*;

mod prototype;

/*
    Point d'entrée, initialisation du démon

    1) Détacher le processus pour qu'il devienne un démon
    2) Congfigurer le serveur réseau pour écouter les commandes
    3) Initialiser les gestionnaires de signaux et de logs
*/

/*
    Création du démon détaché du processus principal :
    https://rex.plil.fr/Enseignement/Reseau/Reseau.IMA4sc/reseau026.html
        - fork() -> se lance en fond
        - setsid() -> pour se détacher du terminal courant
        - chdir() -> évite d'empêcher un démontage
        - close(0, 1, 2) -> on ferme les descripteurs inutiles
*/
fn daemonize() -> Result<(), ()>{
    match unsafe {fork()} {
        Ok(ForkResult::Parent { .. }) => std::process::exit(0),
        Ok(ForkResult::Child) => {
            setsid().expect("error daemonize() : échec du setsid.");
            chdir("/").expect("error daemonize() : échec du chdir.");

            close(STDIN_FILENO).expect("error daemonize() : échec de close sur STDIN_FILENO");
            close(STDOUT_FILENO).expect("error daemonize() : échec de close sur STDOUT_FILENO");
            close(STDERR_FILENO).expect("error daemonize() : échec de close sur STDERR_FILENO");

            // TODO logique du démon qui attend et écoute les commandes : loop => comme dans le prototype
            // TODO doit être a l'écoute des signaux
        }
        Err(_) => std::process::exit(1),
    }

    Ok(())
}

/*
    Configurer le serveur réseau pour qu'il écoute sur un port TCP et exécute les commandes
*/
fn start_server() -> Result<(), ()> {
    let listener = TcpListener::bind("127.0.0.1:80").unwrap();

    for _ in listener.incoming() {
        // TODO handle_client() ce que fais notre client (-> ici qu'il serait intêressant d'utiliser l'exécution des commandes ? => poll)
    }

    Ok(())
}


fn main() {
    prototype();
}
