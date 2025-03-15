use std::net::TcpListener;
use std::net::TcpStream;

/*
    Gestion des communications réseaux

    1) Serveur TCP pour recevoir les commandes
    2) Client capable d'envoyer des comandes a d'autre démons (??? je suis pas sur)
*/

/*
    Configurer le serveur réseau pour qu'il écoute sur un port TCP et exécute les commandes
*/
fn start_server() -> Result<(), ()> {
    let listener = TcpListener::bind("127.0.0.1:80")
        .unwrap();
    println!("Server listening on port 80");

    for stream in listener.incoming() {
        // TODO handle_client() ce que fais notre client (-> ici qu'il serait intêressant d'utiliser l'exécution des commandes ? => poll)
        match stream {
            Ok(stream) => {
                handle_client(stream);
            },
            Err(e) => {
                eprintln!("Connection failed: {}", e);
                return Err(());
            },
        }
    }

    Ok(())
}

#[allow(unused_variables)]
fn handle_client(stream: TcpStream) {
    ()
}