//implementation de code C++ dans un projet C
//c/
//├── include/
//│   ├── serialize.h         // Header C++ original
//│   └── serialize_c.h       // Header C (interface)
//└── daemon-features/
//    ├── serialize.cpp       // Implémentation C++ originale
//    └── serialize_wrapper.cpp  // Implémentation du wrapper
//Dans le code C, il faudra utiliser les fonctions de serialize_wrapper.cpp


#include "../include/serialize_c.h"
#include "../include/serialize.h"

extern "C" {

    struct buffer_info* send_command_to_demon_c(command *cmd) {
        try {
            return send_command_to_demon(cmd);
        } catch (...) {
            return nullptr;
        }
    }

    struct buffer_info* send_kill_to_demon_c(int32_t pid) {
        try {
            return send_kill_to_demon(pid);
        } catch (...) {
            return nullptr;
        }
    }

    command* receive_command_c(uint8_t *buffer, int size) {
        try {
            return receive_command(buffer, size);
        } catch (...) {
            return nullptr;
        }
    }

    int32_t receive_kill_c(uint8_t *buffer, int size) {
        try {
            return receive_kill(buffer, size);
        } catch (...) {
            return -1;
        }
    }

    enum Event receive_message_from_user_c(uint8_t *buffer, int size) {
        try {
            return receive_message_from_user(buffer, size);
        } catch (...) {
            return NONE;
        }
    }

    struct buffer_info* send_processlaunched_to_user_c(int32_t pid) {
        try {
            return send_processlaunched_to_user(pid);
        } catch (...) {
            return nullptr;
        }
    }

    struct buffer_info* send_childcreationerror_to_user_c(uint32_t error_code) {
        try {
            return send_childcreationerror_to_user(error_code);
        } catch (...) {
            return nullptr;
        }
    }

    struct buffer_info* send_processterminated_to_user_c(int32_t pid, uint32_t error_code) {
        try {
            return send_processterminated_to_user(pid, error_code);
        } catch (...) {
            return nullptr;
        }
    }

    struct buffer_info* send_tcpsocketlistening_to_user_c(uint16_t port) {
        try {
            return send_tcpsocketlistening_to_user(port);
        } catch (...) {
            return nullptr;
        }
    }

    struct buffer_info* send_inotifypathupdated_to_user_c(InotifyPathUpdated *inotify) {
        try {
            return send_inotifypathupdated_to_user(inotify);
        } catch (...) {
            return nullptr;
        }
    }

    int32_t receive_processlaunched_c(uint8_t *buffer, int size) {
        try {
            return receive_processlaunched(buffer, size);
        } catch (...) {
            return -1;
        }
    }

    int32_t receive_childcreationerror_c(uint8_t *buffer, int size) {
        try {
            return receive_childcreationerror(buffer, size);
        } catch (...) {
            return -1;
        }
    }

    struct process_terminated_info* receive_processterminated_c(uint8_t *buffer, int size) {
        try {
            return receive_processterminated(buffer, size);
        } catch (...) {
            return nullptr;
        }
    }

    uint16_t receive_TCPSocketListening_c(uint8_t *buffer, int size) {
        try {
            return receive_TCPSocketListening(buffer, size);
        } catch (...) {
            return 0;
        }
    }

    struct InotifyPathUpdated* receive_inotifypathupdated_c(uint8_t *buffer, int size) {
        try {
            return receive_inotifypathupdated(buffer, size);
        } catch (...) {
            return nullptr;
        }
    }

    enum Event receive_message_from_demon_c(uint8_t *buffer, int size) {
        try {
            return receive_message_from_demon(buffer, size);
        } catch (...) {
            return NONE;
        }
    }

}