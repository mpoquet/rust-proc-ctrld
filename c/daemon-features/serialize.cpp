//implementation de code C++ dans un projet C
//c/
//├── include/
//│   ├── serialize.h         // Header C++ original
//│   └── serialize_c.h       // Header C (interface)
//└── daemon-features/
//    ├── serialize.cpp       // Implémentation C++ originale
//    └── serialize_wrapper.cpp  // Implémentation du wrapper
//Dans le code C, il faudra utiliser les fonctions de serialize_wrapper.cpp

#include "../include/serialize.h"
#include <flatbuffers/flatbuffers.h>
#include "../include/demon_generated.h"

//
///PARTIE USER->DEMON
//

void serialize_command(flatbuffers::FlatBufferBuilder *builder, command *cmd) {
    if (!builder || !cmd) {
        throw std::invalid_argument("Invalid arguments");
    }

    // Create string references array for args
    std::vector<flatbuffers::Offset<flatbuffers::String>> args_refs;
    args_refs.reserve(cmd->args_size);
    for (size_t i = 0; i < cmd->args_size; ++i) {
        args_refs.push_back(builder->CreateString(cmd->args[i]));
    }

    // Create string references array for envp
    std::vector<flatbuffers::Offset<flatbuffers::String>> envp_refs;
    envp_refs.reserve(cmd->envp_size);
    for (size_t i = 0; i < cmd->envp_size; ++i) {
        envp_refs.push_back(builder->CreateString(cmd->envp[i]));
    }

    auto path = builder->CreateString(cmd->path);
    auto args = builder->CreateVector(args_refs);
    auto envp = builder->CreateVector(envp_refs);

    // Create surveillances vector
    std::vector<flatbuffers::Offset<demon::SurveillanceEvent>> surveillances;

    // Process each surveillance event
    for (size_t i = 0; i < cmd->to_watch_size; ++i) {
        flatbuffers::Offset<void> event_data;
        demon::Surveillance event_type;

        switch (cmd->to_watch[i].event) {
            case INOTIFY: {
                auto inotify_params = static_cast<struct inotify_parameters*>(cmd->to_watch[i].ptr_event);
                event_data = demon::CreateInotify(
                    *builder,
                    builder->CreateString(inotify_params->root_paths),
                    inotify_params->mask,
                    inotify_params->size
                ).Union();
                event_type = demon::Surveillance_Inotify;
                break;
            }
            case WATCH_SOCKET: {
                auto socket = static_cast<struct tcp_socket*>(cmd->to_watch[i].ptr_event);
                event_data = demon::CreateTCPSocket(
                    *builder,
                    socket->destport
                ).Union();
                event_type = demon::Surveillance_TCPSocket;
                break;
            }
            default:
                continue;
        }

        surveillances.push_back(
            demon::CreateSurveillanceEvent(
                *builder,
                event_type,
                event_data
            )
        );
    }

    auto surveillance_vector = builder->CreateVector(surveillances);

    // Create the RunCommand table
    auto run_command = demon::CreateRunCommand(
        *builder,
        path,
        args,
        envp,
        cmd->flags,
        cmd->stack_size,
        surveillance_vector
    );

    // Create the Message with RunCommand as event
    auto message = demon::CreateMessage(
        *builder,
        demon::Event_RunCommand,
        run_command.Union()
    );

    // Finish the buffer
    builder->Finish(message);

}

//fonction pour envoyer une commande au démon
buffer_info* send_command_to_demon(command *cmd) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info(); // Use new instead of malloc    
    if (!info) {
        return nullptr;
    }
    
    try {
        serialize_command(&builder, cmd);
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un killprocess au démon
buffer_info* send_kill_to_demon(int32_t pid) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto kill_process = demon::CreateKillProcess(builder, pid);
        auto event = demon::CreateMessage(builder, demon::Event_KillProcess, kill_process.Union());
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour désérialiser RunCommand
command* receive_command(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return nullptr;
    }

    command* final_command = new command(); // Use new instead of malloc
    if (!final_command) {
        return nullptr;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        delete final_command;
        return nullptr;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_RunCommand) {
        delete final_command;
        return nullptr;
    }

    auto run_command = message->events_as_RunCommand();
    final_command->path = strdup(run_command->path()->c_str());
    final_command->args_size = run_command->args()->size();
    final_command->envp_size = run_command->envp()->size();
    final_command->flags = run_command->flags();
    final_command->stack_size = run_command->stack_size();
    
    // Allocate memory for args and envp
    final_command->args = new char*[final_command->args_size];
    final_command->envp = new char*[final_command->envp_size];

    // Copy args
    for (size_t i = 0; i < final_command->args_size; ++i) {
        final_command->args[i] = strdup(run_command->args()->Get(i)->c_str());
    }

    // Copy envp
    for (size_t i = 0; i < final_command->envp_size; ++i) {
        final_command->envp[i] = strdup(run_command->envp()->Get(i)->c_str());
    }

    // Process surveillances
    auto surveillances = run_command->to_watch();
    if (surveillances) {
        final_command->to_watch_size = surveillances->size();
        final_command->to_watch = new surveillance[final_command->to_watch_size]; // Use new instead of malloc

        for (size_t i = 0; i < final_command->to_watch_size; ++i) {
            auto surveillance_event = surveillances->Get(i);
            switch (surveillance_event->event_type()) {
                case demon::Surveillance_Inotify: {
                    auto inotify_event = surveillance_event->event_as_Inotify();
                    auto inotify_params = new inotify_parameters(); // Use new instead of malloc
                    inotify_params->root_paths = strdup(inotify_event->root_paths()->c_str());
                    inotify_params->mask = inotify_event->mask();
                    inotify_params->size = inotify_event->size();
                    final_command->to_watch[i].event = INOTIFY;
                    final_command->to_watch[i].ptr_event = inotify_params;
                    break;
                }
                case demon::Surveillance_TCPSocket: {
                    auto socket_event = surveillance_event->event_as_TCPSocket();
                    auto socket_params = new tcp_socket(); // Use new instead of malloc
                    socket_params->destport = socket_event->destport();
                    final_command->to_watch[i].event = WATCH_SOCKET;
                    final_command->to_watch[i].ptr_event = socket_params;
                    break;
                }
                default:
                    break;
            }
        }
    } else {
        final_command->to_watch_size = 0;
        final_command->to_watch = nullptr;
    }
    return final_command;
}

//fonction pour désérialiser un killprocess
int32_t receive_kill(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return -1;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return -1;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_KillProcess) {
        return -1;
    }

    auto kill_process = message->events_as_KillProcess();
    return kill_process->pid();
}

//permet au démon de savoir si le message reçu est une commande ou un killprocess

Event receive_message_from_user(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return NONE;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return NONE;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() == demon::Event_RunCommand) {
        return RUN_COMMAND;
    } else if (message->events_type() == demon::Event_KillProcess) {
        return KILL_PROCESS;
    } else {
        return NONE; 
    }
}

//
///PARTIE DEMON->USER
//

//fonction pour envoyer un processlaunched a l'user
buffer_info* send_processlaunched_to_user(int32_t pid) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto process_launched = demon::CreateProcessLaunched(builder, pid);
        auto event = demon::CreateMessage(builder, demon::Event_ProcessLaunched, process_launched.Union());
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un childcreationerror a l'user
buffer_info* send_childcreationerror_to_user(uint32_t error_code) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto child_creation_error = demon::CreateChildCreationError(builder, error_code);
        auto event = demon::CreateMessage(builder, demon::Event_ChildCreationError, child_creation_error.Union());
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un processterminaed a l'user
buffer_info* send_processterminated_to_user(int32_t pid, uint32_t error_code) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto process_terminated = demon::CreateProcessTerminated(builder, pid, error_code);
        auto event = demon::CreateMessage(builder, demon::Event_ProcessTerminated, process_terminated.Union());
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un TCPSocketListening a l'user
buffer_info* send_tcpsocketlistening_to_user(uint16_t port) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto tcp_socket = demon::CreateTCPSocketListening(builder, port);
        auto event = demon::CreateMessage(builder, demon::Event_TCPSocketListening, tcp_socket.Union());
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un inotifypathupdated a l'user

buffer_info* send_inotifypathupdated_to_user(InotifyPathUpdated *inotify) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto path_str = builder.CreateString(inotify->path);
        
        auto inotify_path_updated = demon::CreateInotifyPathUpdated(
            builder,
            path_str,
            static_cast<demon::InotifyEvent>(inotify->event),  
            inotify->size,
            inotify->size_limit  
        );
        
        auto event = demon::CreateMessage(
            builder, 
            demon::Event_InotifyPathUpdated, 
            inotify_path_updated.Union()
        );
        
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un inofitywatchlistupdated a l'user
buffer_info* send_inotifywatchlistupdated_to_user(char *path) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto path_str = builder.CreateString(path);
        
        auto inotify_watch_list_updated = demon::CreateInotifyWatchListUpdated(
            builder,
            path_str
        );
        
        auto event = demon::CreateMessage(
            builder, 
            demon::Event_InotifyWatchListUpdated, 
            inotify_watch_list_updated.Union()
        );
        
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un socketwatched a l'user
buffer_info* send_socketwatched_to_user(int32_t port) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto socket_watched = demon::CreateSocketWatched(builder, port);
        auto event = demon::CreateMessage(builder, demon::Event_SocketWatched, socket_watched.Union());
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour envoyer un socketwatchterminated a l'user
buffer_info* send_socketwatchterminated_to_user(struct socket_watch_info* socket_info) {
    flatbuffers::FlatBufferBuilder builder(1024);
    buffer_info* info = new buffer_info();  
    if (!info) {
        return nullptr;
    }
    
    try {
        auto socket_watch_terminated = demon::CreateSocketWatchTerminated(
            builder,
            socket_info->port,
            static_cast<demon::SocketState>(socket_info->state)
        );
        
        auto event = demon::CreateMessage(
            builder, 
            demon::Event_SocketWatchTerminated, 
            socket_watch_terminated.Union()
        );
        
        builder.Finish(event);
        
        size_t size = builder.GetSize();
        uint8_t* buffer_copy = new uint8_t[size];
        memcpy(buffer_copy, builder.GetBufferPointer(), size);
        info->buffer = buffer_copy;
        info->size = size;
    } catch (...) {
        delete info;
        throw;
    }
    
    return info;
}

//fonction pour reçevoir processlaunched du démon
int32_t receive_processlaunched(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return -1;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return -1;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_ProcessLaunched) {
        return -1;
    }

    auto process_launched = message->events_as_ProcessLaunched();
    return process_launched->pid();
}

//fonction pour reçevoir childerror du démon
int32_t receive_childcreationerror(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return -1;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return -1;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_ChildCreationError) {
        return -1;
    }

    auto child_creation_error = message->events_as_ChildCreationError();
    return child_creation_error->error_code();
}

//fonction pour reçevoir processterminaed du démon
process_terminated_info* receive_processterminated(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return nullptr;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return nullptr;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_ProcessTerminated) {
        return nullptr;
    }

    auto process_terminated = message->events_as_ProcessTerminated();
    process_terminated_info* infos = new process_terminated_info();  
    if (!infos) {
        return nullptr;
    }
    
    infos->pid = process_terminated->pid();
    infos->error_code = process_terminated->error_code();
    
    return infos;
}

//fonction pour reçevoir TCPSocketListening du démon
uint16_t receive_TCPSocketListening(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return 0;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return 0;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_TCPSocketListening) {
        return 0;
    }

    auto tcp_socket = message->events_as_TCPSocketListening();
    return tcp_socket->port();
}

//fonction pour reçevoir inotifypathupdated du démon

InotifyPathUpdated* receive_inotifypathupdated(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return nullptr;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return nullptr;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_InotifyPathUpdated) {
        return nullptr;
    }

    auto inotify_path_updated = message->events_as_InotifyPathUpdated();
    InotifyPathUpdated* inotify = new InotifyPathUpdated();  
    if (!inotify) {
        return nullptr;
    }
    
    try {
        inotify->path = strdup(inotify_path_updated->path()->c_str());
        if (!inotify->path) {
            delete inotify;
            return nullptr;
        }

        inotify->event = static_cast<InotifyEvent>(inotify_path_updated->trigger_events());
        
        inotify->size = inotify_path_updated->size();
        inotify->size_limit = inotify_path_updated->size_limit();
        
        return inotify;
    } catch (...) {
        if (inotify->path) free(inotify->path);
        delete inotify;
        throw;
    }
}

//fonction pour reçevoir inotifywatchlistupdated du démon
char* receive_inotifywatchlistupdated(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return nullptr;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return nullptr;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_InotifyWatchListUpdated) {
        return nullptr;
    }

    auto inotify_watch_list_updated = message->events_as_InotifyWatchListUpdated();
    char* path = strdup(inotify_watch_list_updated->path()->c_str());
    
    return path;
}

//fonction pour reçevoir socketwatched du démon
int32_t receive_socketwatched(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return -1;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return -1;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_SocketWatched) {
        return -1;
    }

    auto socket_watched = message->events_as_SocketWatched();
    return socket_watched->port();
}

//fonction pour reçevoir socketwatchterminated du démon
struct socket_watch_info* receive_socketwatchterminated(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return nullptr;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return nullptr;
    }

    auto message = demon::GetMessage(buffer);
    if (message->events_type() != demon::Event_SocketWatchTerminated) {
        return nullptr;
    }

    auto socket_watch_terminated = message->events_as_SocketWatchTerminated();
    struct socket_watch_info* socket_info = new socket_watch_info();  
    if (!socket_info) {
        return nullptr;
    }
    
    socket_info->port = socket_watch_terminated->port();
    socket_info->state = static_cast<SocketState>(socket_watch_terminated->state());
    
    return socket_info;
}

//permet a l'user de savoir de quel type est le message reçu
// (processlaunched, childcreationerror, processterminated, tcpsocketlistening, inotifypathupdated)
Event receive_message_from_demon(uint8_t *buffer, int size) {
    if (!buffer || size <= 0) {
        return NONE;
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (!demon::VerifyMessageBuffer(verifier)) {
        return NONE;
    }

    auto message = demon::GetMessage(buffer);
    switch (message->events_type()) {
        case demon::Event_ProcessLaunched:
            return PROCESS_LAUNCHED;
        case demon::Event_ChildCreationError:
            return CHILD_CREATION_ERROR;
        case demon::Event_ProcessTerminated:
            return PROCESS_TERMINATED;
        case demon::Event_TCPSocketListening:
            return TCP_SOCKET_LISTENING;
        case demon::Event_InotifyPathUpdated:
            return INOTIFY_PATH_UPDATED;
        case demon::Event_InotifyWatchListUpdated:
            return INOTIFY_WATCH_LIST_UPDATED;
        case demon::Event_SocketWatched:
            return SOCKET_WATCHED;
        case demon::Event_SocketWatchTerminated:
            return SOCKET_WATCH_TERMINATED;
        default:
            return NONE;
    }
}