use flatbuffers::FlatBufferBuilder;

use super::demon_generated::demon::{self, Message, RunCommand, RunCommandArgs};
use crate::proto::serialisation::demon::MessageArgs;
use crate::proto::serialisation::demon::Event;


pub fn serialize_established_tcp_connection(port: u32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let established = demon::EstablishTCPConnection::create(
        &mut builder, 
        &demon::EstablishTCPConnectionArgs {destport: port}    
    );

    let mess = demon::Message::create(
        &mut builder, 
        &demon::MessageArgs {
            events_type: demon::Event::EstablishTCPConnection,
            events: Some(established.as_union_value())
        }
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

pub fn serialize_run_command(path_command: &str, args_tab: Vec<&str>, args_envs: Vec<&str>) -> Vec<u8>{
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