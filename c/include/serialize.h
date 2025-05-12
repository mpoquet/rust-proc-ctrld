#ifndef SERIALIZE_H
#define SERIALIZE_H

#include "data_structs.h"

buffer_info* send_command_to_demon(command *cmd);
buffer_info* send_kill_to_demon(int32_t pid);
command* receive_command(uint8_t *buffer, int size);
int32_t receive_kill(uint8_t *buffer, int size);

Event receive_message_from_user(uint8_t *buffer, int size);

buffer_info* send_processlaunched_to_user(int32_t pid);
buffer_info* send_childcreationerror_to_user(uint32_t error_code);
buffer_info* send_processterminated_to_user(int32_t pid, uint32_t error_code);
buffer_info* send_tcpsocketlistening_to_user(uint16_t port);
buffer_info* send_inotifypathupdated_to_user(struct InotifyPathUpdated *inotify);

int32_t receive_processlaunched(uint8_t *buffer, int size);
int32_t receive_childcreationerror(uint8_t *buffer, int size);
process_terminated_info* receive_processterminated(uint8_t *buffer, int size);
uint16_t receive_TCPSocketListening(uint8_t *buffer, int size);
InotifyPathUpdated* receive_inotifypathupdated(uint8_t *buffer, int size);

Event receive_message_from_demon(uint8_t *buffer, int size);

#endif // SERIALIZE_H