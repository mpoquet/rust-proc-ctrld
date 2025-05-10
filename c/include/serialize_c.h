//Interface C pure

#ifndef SERIALIZE_C_H
#define SERIALIZE_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "serialize.h" //Pour réutiliser les structures existantes

//Déclaration des fonctions avec convention C
buffer_info* send_command_to_demon_c(command *cmd);
buffer_info* send_kill_to_demon_c(int32_t pid);
command* receive_command_c(uint8_t *buffer, int size);
int32_t receive_kill_c(uint8_t *buffer, int size);

Event receive_message_from_user_c(uint8_t *buffer, int size);

buffer_info* send_processlaunched_to_user_c(int32_t pid);
buffer_info* send_childcreationerror_to_user_c(uint32_t error_code);
buffer_info* send_processterminated_to_user_c(int32_t pid, uint32_t error_code);
buffer_info* send_tcpsocketlistening_to_user_c(uint16_t port);
buffer_info* send_inotifypathupdated_to_user_c(InotifyPathUpdated *inotify);

int32_t receive_processlaunched_c(uint8_t *buffer, int size);
int32_t receive_childcreationerror_c(uint8_t *buffer, int size);
process_terminated_info* receive_processterminated_c(uint8_t *buffer, int size);
uint16_t receive_TCPSocketListening_c(uint8_t *buffer, int size);
InotifyPathUpdated* receive_inotifypathupdated_c(uint8_t *buffer, int size);

Event receive_message_from_demon_c(uint8_t *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif // SERIALIZE_C_H