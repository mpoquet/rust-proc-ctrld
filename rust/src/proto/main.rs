use std::env;
use inotify::{EventMask, Inotify};
use tokio::io::AsyncReadExt;
use std::error::Error;

// monitoring tools
use crate::monitoring_tools::inotify_tool::read_events_inotify;
use crate::monitoring_tools::network::read_events_port_tokio;
use crate::monitoring_tools::signal_tool::send_sigkill;
use crate::monitoring_tools::command::exec_command;

// flatbuffers
use crate::proto::demon_generated::demon::{root_as_message, finish_message_buffer,
    Event, InotifyEvent, Inotify, TCPSocket, Surveillance, SurveillanceEvent, RunCommand, KillProcess};

use tokio::net::TcpListener;

//est-ce que l'on fait passer le retour par cette fonction ?
async fn handle_message(buf: &[u8]) {

    //Création du builder flatbuffer
    let mut bldr = FlatBufferBuilder::new();
    bldr.reset();

    let msg = root_as_message(buf).expect("error root_as_message");

    match msg.events_type() {
        Event::EstablishTCPConnection => {
            // TODO


            //Affectation temporaire, pour déclarer la variable
            let destination_port : i8 = 5;

            //Creation de l'objet EstablishTCPConnection
            let args = EstablishTCPConnectionArgs{destport: destination_port};
            let object_TCP_Co = EstablishTCPConnection::create(&mut bldr, &args);

            //Creation et sérialisation de l'objet Event pour le retour (as_union_value à revoir)
            let event = Event::create(&mut bldr, EventArgs{Some(object_TCP_Co.as_union_value())});

            bldr.finish(event, None);

            let buf = bldr.finished_data();

            return buf;          
        }
        Event::EstablishUnixConnection => {
            // TODO

            //Affectation temporaire, pour déclarer la variable
            let destination_port : String = "abc";

            //Creation de l'objet EstablishUnixConnection
            let args = EstablishUnixConnectionArgs{destport: destination_port};
            let object_Unix_Co = EstablishUnixConnection::create(&mut bldr, &args);

            //Creation et sérialisation de l'objet Event pour le retour
            let event = Event::create(&mut bldr, EventArgs{Some(object_Unix_Co.as_union_value())});

            bldr.finish(event, None);

            let buf = bldr.finished_data();

            return buf;


        }
        Event::InotifyPathUpdated => {
            let from_message =
                msg.events_as_inotify_path_updated().expect("error events_as_inotify...");
            let path = from_message.path().expect("No path from the message");
            let trigger_events = from_message.trigger_events().expect("No vector of trigger events for inotify");
            let reach_size = 500;                           // TODO

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

            //retour ici ?
        }
        Event::KillProcess => {
            let from_mess = msg.events_as_kill_process().expect("error events_as_kill_process");
            let pid_kill = from_mess.pid();

            send_sigkill(pid_kill).expect("error while sending SIGKILL");


            //Creation de l'objet KillProcess
            let args = KillProcessArgs{pid: pid_kill};
            let object_Kill = KillProcess::create(&mut bldr, &args);

            //Creation et sérialisation de l'objet Event pour le retour
            let event = Event::create(&mut bldr, EventArgs{Some(object_Kill.as_union_value())});

            bldr.finish(event, None);

            let buf = bldr.finished_data();

            return buf;
            
        }
        Event::ProcessLaunched => {
            let from_mess = msg.events_as_process_launched().expect("error events_as_process_launched");
            let pid_launch = from_mess.pid();
            // TODO

            //Creation de l'objet ProcessLaunched
            let args = ProcessLaunchedArgs{pid: pid_launch};
            let object_Launch = ProcessLaunched::create(&mut bldr, &args);

            //Creation de l'objet Event pour le retour
            let event = Event::create(&mut bldr, EventArgs{Some(object_Launch.as_union_value())});

            bldr.finish(event, None);

            let buf = bldr.finished_data();

            return buf;
        }
        Event::ProcessTerminated => {
            let from_mess = msg.events_as_process_terminated().expect("error events_as_process_terminated");
            let pid_terminated = from_mess.pid();
            // TODO

            //Creation de l'objet ProcessTerminated
            let args = ProcessTerminatedArgs{pid: pid_terminated};
            let object_Terminated = ProcessTerminated::create(&mut bldr, &args);

            //Creation de l'objet Event pour le retour
            let event = Event::create(&mut bldr, EventArgs{Some(object_Terminated.as_union_value())});

            bldr.finish(event, None);

            let buf = bldr.finished_data();

            return buf;
            

        }
        Event::RunCommand => {
            let from_mess = msg.events_as_run_command().expect("error events_as_run_command");
            let flags = from_mess.flags();
            let stack_size = from_mess.stack_size();
            let to_watch = from_mess.to_watch().expect("error to get to_watch");
            
            exec_command(&from_mess).await;

            //retour ici ? Pas de doublon avec ProcessLaunched ?
        }
        Event::TCPSocketListening => {
            println!("Handling EventType2");
            let from_mess = msg.events_as_tcpsocket_listening().expect("error from receiving message");
            let port = from_mess.port();

            read_events_port_tokio(port).await.expect("error read_events_port");

            //retour ici ?
        }
        Event::ChildCreationError => {
            //TODO

            //Affectation temporaire, pour déclarer la variable
            let erreur : i32 = 5;

            //Creation de l'objet ChildCreationError
            let args = ChildCreationErrorArgs{errno: erreur};
            let object_ChildErr = ChildCreationError::create(&mut bldr, &args);

            //Creation de l'objet Event de retour
            let event = Event::create(&mut bldr, EventArgs{Some(object_ChildErr.as_union_value())});

            bldr.finish(event, None);

            let buf = bldr.finished_data();

            return buf;



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

                //
                
                //Traitement + obtention du retour
                let mut retour = handle_message(&buf).await;

                //Envoi sur le réseau
                socket.write(&mut retour).await;
                
            }
        });
    }
}