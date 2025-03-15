use std::{io::{self, Write}, process::Command};

/*
    V1
    Fonction qui agit localement comme un "petit terminal"
    stop pour arrêter le processus 
*/
#[allow(dead_code)]
pub fn prototype() {

    println!("Lancement du Démon de Contrôle");
    loop {
        let mut input = String::new();
        
        print!("-> ");
        io::stdout().flush().expect("Erreur lors du flush");

        io::stdin()
            .read_line(&mut input)
            .expect("Échec de la lecture");

        
        if input == "stop\n" { // Arrêt du démon de contrôle
            println!("Commande de terminaison.");
            break;
        }
        
        // exécute la commande (si possible)
        let mut parts = input.split_whitespace();
        if let Some(cmd) = parts.next() {
            let args: Vec<&str> = parts.collect();

            let status = Command::new(cmd)
                .args(&args)
                .spawn(); // créer un processus - DOIT ON UTILISER ÇA OU CLONE

            match status {
                Ok(mut child) => {
                    child.wait().expect("Erreur lors de l'attente du processus");
                }
                Err(e) => {
                    println!("Erreur: Impossible d'exécuter '{}': {}", input, e);
                }
            }
        }
    }

    println!("Démon de contrôle ce termine.");
}