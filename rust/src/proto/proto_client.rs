#[allow(unused)]

//use inotify::{EventMask, Inotify};
//use tokio::io::AsyncReadExt;
use std::error::Error;

//Lecture & Ecriture
use std::env;
use std::io; 
use std::net::TcpListener;
use std::io::Write;

//use crate::monitoring_tools::command::exec_command;

use flatbuffers::FlatBufferBuilder;

use crate::proto::demon_generated::demon::RunCommandArgs;

// flatbuffers
use crate::proto::demon_generated::demon::{root_as_message, finish_message_buffer, Message, MessageArgs, Event, InotifyEvent, RunCommand};




fn main() {
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
    bldr.finished_data();


    //Initialisation de la connection TCP (ne prends pas en compte l'IP pour l'instant)
    let mut args = env::args();
    if args.len() != 2 {
        println!("Usage : {:?} <PortId>", args.next())
    }

    let port: u16 = args.nth(1)
        .expect("error getting args")
        .parse()
        .expect("invalid port number");

    println!("We listen on the port {}", port);

    let listener = TcpListener::bind(format!("127.0.0.1::{}", port)).unwrap();


    let Ok((mut socket, addr)) = listener.accept();

    println!("{}", addr);

    //Envoi sur le réseau
    socket.write(&mut mess);
    
}
