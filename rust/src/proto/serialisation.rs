use flatbuffers::{FlatBufferBuilder, WIPOffset};

use super::demon_generated::demon::{InotifyEvent, SurveillanceEvent};
use super::demon_generated::demon::{self, Message, RunCommand, RunCommandArgs};
use crate::proto::serialisation::demon::MessageArgs;
use crate::proto::serialisation::demon::Event;

pub fn serialize_execve_terminated(pid: i32, command_name: String, success: bool) -> Vec<u8>{
    let mut builder = FlatBufferBuilder::new();

    let command_name_str = builder.create_string(&command_name);
    let execve = demon::ExecveTerminated::create(
        &mut builder,
        &demon::ExecveTerminatedArgs {
            pid,
            command_name: Some(command_name_str),
            success
        },
    );

    let mess = demon::Message::create(
        &mut builder,
        &demon::MessageArgs {
            events_type: demon::Event::ExecveTerminated,
            events: Some(execve.as_union_value()),
        },
    );

    builder.finish(mess, None);
    builder.finished_data().to_vec()
}


pub fn serialize_tcp_socket_listenning(port: u16) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let tcp_socket = demon::TCPSocketListening::create(
        &mut builder,
        &demon::TCPSocketListeningArgs {
            port,
        },
    );

    let mess = demon::Message::create(
        &mut builder,
        &demon::MessageArgs {
            events_type: demon::Event::TCPSocketListening,
            events: Some(tcp_socket.as_union_value()),
        },
    );

    builder.finish(mess, None);
    builder.finished_data().to_vec()
}


pub fn serialize_process_launched(pid: i32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let process_launched = demon::ProcessLaunched::create(
        &mut builder,
        &demon::ProcessLaunchedArgs {pid}
    );

    let mess = demon::Message::create(
        &mut builder,
        &demon::MessageArgs {
            events_type: demon::Event::ProcessLaunched,
            events: Some(process_launched.as_union_value()),
        }
    );

    builder.finish(mess, None);
    builder.finished_data().to_vec()
}   

pub fn serialize_child_creation_error(error_code: u32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let child_error = demon::ChildCreationError::create(
        &mut builder, &demon::ChildCreationErrorArgs {
            error_code
        }
    );

    let mess = demon::Message::create(
        &mut builder,
        &demon::MessageArgs {
            events_type: demon::Event::ChildCreationError,
            events: Some(child_error.as_union_value()),
        }
    );

    builder.finish(mess, None);
    builder.finished_data().to_vec()
}

pub fn serialize_process_terminated(pid: i32, error_code: u32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let terminated = demon::ProcessTerminated::create(
        &mut builder,
        &demon::ProcessTerminatedArgs { pid, error_code },
    );

    let message = demon::Message::create(
        &mut builder,
        &demon::MessageArgs {
            events_type: demon::Event::ProcessTerminated,
            events: Some(terminated.as_union_value()),
        },
    );
    
    builder.finish(message, None);
    builder.finished_data().to_vec()
}

pub fn serialize_run_command<'a>(
    bldr: &mut FlatBufferBuilder<'a>,
    path_command: &str, 
    args_tab: Vec<&str>, 
    args_envs: Vec<&str>,
    flags: u32,
    stack_size: u32,
    to_watch: Vec<WIPOffset<SurveillanceEvent<'a>>>
) -> Vec<u8> {
    let mut bldr = bldr;

    // Création des offsets pour les vecteurs de chaînes
    let args_offsets: Vec<_> = args_tab.iter().map(|s| bldr.create_string(s)).collect();
    let envs_offsets: Vec<_> = args_envs.iter().map(|s| bldr.create_string(s)).collect();

    let args_vector = bldr.create_vector(&args_offsets);
    let envs_vector = bldr.create_vector(&envs_offsets);
    
    // Construction de l'objet RunCommand avec le schéma étendu
    let args_build = RunCommandArgs {
        path: Some(bldr.create_string(&path_command)),
        args: Some(args_vector),
        envp: Some(envs_vector),
        flags,
        stack_size,
        to_watch: Some(bldr.create_vector(&to_watch)),
        ..Default::default()
    };

    let object_run_command = RunCommand::create(&mut bldr, &args_build);

    // Création et sérialisation de l'objet Message pour l'envoi
    let mess = Message::create(
        &mut bldr,
        &MessageArgs {
            events_type: Event::RunCommand,
            events: Some(object_run_command.as_union_value()),
        }
    );

    bldr.finish(mess, None);
    bldr.finished_data().to_vec()
}

pub fn serialize_inotify(path: &str, mask: i32, size: u32) -> Vec<u8> {
    //Initialisation du builder
    let mut bldr = FlatBufferBuilder::new();
    bldr.reset();

    // Création de l'objet Inotify à partir des InotifyArgs.
    let inotify_args = demon::InotifyArgs {
        root_paths: Some(bldr.create_string(path)),
        mask,
        size,
        ..Default::default()
    };
    let object_inotify = demon::Inotify::create(&mut bldr, &inotify_args);

    // Construction et sérialisation de l'objet Message pour l'envoi.
    let mess = demon::Message::create(
        &mut bldr,
        &demon::MessageArgs {
            events_type: demon::Event::InotifyPathUpdated,
            events: Some(object_inotify.as_union_value()),
        }
    );

    bldr.finish(mess, None);
    bldr.finished_data().to_vec()
}

pub fn serialize_inotify_path_update(path: &str, trigger: InotifyEvent, size: u32, size_limit: u32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    // Création de l'offset pour la chaîne de caractères "path"
    let path_offset = builder.create_string(path);

    // Création de l'objet InotifyPathUpdated
    let inotify_update = demon::InotifyPathUpdated::create(
        &mut builder,
        &demon::InotifyPathUpdatedArgs {
            path: Some(path_offset),
            trigger_events: trigger,
            size,
            size_limit,
        },
    );

    // Création du message en incluant l'objet InotifyPathUpdated
    let message = Message::create(
        &mut builder,
        &MessageArgs {
            events_type: Event::InotifyPathUpdated,
            events: Some(inotify_update.as_union_value()),
        },
    );

    builder.finish(message, None);
    builder.finished_data().to_vec()
}

pub fn serialize_socket_watched(port: i32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let socket_watched = demon::SocketWatched::create(
        &mut builder,
        &demon::SocketWatchedArgs {
            port,
        },
    );

    let mess = Message::create(
        &mut builder,
        &MessageArgs {
            events_type: Event::SocketWatched, // Assurez-vous que l'enum Event possède ce variant
            events: Some(socket_watched.as_union_value()),
        },
    );

    builder.finish(mess, None);
    builder.finished_data().to_vec()
}

pub fn serialize_socket_watch_terminated(port: i32, state: demon::SocketState) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let socket_watch_terminated = demon::SocketWatchTerminated::create(
        &mut builder,
        &demon::SocketWatchTerminatedArgs {
            port,
            state,
        },
    );

    let mess = Message::create(
        &mut builder,
        &MessageArgs {
            events_type: Event::SocketWatchTerminated, // Assurez-vous que l'enum Event possède ce variant
            events: Some(socket_watch_terminated.as_union_value()),
        },
    );

    builder.finish(mess, None);
    builder.finished_data().to_vec()
}