#[allow(unused)]

//use inotify::{EventMask, Inotify};
//use tokio::io::AsyncReadExt;
use std::error::Error;

//Lecture & Ecriture
use std::io::prelude::*;
use std::env;
use std::io; 
use std::net::TcpListener;
use std::net::TcpStream;
use std::io::Write;

//use crate::monitoring_tools::command::exec_command;

use flatbuffers::FlatBufferBuilder;

use crate::proto::demon_generated::demon::RunCommandArgs;

// flatbuffers
use crate::proto::demon_generated::demon::{root_as_message, finish_message_buffer, Message, MessageArgs, Event, InotifyEvent, RunCommand};


fn handle_message(buff: &[u8]){
    let msg = root_as_message(buff).expect("error root_as_message");

    match msg.events_type(){
        Event::ProcessLaunched => {
            //Recupération de l'objet ProcessLaunched et du pid
            let from_message = msg.events_as_process_launched().expect("error event as process launched");
            let pid = from_message.pid();

            //Sortie
            println!("Processus lancé \npid:{}", pid);
        }

        Event::ChildCreationError => {
            //Recupération de l'objet ChildCreationError et du errno
            let from_message = msg.events_as_child_creation_error().expect("error event as child creation error");
            let errno = from_message.errno();

            //Sortie
            println!("Erreur lors du lancement du processus fils \nerrno:{}", errno);
        }

        Event::ProcessTerminated => {
            //Récupération de l'objet ProcessTerminated, du pid et du errno
            let from_message = msg.events_as_process_terminated().expect("error event as process terminated");
            let pid = from_message.pid();
            let errno = from_message.errno();

            //Sortie
            println!("Processus terminé. \npid:{} \nerrno:{}", pid, errno);
        }

        _ => {
            println!("Reception d'un event inconnue");
        }
    }
}



fn main() -> Result<(), Box<dyn std::error::Error>> {
    println!("Prototype client en rust \n");

    let mut presence_clone = false;
    let mut flag: u32 = 0;
    let mut stack: u32 = 0;


    //Lecture path
    println!("Path : ");
    let mut path_command = String::new();
    io::stdin().read_line(&mut path_command).expect("Echec lecture");

    //Lecture des arguments
    println!("Arguments : ");
    let mut args_command = String::new();
    io::stdin().read_line(&mut args_command).expect("Echec lecture");

    //Lecture des environnements
    println!("envs : ");
    let mut envs_command = String::new();
    io::stdin().read_line(&mut envs_command).expect("Echec lecture");

    
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


    println!("path : {}", path_command);
    println!("arguments : {}", args_command);
    println!("environnements : {}", envs_command);

    if presence_clone{
        println!("flag : {}", flag);
        println!("stack : {}", stack);
    }

    //Attention : Le dernier string contient un \n
    let mut args_tab: Vec<&str> = args_command.split(" ").collect();
    println!("{:?}", args_tab);

    let mut args_envs: Vec<&str> = envs_command.split(" ").collect();
    println!("{:?}", args_envs);

    //Initialisation du builder
    let mut bldr = FlatBufferBuilder::new();
    bldr.reset();


    //Creation de l'objet RunCommand
    let args_build = RunCommandArgs{path: Some(bldr.create_string(&path_command)), 
                                    args: Some(bldr.create_vector_of_strings(&args_tab)),
                                    envp: Some(bldr.create_vector_of_strings(&args_envs)),
                                    ..Default::default()};
    
    let object_run_command = RunCommand::create(&mut bldr, &args_build);

    //Creation et serialisation de l'objet Message pour l'envoi
    let mess = Message::create(
        &mut bldr, 
        &MessageArgs{
            events_type: Event::RunCommand,
            events: Some(object_run_command.as_union_value())});

    bldr.finish(mess, None);


    //Initialisation de la connection TCP (ne prends pas en compte l'IP et le port pour l'instant)
    /*let mut args = env::args();
    if args.len() != 2 {
        println!("Usage : {:?} <PortId>", args.next())
    }

    let port: u16 = args.nth(1)
        .expect("error getting args")
        .parse()
        .expect("invalid port number");

    println!("We listen on the port {}", port);

    let listener = TcpListener::bind(format!("127.0.0.1::{}", port)).unwrap(); 


    let Ok((mut socket, addr)) = listener.accept(); */

    let port: u16 = 8080;


    let mut stream = TcpStream::connect(format!("127.0.0.1::{}", port))?;

    //Envoi sur le réseau
    stream.write(&bldr.finished_data().to_vec())?;
    
    //Reception du retour
    // Lire la taille (4 octets)
    let mut len_buf = [0u8; 4];
    if stream.read_exact(&mut len_buf).is_err() {
        println!("Demon déconnecté");
    }

    let msg_len = u32::from_le_bytes(len_buf) as usize;

    // Lire le message flatbuffer complet
    let mut buf = vec![0u8; msg_len];
    if stream.read_exact(&mut buf).is_err() {
        println!("Erreur de lecture message");
    }

    handle_message(&buf);  

    Ok(())
}
