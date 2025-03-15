use std::io::Read;
use std::net::TcpListener;
use std::net::TcpStream;
use std::sync::{Arc, atomic::{AtomicBool, Ordering}};

/*
    Gestion des communications réseaux

    1) Serveur TCP pour recevoir les commandes
    2) Client capable d'envoyer des comandes a d'autre démons (??? je suis pas sur)
*/

/*
    Configurer le serveur réseau pour qu'il écoute sur un port TCP et exécute les commandes
*/
pub fn start_server(running: Arc<AtomicBool>) -> Result<(), ()> {
    let listener = TcpListener::bind("127.0.0.1:80").unwrap();
    println!("Server listening on port 80");

    for stream in listener.incoming() {
        if !running.load(Ordering::SeqCst) {
            break;
        }

        match stream {
            Ok(stream) => {
                handle_client(stream);
            }
            Err(e) => {
                eprintln!("Connection failed: {}", e);
            }
        }
    }

    Ok(())
}

/*
    Gestion du client des commandes sur le stream TCP
*/
pub fn handle_client(mut stream: TcpStream) {
    let mut buffer = [0; 512]; // Création de buffer pour le stream TCP
    let mess = stream.read(&mut buffer);

    match mess {
        Ok(_) => todo!(),
        Err(_) => todo!(),
    } 
}