use flatbuffers::FlatBufferBuilder;

use super::demon_generated::demon;


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

pub fn serialize_child_creation_error(errno: u32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let child_error = demon::ChildCreationError::create(
        &mut builder, &demon::ChildCreationErrorArgs {
            errno
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

pub fn serialize_process_terminated(pid: i32, errno: u32) -> Vec<u8> {
    let mut builder = FlatBufferBuilder::new();

    let terminated = demon::ProcessTerminated::create(
        &mut builder,
        &demon::ProcessTerminatedArgs { pid, errno },
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