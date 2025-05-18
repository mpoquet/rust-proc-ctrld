#[allow(unused)]

//use inotify::{EventMask, Inotify};
//use tokio::io::AsyncReadExt;
use std::error::Error;

//Lecture & Ecriture
use std::io::prelude::*;
use std::io; 
use std::net::TcpStream;
use std::io::Write;

use flatbuffers::FlatBufferBuilder;
use inotify::EventMask;
// flatbuffers
use rust_proc_ctrl::proto::demon_generated::demon::{root_as_message, Event, Inotify, InotifyArgs, SocketState, Surveillance, SurveillanceEvent, SurveillanceEventArgs, TCPSocket, TCPSocketArgs};
use rust_proc_ctrl::proto::serialisation::serialize_run_command;

enum ReturnHandleMessage {
    Continue,
    End,
}


fn handle_message(buff: &[u8]) -> ReturnHandleMessage {
    let msg = root_as_message(buff).expect("error root_as_message");

    match msg.events_type(){
        Event::ProcessLaunched => {
            //Recupération de l'objet ProcessLaunched et du pid
            let from_message = msg.events_as_process_launched().expect("error event as process launched");
            let pid = from_message.pid();

            //Sortie
            println!("Processus lancé \npid:{}", pid);
            ReturnHandleMessage::Continue
        }
        Event::TCPSocketListening => {
            let from_mess = msg.events_as_tcpsocket_listening().expect("error event as established tcp connection");
            let port = from_mess.port();

            println!("Established TCP Connection\nport:{}", port);

            ReturnHandleMessage::Continue
        }
        Event::ExecveTerminated => {
            let from_mess = msg.events_as_execve_terminated().expect("error event as execve terminated");
            let pid = from_mess.pid();
            let command = from_mess.command_name().expect("error getting command name execve terminated");
            let succed = from_mess.success();

            if succed {
                println!("Execve success launched the command : {}\npid:{}", command, pid);
            }
            else {
                println!("Execve failed, the command was {}.\npid:{}", command, pid);
            }
            ReturnHandleMessage::Continue
        }
        Event::ChildCreationError => {
            //Recupération de l'objet ChildCreationError et du errno
            let from_message = msg.events_as_child_creation_error().expect("error event as child creation error");
            let errno = from_message.error_code();

            //Sortie
            println!("Erreur lors du lancement du processus fils \nerrno:{}", errno);
            ReturnHandleMessage::End
        }

        Event::ProcessTerminated => {
            //Récupération de l'objet ProcessTerminated, du pid et du errno
            let from_message = msg.events_as_process_terminated().expect("error event as process terminated");
            let pid = from_message.pid();
            let errno = from_message.error_code();

            //Sortie
            println!("Processus terminé. \npid:{} \nCode de retour :{}", pid, errno);
            ReturnHandleMessage::End
        }
        Event::SocketWatchTerminated => {
            let from_mess = msg.events_as_socket_watch_terminated().expect("error events as socket watch terminated");
            let port = from_mess.port();
            let state = from_mess.state();

            print!("Socket ");
            match state {
                SocketState::created => print!("established "),
                SocketState::listeing => print!("listening "),
                _ => print!("state unknown "),
            }
            println!("on port : {}", port);

            ReturnHandleMessage::Continue
        }
        Event::InotifyPathUpdated => {
            let from_mess = msg.events_as_inotify_path_updated().expect("error events as inotify path update");
            let path = from_mess.path().expect("error pas de path");
            let size_limit = from_mess.size_limit();
            let trigger = from_mess.trigger_events();

            println!("Inotify path update, watching : {} with the trigger {:?} and the limit size : {}", path, trigger, size_limit);

            ReturnHandleMessage::Continue
        }
        _ => {
            println!("Reception d'un event inconnue");
            ReturnHandleMessage::End
        }
    }
}



fn main() -> Result<(), Box<dyn std::error::Error>> {
    let port: u16 = 8080;

    let mut input_line = String::new();
    io::stdin().read_line(&mut input_line).expect("Échec de lecture");
    let tokens : Vec<&str> = input_line.trim().split_whitespace().filter(|s| !s.eq(&"\n")).collect();

    let path_command = String::from(tokens[0]);
    let args_command = tokens.iter()
        .filter(|&arg| !arg.eq(&path_command) && !arg.contains("="))
        .cloned()
        .collect::<Vec<&str>>()
        .join(" ");

    let envs_command = tokens.iter()
        .filter(|&arg| arg.contains("="))
        .cloned()
        .collect::<Vec<&str>>()
        .join(" ");

    //Attention : Le dernier string contient un \n
    let args_tab: Vec<&str> = args_command.split(" ").collect();
    // println!("args_tab = {:?}", args_tab);

    let args_envs: Vec<&str> = envs_command.split(" ").collect();
    // println!("envs_tab = {:?}", args_envs);

    let mut fbb = FlatBufferBuilder::new();

    // let to_watch = if path_command == "inotify" {
    //     let root_path_offset = fbb.create_string(".");
    //     let inotify_obj = Inotify::create(&mut fbb, &InotifyArgs {
    //         root_paths: Some(root_path_offset),
    //         mask: EventMask::ACCESS.bits() as i32,
    //         size: 300,
    //     });
    //     let inotify_evt = SurveillanceEvent::create(&mut fbb, &SurveillanceEventArgs {
    //         event_type: Surveillance::Inotify,
    //         event: Some(inotify_obj.as_union_value()),
    //     });
    //     vec![inotify_evt]
    // }
    // else if path_command == "socket" {
    //     let socket_obj = TCPSocket::create(&mut fbb, &TCPSocketArgs {
    //         destport: 8080,
    //     });
    //     let socket_evt = SurveillanceEvent::create(&mut fbb, &SurveillanceEventArgs {
    //         event_type: Surveillance::TCPSocket,
    //         event: Some(socket_obj.as_union_value()),
    //     });
    //     vec![socket_evt]
    // }
    // else {
    //     vec![]
    // };

    let root_path_offset = fbb.create_string(".");
    let inotify_obj = Inotify::create(&mut fbb, &InotifyArgs {
        root_paths: Some(root_path_offset),
        mask: EventMask::ACCESS.bits() as i32,
        size: 300,
    });
    let inotify_evt = SurveillanceEvent::create(&mut fbb, &SurveillanceEventArgs {
        event_type: Surveillance::Inotify,
        event: Some(inotify_obj.as_union_value()),
    });

    let to_watch = vec![inotify_evt];

    let finished_data = serialize_run_command(&mut fbb, &path_command, args_tab, args_envs, 0, 0, to_watch);

    let data_len = finished_data.len() as u32;
    let mut stream = TcpStream::connect(format!("127.0.0.1:{}", port))?;

    // Envoyer la taille en 4 octets
    stream.write_all(&data_len.to_le_bytes())?;
    stream.write_all(&finished_data)?;

    loop {
        //Reception du retour
        let mut len_buf = [0u8; 4];
        if stream.read_exact(&mut len_buf).is_err() {
            println!("Demon déconnecté");
        }
        let msg_len = u32::from_le_bytes(len_buf) as usize;
        let mut buf = vec![0u8; msg_len];
        if stream.read_exact(&mut buf).is_err() {
            println!("Erreur de lecture message");
        }
        match handle_message(&buf) {
            ReturnHandleMessage::Continue => continue,
            ReturnHandleMessage::End => break,
        }
    }

    Ok(())
}
