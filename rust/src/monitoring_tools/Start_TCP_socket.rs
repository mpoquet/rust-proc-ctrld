mod inotify;

use nix::unistd::Pid;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use std::time::Duration;
use std::net::TcpListener;

#[tokio::main]
async fn main() -> std::io::Result<()> {
    println!("Je suis le process {} et je démarre le serveur.", Pid::this());
    // Création synchrone du socket
    let std_listener = std::net::TcpListener::bind("127.0.0.1:46550")?;
    std_listener.set_nonblocking(true)?;
    let listener = tokio::net::TcpListener::from_std(std_listener)?;
    
    println!("Serveur en écoute sur 127.0.0.1:46550");

    loop {
        // Accepter une nouvelle connexion entrante
        let (mut socket, addr) = listener.accept().await?;
        println!("Nouvelle connexion de {}", addr);

        // Lancer une tâche pour gérer la connexion sans bloquer le serveur
        tokio::spawn(async move {
            let mut buf = [0u8; 1024];
            loop {
                match socket.read(&mut buf).await {
                    Ok(0) => {
                        // La lecture retourne 0 lorsque la connexion est fermée par le client
                        println!("Fermeture de la connexion de {}", addr);
                        return;
                    }
                    Ok(n) => {
                        // Afficher les données reçues (pour le débogage, par exemple)
                        println!("Reçu {} octets de {}: {:?}", n, addr, &buf[..n]);

                        println!("on fait un echo");
                        // Ici on fait un echo : on renvoie les données reçues
                        if let Err(e) = socket.write_all(&buf[..n]).await {
                            eprintln!("Erreur d'écriture sur {}: {}", addr, e);
                            return;
                        }
                        tokio::time::sleep(Duration::from_secs(2)).await;
                        println!("Fermeture de la connexion de {}", addr);
                        return;
                    }
                    Err(e) => {
                        eprintln!("Erreur de lecture sur {}: {}", addr, e);
                        return;
                    }
                }
            }
        });
    }
}
