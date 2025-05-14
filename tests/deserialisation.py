import flatbuffers
import os
import sys

import demon.ChildCreationError
import demon.ProcessLaunched
import demon.ProcessTerminated
import demon.Surveillance

sys.path.append(os.path.join(os.path.dirname(__file__), '../demon'))

import demon.Event
import demon.Inotify
import demon.EstablishTCPConnection
import demon.EstablishUnixConnection
import demon.Message
import demon.RunCommand
import demon.InotifyEvent
import demon.Event
import demon.TCPSocket
import demon.Surveillance
import demon.SurveillanceEvent


#Cette fonction gère le retour envoyé par le démon.
#Elle prend en paramètre le buffer retourné


def handler(buff):
    msg = demon.Message.Message.GetRootAsMessage(buff, 4)
    type_message = msg.EventsType()

    match type_message:
        case demon.Event.Event.ProcessLaunched:
            union_proc_launched = demon.ProcessLaunched.ProcessLaunched()
            union_proc_launched.Init(msg.Events().Bytes, msg.Events().Pos)

            pid = union_proc_launched.Pid()

            print("Processus lancé ", pid)

        case demon.Event.Event.ChildCreationError:
            union_child_crea_error = demon.ChildCreationError.ChildCreationError()
            union_child_crea_error.Init(msg.Events().Bytes, msg.Events().Pos)

            errno = union_child_crea_error.ErrorCode()

            print("Erreur lors du lancement du processus fils ", errno)

        case demon.Event.Event.ProcessTerminated:
            union_proc_terminated = demon.ProcessTerminated.ProcessTerminated()
            union_proc_terminated.Init(msg.Events().Bytes, msg.Events().Pos)

            pid = union_proc_terminated.Pid()
            errno = union_proc_terminated.ErrorCode()

            print("Processus terminé. ", pid)
            print("Code de retour ", errno)

        case _ :
            print("Reception d'un event inconnue")


#test
'''
bldr = flatbuffers.Builder(1024)

demon.ProcessLaunched.Start(bldr)
demon.ProcessLaunched.AddPid(bldr, 5)
pid_envoye = demon.ProcessLaunched.End(bldr)

demon.Message.Start(bldr)
demon.Message.AddEventsType(bldr, demon.Event.Event().ProcessLaunched)
demon.Message.AddEvents(bldr, pid_envoye)
termine = demon.Message.End(bldr)

bldr.Finish(termine)

buf = bldr.Output()

handler(buf)

'''
