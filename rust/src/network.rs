use std::io::Read;
use std::io::Write;
use std::net::TcpListener;
use std::net::TcpStream;
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};

use colored::*;

/*
    Gestion des communications réseaux

    1) Serveur TCP pour recevoir les commandes
    2) Client capable d'envoyer des comandes a d'autre démons (??? je suis pas sur)
*/

/*
    Configurer le serveur réseau pour qu'il écoute sur un port TCP et exécute les commandes
*/
pub fn start_server(running: Arc<AtomicBool>) -> Result<(), ()> {
    let port = "8080";
    let listener = TcpListener::bind("127.0.0.1:".to_owned()+port)
        .unwrap();
    println!("Server listening on port {}", port);

    for stream in listener.incoming() {
        if !running.load(Ordering::SeqCst) {
            break;
        }

        match stream {
            Ok(mut stream) => {
                println!("Client connecté: {:?}", stream.peer_addr()); // retourne l'adresse du socket du client lors de la connexion TCP
                let mut buffer = [0; 512];

                match stream.read(&mut buffer) {
                    // Réponse de notre serveur
                    Ok(size) => {
                        let received = String::from_utf8_lossy(&buffer[..size]);
                        println!("Message reçu du client dans {} : {:?}", "start_serveur".blue(), received);

                        // Répondre au client
                        let response = "reponse attendu";
                        stream.write_all(response.as_bytes())
                            .expect("Erreur d'envoi de la réponse");
                    },
                    Err(_) => {
                        eprintln!("Erreur du lecture du stream");
                    },
                }
            }
            Err(e) => {
                eprintln!("Connexion échouée: {}", e);
            }
        }
    }

    Ok(())
}

/*
    Gestion du client des commandes sur le stream TCP
*/
#[allow(dead_code)]
pub fn handle_client(mut stream: TcpStream) {
    let mut buffer = [0; 512]; // Création de buffer pour le stream TCP
    let mess = stream.read(&mut buffer);

    match mess {
        Ok(_) => todo!(),
        Err(_) => todo!(),
    } 
}