#[allow(unused)]

//use inotify::{EventMask, Inotify};
//use tokio::io::AsyncReadExt;
use std::error::Error;

//Lecture & Ecriture
use std::io::prelude::*;
use std::io; 
use std::net::TcpStream;
use std::io::Write;

use std::time::SystemTime;

//use crate::monitoring_tools::command::exec_command;

use flatbuffers::FlatBufferBuilder;

use rust_proc_ctrl::proto::demon_generated::demon::RunCommandArgs;

// flatbuffers
use rust_proc_ctrl::proto::demon_generated::demon::{root_as_message, Message, MessageArgs, Event, RunCommand};


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
    
    //Suppression du \n
    path_command.pop();

    //Lecture des arguments
    println!("Arguments : ");
    let mut args_command = String::new();
    io::stdin().read_line(&mut args_command).expect("Echec lecture");

    //Suppression du \n
    args_command.pop();


    //Lecture des environnements
    println!("envs : ");
    let mut envs_command = String::new();
    io::stdin().read_line(&mut envs_command).expect("Echec lecture");

    //Suppression du \n
    envs_command.pop();

    //Mesure du temps
    let depart = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH)?.as_millis();
    
    //Lecture du flag et stack si clone 
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



    if presence_clone{
        println!("flag : {}", flag);
        println!("stack : {}", stack);
    }

    //Construction des tableaux de string
    let args_tab: Vec<&str> = args_command.split(" ").collect();
    //println!("{:?}", args_tab);

    let args_envs: Vec<&str> = envs_command.split(" ").collect();
    //println!("{:?}", args_envs);

    //Initialisation du builder
    let mut bldr = FlatBufferBuilder::new();
    bldr.reset();

    let args_offsets: Vec<_> = args_tab.iter().map(|s| bldr.create_string(s)).collect();
    let envs_offsets: Vec<_> = args_envs.iter().map(|s| bldr.create_string(s)).collect();

    let args_vector = bldr.create_vector(&args_offsets);
    let envs_vector = bldr.create_vector(&envs_offsets);


    //Creation de l'objet RunCommand
    let args_build = RunCommandArgs{
        path: Some(bldr.create_string(&path_command)), 
        args: Some(args_vector),
        envp: Some(envs_vector),
        ..Default::default()};

    let object_run_command = RunCommand::create(&mut bldr, &args_build);

    //Creation et serialisation de l'objet Message pour l'envoi
    let mess = Message::create(
        &mut bldr, 
        &MessageArgs{
            events_type: Event::RunCommand,
            events: Some(object_run_command.as_union_value())});


    let port: u16 = 8080;


    bldr.finish(mess, None);

    let finished_data = bldr.finished_data();
    let data_len = finished_data.len() as u32;
    let mut stream = TcpStream::connect(format!("127.0.0.1:{}", port))?;

    // Envoyer la taille en 4 octets
    stream.write_all(&data_len.to_le_bytes())?;
    stream.write_all(finished_data)?;

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

    //Gestion de la réponse du démon
    handle_message(&buf);   

    //Calcul du temps
    let arrive = SystemTime::now().duration_since(SystemTime::UNIX_EPOCH)?.as_millis() - depart;

    println!("Durée : {} millisecondes", arrive);

    Ok(())
}
