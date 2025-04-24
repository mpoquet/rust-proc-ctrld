#[allow(unused)]

//use inotify::{EventMask, Inotify};
//use tokio::io::AsyncReadExt;
use std::error::Error;

//Lecture & Ecriture
use std::env;
use std::io; 
use std::fs::File;
use std::io::Write;
use std::io::prelude::*;
use std::net::TcpListener;

//use crate::monitoring_tools::command::exec_command;

// flatbuffers
use demon_generated::demon::{root_as_message, finish_message_buffer, Event, InotifyEvent, RunCommand};




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
    let args_build = RunCommandArgs{path: path_command, 
                                    args: args_command,
                                    envp: envs_command,
                                    ..Default::default()};
    
    let object_runCommand = RunCommand::create(&mut bldr, &args_build);

    //Creation et serialisation de l'objet Event pour l'envoi
    let event = Event::create(&mut bldr, EventArgs{Some(object_runCommand.as_union())});


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

    let listener = TcpListener::bind(format!("127.0.0.1::{}", port));

    let (mut socket, addr) = listener.accept();

    //Envoi sur le réseau
    socket.write(&mut event);
    
}
