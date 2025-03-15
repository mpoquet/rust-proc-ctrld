use std::{io::{Read, Write}, net::TcpStream, sync::{atomic::{AtomicBool, Ordering}, Arc}};
use std::thread;

use rust::network::start_server;

#[test]
fn test_server_start_stop() {
    let addr = "127.0.0.1:8080";
    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();

    // Création du serveur TCP au port associé
    let server_thread = thread::spawn(move || {
        start_server(r).unwrap();
    });

    // Simule un petit délai pour s'assurer que le serveur est prêt
    thread::sleep(std::time::Duration::from_secs(1));

    // Envoi d'un stream sur le serveur TCP (joue le rôle du client)
    let client_thread = thread::spawn(move || {

        match TcpStream::connect(addr) {
            Ok(mut stream) => {

                // Écrit un message au serveur
                let mess = "start\n";
                stream.write_all(mess.as_bytes())
                    .expect("erreur lors de l'envoi du message au réseau");
                stream.flush()
                    .expect("erreur lors du flush du message");

                // Attend et lit la réponse du serveur
                let mut buffer = [0; 512];
                let size = stream.read(&mut buffer)
                    .expect("erreur lors de la lecture du message au réseau");
                let reponse = String::from_utf8_lossy(&buffer[..size]);

                // Vérifie le résultat
                assert!(reponse.contains("reponse attendu"), "Réponse innatendu: {}", reponse);
            },
            Err(_) => {
                panic!("Impossible de se connecter au serveur");
            },
        }
    });

    // Simule un délai pour traiter la commande
    thread::sleep(std::time::Duration::from_secs(1));

    // Arrêter le serveur
    running.store(false, Ordering::SeqCst);

    // Assurer la terminaison des threads
    client_thread.join().expect("Erreur lors de l'exécution du client.");
    server_thread.join().expect("Erreur lors de l'arrêt du serveur.");
}