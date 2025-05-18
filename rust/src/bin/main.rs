use inotify::EventMask;
use nix::errno::Errno;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use std::error::Error;
use tokio::net::{TcpListener, TcpStream};

// monitoring tools
use rust_proc_ctrl::monitoring_tools::inotify_tool::read_events_inotify;
use rust_proc_ctrl::monitoring_tools::network::read_events_port_tokio;
use rust_proc_ctrl::monitoring_tools::signal_tool::send_sigkill;
use rust_proc_ctrl::monitoring_tools::command::exec_command;

// flatbuffers
use rust_proc_ctrl::proto::demon_generated::demon::{root_as_message, Event, SocketState, Surveillance, SurveillanceEvent};

// sérialisation
use rust_proc_ctrl::proto::serialisation::{serialize_child_creation_error, serialize_execve_terminated, serialize_process_launched, serialize_process_terminated, serialize_socket_watch_terminated, serialize_tcp_socket_listenning};

async fn handle_surveillance_event(msg: &SurveillanceEvent<'_>, socket: &mut TcpStream) {
    match msg.event_type() {
        Surveillance::Inotify => {
            let inotify_evt = msg.event_as_inotify().expect("error: event_as_inotify");
            let path = inotify_evt.root_paths().unwrap_or("");
            let mask = inotify_evt.mask() as u32;
            let reach_size = inotify_evt.size();

            let mut trig_events = Vec::<EventMask>::new();

            // Mapping bit flags (à adapter à ton système d'encodage)
            if mask & EventMask::ACCESS.bits() != 0 {
                trig_events.push(EventMask::ACCESS);
            }
            if mask & EventMask::CREATE.bits() != 0 {
                trig_events.push(EventMask::CREATE);
                trig_events.push(EventMask::ISDIR);
            }
            if mask & EventMask::DELETE.bits() != 0 {
                trig_events.push(EventMask::DELETE);
            }
            if mask & EventMask::MODIFY.bits() != 0 {
                trig_events.push(EventMask::MODIFY);
            }
            if mask & (EventMask::CLOSE_NOWRITE.bits() | EventMask::CLOSE_WRITE.bits()) != 0 {
                trig_events.push(EventMask::CLOSE_NOWRITE);
                trig_events.push(EventMask::CLOSE_WRITE);
            }

            match read_events_inotify(path, trig_events, reach_size as u64).await {
                Ok(pid) => send_on_socket(serialize_process_terminated(pid as i32, 0), socket).await,
                Err((pid, errno)) => {
                    send_on_socket(serialize_process_terminated(pid as i32, errno as u32), socket).await
                }
            }
        }
        Surveillance::TCPSocket => {
            let from_mess = msg.event_as_tcpsocket().expect("error from receiving message");
            let port = from_mess.destport();

            match read_events_port_tokio(port as u16).await {
                Ok(state) => {
                    let to_send = match state {
                        netstat2::TcpState::Listen => SocketState::listeing,
                        netstat2::TcpState::Established => SocketState::created,
                        _ => SocketState::unknown,
                    };
                    send_on_socket(serialize_socket_watch_terminated(port as i32, to_send), socket).await
                }
                Err((pid ,errno)) => send_on_socket(serialize_process_terminated(pid as i32, errno as u32), socket).await,
            }
        }
        _ => {
            panic!("Wrong 'Surveillance' type");
        }
    } // end match
}

async fn handle_message(buf: &[u8], socket: &mut TcpStream) {

    let msg = root_as_message(buf).expect("error root_as_message()");

    match msg.events_type() {
        Event::KillProcess => {
            let from_mess = msg.events_as_kill_process().expect("error events_as_kill_process");
            let pid_kill = from_mess.pid();

            match send_sigkill(pid_kill) {
                Ok(pid) => send_on_socket(serialize_process_launched(pid as i32), socket).await,
                Err((pid,errno)) => send_on_socket(serialize_process_terminated(pid as i32, errno as u32), socket).await,
            }
        }
        Event::RunCommand => {
            let from_mess = msg.events_as_run_command().expect("error events_as_run_command");
            let mut command = exec_command(&from_mess);
            let command_exec = from_mess.path().expect("error getting path from serialize message").to_string();
            let to_watch = from_mess.to_watch().expect("error getting to watch");

            if !to_watch.is_empty() {
                for event in to_watch {
                    handle_surveillance_event(&event, socket).await;
                }
                return;
            }

            let mut child = match command.spawn() {
                Ok(child) => child,
                Err(error_code) => {
                    let pid = std::process::id() as i32;
                    send_on_socket(serialize_process_launched(pid), socket).await;
                    match error_code.kind() {
                        std::io::ErrorKind::NotFound => {
                            let msg = serialize_execve_terminated(pid, command_exec, false);
                            send_on_socket(msg, socket).await;
                        },
                        _ => {
                            let msg = serialize_child_creation_error(error_code.raw_os_error().unwrap_or(1) as u32);
                            send_on_socket(msg, socket).await;
                        }
                    }
                    send_on_socket(serialize_process_terminated(pid, error_code.raw_os_error().unwrap_or(1) as u32), socket).await;
                    return;
                }
            };
            let pid = child.id().expect("could not get child pid");
            send_on_socket(serialize_process_launched(pid as i32), socket).await;
            send_on_socket(serialize_execve_terminated(pid as i32, command_exec, true), socket).await;
            
            let res = child.wait().await;
            match res {
                Ok(ret_code) => send_on_socket(serialize_process_terminated(pid as i32, ret_code.code().unwrap_or(1) as u32), socket).await,
                Err(errno) => send_on_socket(serialize_process_terminated(pid as i32, errno.raw_os_error().unwrap_or(1) as u32), socket).await,
            }
        }
        _ => {
            send_on_socket(serialize_child_creation_error(Errno::ENODATA as u32), socket).await
        }
    }
}

async fn send_on_socket(retour: Vec<u8>, socket: &mut TcpStream) {
    let data_len = retour.len() as u32;
    let mut resp = data_len.to_le_bytes().to_vec();
    resp.extend(retour);

    let _ = socket.write(&resp).await;
}

#[tokio::main]
pub async fn main() -> Result<(), Box<dyn Error>> {
    let args : Vec<String> = std::env::args().collect();
    if args.len() != 2 {
        println!("Usage : {} <port>.", args[0]);
        return Err(Box::<dyn Error>::from(std::io::Error::new(std::io::ErrorKind::InvalidInput, "Invalid arguments")))
    }

    let port : u16 = args[1].parse::<u16>().expect("Invalid port number");
    println!("The demon pid is {}", std::process::id());
    println!("We listen on the port {}", port);

    let listener = TcpListener::bind(format!("127.0.0.1:{}", port)).await?;

    loop {
        let (mut socket, addr) = listener.accept().await?;
        println!("Connexion de : {}", addr);
        let established_connection = serialize_tcp_socket_listenning(port);
        send_on_socket(established_connection, &mut socket).await;

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
                handle_message(&buf, &mut socket).await;  
            }
        });
    }
}