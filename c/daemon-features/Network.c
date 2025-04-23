#include "../include/Network.h"
#include "../include/demon_builder.h"
#include "../include/demon_verifier.h"
#include "../include/demon_reader.h"

// A helper to simplify creating vectors from C-arrays.
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

// This allows us to verify result in optimized builds.
#define test_assert(x) do { if (!(x)) { assert(0); return -1; }} while(0)

/* Helper function for string access when deserializing.
   flatbuffers_string_t in FlatCC is typically a char pointer. */
static inline const char *get_string_chars(flatbuffers_string_t s) {
    return s ? (const char *)s : "";
}

int establish_connection(int port){
    return 0;
}

//
///PARTIE USER->DEMON
//

static demon_Inotify_ref_t create_inotify(flatcc_builder_t *B, struct inotify_parameters *inotify) {
    demon_Inotify_start(B);
    demon_Inotify_root_paths_create_str(B, inotify->root_paths);
    demon_Inotify_trigger_events_create(B, inotify->i_events, inotify->size);
    demon_Inotify_size_add(B, inotify->size);
    return demon_Inotify_end(B);
}


static demon_TCPSocket_ref_t create_tcp_socket(flatcc_builder_t *B, struct tcp_socket *socket) {
    demon_TCPSocket_start(B);
    demon_TCPSocket_destport_add(B, socket->destport);
    return demon_TCPSocket_end(B);
}

static demon_SurveillanceEvent_ref_t create_surveillance_event(flatcc_builder_t *B, struct surveillance *surv) {
    demon_SurveillanceEvent_start(B);
    
    if(surv->event == INOTIFY) {
        demon_Inotify_ref_t inotify = create_inotify(B, (struct inotify_parameters*)surv->ptr_event);
        demon_SurveillanceEvent_event_Inotify_add(B, inotify);
    } 
    else if(surv->event == WATCH_SOCKET) {
        demon_TCPSocket_ref_t socket = create_tcp_socket(B, (struct tcp_socket*)surv->ptr_event);
        demon_SurveillanceEvent_event_TCPSocket_add(B, socket);
    }
    
    return demon_SurveillanceEvent_end(B);
}

void serialize_command(flatcc_builder_t *B, struct command *cmd) {
    flatbuffers_string_ref_t path = flatbuffers_string_create_str(B, cmd->path);
    flatbuffers_string_vec_ref_t args = flatbuffers_string_vec_create(B, cmd->args, cmd->args_size);
    flatbuffers_string_vec_ref_t envp = flatbuffers_string_vec_create(B, cmd->envp, cmd->envp_size);
    demon_SurveillanceEvent_vec_start(B);
    for(int i = 0; i < cmd->to_watch_size; i++) {
        demon_SurveillanceEvent_vec_push(B, create_surveillance_event(B, &cmd->to_watch[i]));
    }
    demon_SurveillanceEvent_vec_ref_t to_watch = demon_SurveillanceEvent_vec_end(B);
    demon_RunCommand_ref_t serialized_command = demon_RunCommand_create(B, path, args, envp, cmd->flags, cmd->stack_size, to_watch);

    //Creer le message final
    demon_Message_create_as_root(B, serialized_command);


}

//fonction pour envoyer une commande au démon
void send_command_to_demon(command *cmd){

    flatcc_builder_t builder;
    void *buffer;
    size_t size;
    flatcc_builder_init(&builder);
    serialize_command(&builder, cmd);

    //buffer est le message sérialisé a envoyer sur le réseau
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    //liberer l'espace mémoire du builder
    flatcc_builder_clear(&builder);

}

//fonction pour envoyer un killprocess au démon
void send_kill_to_demon(int32_t pid) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);
    demon_KillProcess_ref_t serialized_kill = demon_KillProcess_create(&builder, pid);
    demon_Message_create_as_root(&builder, serialized_kill);
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}


// Helper pour extraire un Inotify d'un SurveillanceEvent
static struct inotify_parameters* extract_inotify(demon_Inotify_table_t inotify) {
    struct inotify_parameters* params = malloc(sizeof(struct inotify_parameters));
    
    // Extraire root_paths
    const char* root_path = demon_Inotify_root_paths(inotify);
    params->root_paths = strdup(root_path);
    
    // Extraire trigger_events
    demon_InotifyEvent_vec_t events = demon_Inotify_trigger_events(inotify);
    size_t events_size = demon_InotifyEvent_vec_len(events);
    params->i_events = malloc(sizeof(InotifyEvent) * events_size);
    params->size = events_size;
    
    for(size_t i = 0; i < events_size; i++) {
        params->i_events[i] = demon_InotifyEvent_vec_at(events, i);
    }
    
    return params;
}

// Helper pour extraire un TCPSocket d'un SurveillanceEvent
static struct tcp_socket* extract_tcp_socket(demon_TCPSocket_table_t socket) {
    struct tcp_socket* tcp = malloc(sizeof(struct tcp_socket));
    tcp->destport = demon_TCPSocket_destport(socket);
    return tcp;
}

//désérialisation RunCommand
static struct command* receive_command(void *buffer, size_t size) {
    //on vérifie que le buffer est valide
    int verify_result = demon_Message_verify_as_root(buffer, size);
    if (!verify_result) {
        fprintf(stderr, "Error buffer verification failed\n");
        return;
    }

    demon_Message_table_t message = demon_Message_as_root(buffer);
    if(demon_Message_events_type(message) == demon_Event_RunCommand) {
        demon_RunCommand_table_t run_command = demon_Message_events(message);
    }

    //Reconstruction de la structure RunCommand
    const char* path = demon_RunCommand_path(run_command);
    flatbuffers_string_vec_t args = demon_RunCommand_args(run_command);
    size_t args_size = flatbuffers_string_vec_len(args);
    flatbuffers_string_vec_t envp = demon_RunCommand_envp(run_command);
    size_t envp_size = flatbuffers_string_vec_len(envp);
    uint32_t flags = demon_RunCommand_flags(run_command);
    uint32_t stack_size = demon_RunCommand_stack_size(run_command);
    demon_SurveillanceEvent_vec_t to_watch = demon_RunCommand_to_watch(run_command);
    size_t to_watch_size = demon_SurveillanceEvent_vec_len(to_watch);

    surveillance* to_watch_array = NULL;
    if(to_watch_size > 0) {
        to_watch_array = malloc(sizeof(surveillance) * to_watch_size);
    
        for(size_t i = 0; i < to_watch_size; i++) {
            demon_SurveillanceEvent_table_t surv_event = demon_SurveillanceEvent_vec_at(to_watch, i);
            demon_Surveillance_union_type_t type = demon_SurveillanceEvent_event_type(surv_event);
        
            // Remplir la structure surveillance
            if(type == demon_Surveillance_Inotify) {
                to_watch_array[i].event = INOTIFY;
                demon_Inotify_table_t inotify = demon_SurveillanceEvent_event_Inotify(surv_event);
                to_watch_array[i].ptr_event = extract_inotify(inotify);
            }
            else if(type == demon_Surveillance_TCPSocket) {
                to_watch_array[i].event = WATCH_SOCKET;
                demon_TCPSocket_table_t socket = demon_SurveillanceEvent_event_TCPSocket(surv_event);
                to_watch_array[i].ptr_event = extract_tcp_socket(socket);
            }
        }
    }
    command final_command;
    final_command = {path, args, args_size, envp, envp_size, flags, stack_size, to_watch_array, to_watch_size};
    return final_command;
}

//désérialisation KillProcess
int32_t receive_kill(void *buffer, size_t size) {
    demon_Message_table_t message = demon_Message_as_root(buffer);
    if(demon_Message_events_type(message) == demon_Event_KillProcess) {
        demon_KillProcess_table_t kill_process = demon_Message_events(message);
    }
    int32_t pid = demon_KillProcess_pid(kill_process);
}

//permet au démon de savoir si le message reçu est une commande ou un killprocess
Event receive_message_from_user(void *buffer, size_t size) {
    if(demon_Message_events_type(message) == demon_Event_RunCommand) {
        return RUN_COMMAND;
    }
    if(demon_Message_events_type(message) == demon_Event_KillProcess) {
        return KILL_PROCESS;
    }
    return NULL;
}


//
///PARTIE DEMON->USER
//

//fonction pour envoyer un processlaunched a l'user
void send_processlaunched_to_user(int32_t pid) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);
    demon_ProcessLaunched_ref_t process_launched = demon_ProcessLaunched_create(&builder, pid);
    demon_Message_create_as_root(&builder, process_launched);
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}

//fonction pour envoyer un childcreationerror a l'user
void send_childcreationerror_to_user(uint32_t errno) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);
    demon_ChildCreationError_ref_t child_creation_error = demon_ChildCreationError_create(&builder, errno);
    demon_Message_create_as_root(&builder, child_creation_error);
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}

//fonction pour envoyer un processterminaed a l'user
void send_processterminated_to_user(int32_t pid) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);
    demon_ProcessTerminated_ref_t process_terminated = demon_ProcessTerminated_create(&builder, pid);
    demon_Message_create_as_root(&builder, process_terminated);
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}

//fonction pour envoyer un TCPSocketListening a l'user
void send_tcpsocketlistening_to_user(uint16_t port) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);
    demon_TCPSocketListening_ref_t TCP_socket = demon_TCPSocketListening_create(&builder, port);
    demon_Message_create_as_root(&builder, TCP_socket);
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}

//fonction pour envoyer un processterminaed a l'user
void send_processterminated_to_user(int32_t pid) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);
    demon_ProcessTerminated_ref_t process_terminated = demon_ProcessTerminated_create(&builder, pid);
    demon_Message_create_as_root(&builder, process_terminated);
    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}

//fonction pour envoyer un inotifypathupdated a l'user
void send_inotifypahtupdated_to_user(struct inotify_parameters *inotify) {
    flatcc_builder_t builder;
    void* buffer;
    size_t size;

    flatcc_builder_init(&builder);

    demon_Inotify_start(&builder);
    demon_Inotify_root_paths_create_str(&builder, inotify->root_paths);
    demon_Inotify_trigger_events_create(&builder, inotify->i_events, inotify->size);
    demon_Inotify_size_add(&builder, inotify->size);
    demon_InotifyPathUpdated_ref_t path_updated = demon_Inotify_end(&builder);

    demon_Message_create_as_root(&builder, path_updated);

    buffer = flatcc_builder_finalize_buffer(&builder, &size);

    //Code d'utilisation du buffer (envoi du buffer sur le réseau par exemple)

    flatcc_builder_clear(&builder);
}

//il me reste a ajouter les fonctions de désérialisation des 5 messages si dessous 

//permet a l'user de savoir si le message reçu est une commande ou un killprocess
Event receive_message_from_demon(void *buffer, size_t size) {
    if(demon_Message_events_type(message) == demon_Event_ProcessLaunched) {
        return PROCESS_LAUNCHED;
    }
    if(demon_Message_events_type(message) == demon_Event_ChildCreationError) {
        return CHILD_CREATION_ERROR;
    }
    if(demon_Message_events_type(message) == demon_Event_ProcessTerminated) {
        return PROCESS_TERMINATED;
    }
    if(demon_Message_events_type(message) == demon_Event_TCPSocketListening) {
        return TCP_SOCKET_LISTENING;
    }
    if(demon_Message_events_type(message) == demon_Event_InotifyPathUpdated) {
        return INOTIFY_PATH_UPDATED;
    }
    return NULL;
}
