//Interface C pure

#ifndef SERIALIZE_C_H
#define SERIALIZE_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "data_structs.h"

//Déclaration des fonctions avec convention C
struct buffer_info* send_command_to_demon_c(command *cmd);
struct buffer_info* send_kill_to_demon_c(int32_t pid);
command* receive_command_c(uint8_t *buffer, int size);
int32_t receive_kill_c(uint8_t *buffer, int size);

enum Event receive_message_from_user_c(uint8_t *buffer, int size);

struct buffer_info* send_execveterminated_to_user_c(struct execve_info* info);
struct buffer_info* send_processlaunched_to_user_c(int32_t pid);
struct buffer_info* send_childcreationerror_to_user_c(uint32_t error_code);
struct buffer_info* send_processterminated_to_user_c(int32_t pid, uint32_t error_code);
struct buffer_info* send_tcpsocketlistening_to_user_c(uint16_t port);
struct buffer_info* send_inotifypathupdated_to_user_c(struct InotifyPathUpdated *inotify);
struct buffer_info* send_inotifywatchlistupdated_to_user_c(char *path);
struct buffer_info* send_socketwatched_to_user_c(int32_t port);
struct buffer_info* send_socketwatchterminated_to_user_c(struct socket_watch_info* socket_info);

struct execve_info* receive_execveterminated_c(uint8_t *buffer, int size);
int32_t receive_processlaunched_c(uint8_t *buffer, int size);
int32_t receive_childcreationerror_c(uint8_t *buffer, int size);
struct process_terminated_info* receive_processterminated_c(uint8_t *buffer, int size);
uint16_t receive_TCPSocketListening_c(uint8_t *buffer, int size);
struct InotifyPathUpdated* receive_inotifypathupdated_c(uint8_t *buffer, int size);
char* receive_inotifywatchlistupdated_c(uint8_t *buffer, int size);
int32_t receive_socketwatched_c(uint8_t *buffer, int size);
struct socket_watch_info* receive_socketwatchterminated_c(uint8_t *buffer, int size);

enum Event receive_message_from_demon_c(uint8_t *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif // SERIALIZE_C_H