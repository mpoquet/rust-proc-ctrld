use inotify::EventMask;
use nix::errno::Errno;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use std::error::Error;
use tokio::net::TcpListener;


// monitoring tools
use rust_proc_ctrl::monitoring_tools::inotify_tool::read_events_inotify;
use rust_proc_ctrl::monitoring_tools::network::read_events_port_tokio;
use rust_proc_ctrl::monitoring_tools::signal_tool::send_sigkill;
use rust_proc_ctrl::monitoring_tools::command::exec_command;

// flatbuffers
use rust_proc_ctrl::proto::demon_generated::demon::{root_as_message,Event, InotifyEvent};

// sérialisation
use rust_proc_ctrl::proto::serialisation::{serialize_child_creation_error, serialize_process_launched, serialize_process_terminated};

async fn handle_message(buf: &[u8]) -> Vec<u8> {

    let msg = root_as_message(buf).expect("error root_as_message()");

    match msg.events_type() {
        Event::InotifyPathUpdated => {
            let from_message =
                msg.events_as_inotify_path_updated().expect("error events_as_inotify...");
            let path = from_message.path().expect("No path from the message");
            let trigger_events = from_message.trigger_events().expect("No vector of trigger events for inotify");
            let reach_size = from_message.size();

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

            match read_events_inotify(path, trig_events, reach_size as u64).await {
                Ok(pid) => serialize_process_launched(pid as i32),
                Err((pid,errno)) => serialize_process_terminated(pid as i32, errno as u32),
            }
        }
        Event::KillProcess => {
            let from_mess = msg.events_as_kill_process().expect("error events_as_kill_process");
            let pid_kill = from_mess.pid();

            match send_sigkill(pid_kill) {
                Ok(pid) => serialize_process_launched(pid as i32),
                Err((pid,errno)) => serialize_process_terminated(pid as i32, errno as u32),
            }
        }
        Event::RunCommand => {
            let from_mess = msg.events_as_run_command().expect("error events_as_run_command");

            let res = exec_command(&from_mess).await;
            
            match res {
                Ok(pid) => serialize_process_launched(pid as i32),
                Err((pid,errno)) => serialize_process_terminated(pid as i32, errno as u32),
            }
        }
        Event::TCPSocketListening => {
            let from_mess = msg.events_as_tcpsocket_listening().expect("error from receiving message");
            let port = from_mess.port();

            match read_events_port_tokio(port).await {
                Ok(pid) => serialize_process_launched(pid as i32),
                Err((pid ,errno)) => serialize_process_terminated(pid as i32, errno as u32),
            }
        }
        _ => {
            serialize_child_creation_error(Errno::ENODATA as u32)
        }
    }
}

#[tokio::main]
pub async fn main() -> Result<(), Box<dyn Error>> {
    let port: u16 = 8080;
    println!("We listen on the port {}", port);

    let listener = TcpListener::bind(format!("127.0.0.1:{}", port)).await?;

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
                
                //Traitement + obtention du retour
                let mut retour = handle_message(&buf).await;

                //Envoi sur le réseau
                let _ = socket.write(&mut retour).await;
                
            }
        });
    }
}