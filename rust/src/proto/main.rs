use std::env;
use inotify::{EventMask, Inotify};
use tokio::io::AsyncReadExt;
use std::error::Error;

// monitoring tools
use crate::monitoring_tools::inotify_tool::read_events_inotify;
use crate::monitoring_tools::network::read_events_port_tokio;
use crate::monitoring_tools::signal_tool::{block_all_signals, send_sigkill};

// flatbuffers
use crate::proto::demon_generated::demon::{root_as_message, Event, InotifyEvent};

use tokio::net::TcpListener;

async fn handle_message(buf: &[u8]) {
    let msg = root_as_message(buf).expect("error root_as_message");

    match msg.events_type() {
        Event::EstablishTCPConnection => {
            // TODO
        }
        Event::EstablishUnixConnection => {
            // TODO
        }
        Event::InotifyPathUpdated => {
            let reach_size = 400;                           // TODO
            let from_message =
                msg.events_as_inotify_path_updated().expect("error events_as_inotify...");
            let path = from_message.path().expect("No path from the message");
            let trigger_events = from_message.trigger_events().expect("No vector of trigger events for inotify");

            // new vector of "real" Inotify Event not from flatbuffer
            let mut trig_events = Vec::<EventMask>::new();

            for event in trigger_events {
                match event {
                    InotifyEvent::access => trig_events.push(EventMask::ACCESS),
                    InotifyEvent::creation => {
                        trig_events.push(EventMask::CREATE);
                        trig_events.push(EventMask::ISDIR);                        
                    }
                    InotifyEvent::deletion => trig_events.push(EventMask::DELETE),
                    InotifyEvent::modification => trig_events.push(EventMask::MODIFY),
                    // when we want to know the size of a file, we watch when the file is Close
                    InotifyEvent::size => {
                        trig_events.push(EventMask::CLOSE_NOWRITE);
                        trig_events.push(EventMask::CLOSE_WRITE);
                    }
                    _ => {
                        eprintln!("flags error for inotify events");
                    }
                }
            }

            read_events_inotify(path, trig_events, reach_size).await.expect("error reading inotify events");
        }
        Event::KillProcess => {
            let from_mess = msg.events_as_kill_process().expect("error events_as_kill_process");
            let pid = from_mess.pid();

            send_sigkill(pid).expect("error while sending SIGKILL");
        }
        Event::ProcessLaunched => {
            // TODO
        }
        Event::ProcessTerminated => {
            // TODO
        }
        Event::RunCommand => {
            // TODO
        }
        Event::TCPSocketListening => {
            println!("Handling EventType2");
            let from_mess = msg.events_as_tcpsocket_listening().expect("error from receiving message");
            let port = from_mess.port();

            read_events_port_tokio(port).await.expect("error read_events_port");
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

                handle_message(&buf).await;
            }
        });
    }
}