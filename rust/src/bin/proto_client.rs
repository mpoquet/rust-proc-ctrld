#[allow(unused)]

//use inotify::{EventMask, Inotify};
//use tokio::io::AsyncReadExt;
use std::error::Error;

//Lecture & Ecriture
use std::io::prelude::*;
use std::io; 
use std::net::TcpStream;
use std::io::Write;

// flatbuffers
use rust_proc_ctrl::proto::demon_generated::demon::{root_as_message, Event, InotifyEvent, SocketState};
use rust_proc_ctrl::proto::serialisation::{serialize_inotify_path_update, serialize_run_command, serialize_socket_watched};

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

            ReturnHandleMessage::End
        }
        _ => {
            println!("Reception d'un event inconnue");
            ReturnHandleMessage::End
        }
    }
}



fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut presence_clone = false;
    let mut flag: u32 = 0;
    let mut stack: u32 = 0;

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
    
    //Lecture du flag et stack si clone 
    //Attention : le eq() ne semble pas fonctionner ici (toujours false)
    if path_command.eq("clone"){
        //Lecture du flag
        println!("flag : ");
        let mut flag_command = String::new();
        io::stdin().read_line(&mut flag_command).expect("Echec lecture");
        flag = flag_command.trim().parse().expect("int attendu");

        //Lecture du stack
        println!("stack : ");
        let mut stack_command = String::new();
        io::stdin().read_line(&mut stack_command).expect("Echec lecture");
        stack = stack_command.trim().parse().expect("int attendu");

        presence_clone = true;
    }

    // println!("path : {}", path_command);
    // println!("arguments : {}", args_command);
    // println!("environnements : {}", envs_command);

    if presence_clone{
        println!("flag : {}", flag);
        println!("stack : {}", stack);
    }

    //Attention : Le dernier string contient un \n
    let args_tab: Vec<&str> = args_command.split(" ").collect();
    // println!("args_tab = {:?}", args_tab);

    let args_envs: Vec<&str> = envs_command.split(" ").collect();
    // println!("envs_tab = {:?}", args_envs);

    // Match vers sérialisation

    let finished_data =
    if path_command.eq("inotify") {
        serialize_inotify_path_update(".", InotifyEvent::accessed, 30, 6000)
    }
    else if path_command.eq("socket") {
        serialize_socket_watched(9090)
    }
    else {
        serialize_run_command(&path_command, args_tab, args_envs)
    };

    let port: u16 = 8080;

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
