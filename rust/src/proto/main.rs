use std::env;
use tokio::io::AsyncReadExt;
use std::error::Error;


use crate::proto::demon_generated::demon::{root_as_message, Event};

use tokio::net::TcpListener;

fn handle_message(buf: &[u8]) {
    let msg = root_as_message(buf).expect("error root_as_message");

    match msg.events_type() {
        Event::child_creation_error => {
            println!("Handling child creation error");
        }
        Event::establish_tcp_connection => {
            println!("Handling EventType2");
        }
        Event::establish_unix_connection => {
            println!("Handling EventType2");
        }
        Event::inotify_path_updated => {
            println!("Handling EventType2");
        }
        Event::kill_process => {
            println!("Handling EventType2");
        }
        Event::process_launched => {
            println!("Handling EventType2");
        }
        Event::process_terminated => {
            println!("Handling EventType2");
        }
        Event::run_command => {
            println!("Handling EventType2");
        }
        Event::tcp_socket_listening => {
            println!("Handling EventType2");
        }
        _ => {
            println!("Unknown event type");
        }
    }
}

#[tokio::main]
pub async fn main() -> Result<(), Box<dyn Error>> {
    let mut args = env::args();
    if args.len() != 2 {
        println!("Usage : {:?} <PortId>", args.next())
    }

    let port: u16 = args.nth(1)
        .expect("error getting args")
        .parse()
        .expect("invalid port number");

    println!("We listen on the port {}", port);

    let listener = TcpListener::bind(format!("127.0.0.1::{}", port)).await?;

    loop {
        let (mut socket, addr) = listener.accept().await?;
        println!("Connexion de : {}", addr);

        tokio::spawn(async move {
            loop {
                // Lire la taille (4 octets)
                let mut len_buf = [0u8; 4];
                if socket.read_exact(&mut len_buf).await.is_err() {
                    println!("Client déconnecté");
                    break;
                }

                let msg_len = u32::from_le_bytes(len_buf) as usize;

                // Lire le message flatbuffer complet
                let mut buf = vec![0u8; msg_len];
                if socket.read_exact(&mut buf).await.is_err() {
                    println!("Erreur de lecture message");
                    break;
                }

                handle_message(&buf);
            }
        });
    }
}