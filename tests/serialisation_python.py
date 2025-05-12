import flatbuffers
import os
import sys

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


def serialisation():
    #Variables de RunCommand à envoyer au démon
    path = "echo"
    args = "bonjour test"
    envp = "oui non"
    flags = 0
    stack_size = 0

    #Variable TCPSocket
    destport = 42

    #Variables Inotify
    root_paths = "root"
    mask = 4
    size = 5




    #Traitement des [String]
    args = args.split()
    envp = envp.split()


    #Construction du builder
    bldr = flatbuffers.Builder(1024)

    path_bldr = bldr.CreateString(path)
    root_paths = bldr.CreateString(root_paths)

    #sérialisation des arguments
    liste = []
    for i in range(len(args)):
        liste.append(bldr.CreateString(args[i]))

    demon.RunCommand.StartArgsVector(bldr, len(args))
    for i in range(len(args)):
        bldr.PrependSOffsetTRelative(liste[i])
    args_bldr = bldr.EndVector()


    #sérialisation des envs
    liste = []
    for i in range(len(envp)):
        liste.append(bldr.CreateString(envp[i]))

    demon.RunCommand.StartEnvpVector(bldr, len(envp))
    for i in range(len(envp)):
        bldr.PrependSOffsetTRelative(liste[i])
    envp_bldr = bldr.EndVector()


    #sérialisation TCPSocket
    demon.TCPSocket.Start(bldr)
    demon.TCPSocket.AddDestport(bldr, destport)
    tcp_socket = demon.TCPSocket.End(bldr)

    
    #sérialisation Inotify
    demon.Inotify.Start(bldr)
    demon.Inotify.AddRootPaths(bldr, root_paths)
    demon.Inotify.AddMask(bldr, mask)
    demon.Inotify.AddSize(bldr, size)
    inotify = demon.Inotify.End(bldr)


    #sérialisation surveillance1
    demon.SurveillanceEvent.Start(bldr)
    demon.SurveillanceEvent.AddEventType(bldr, demon.Surveillance.Surveillance().TCPSocket)
    demon.SurveillanceEvent.AddEvent(bldr, tcp_socket)
    surveillance1 = demon.SurveillanceEvent.End(bldr)

    #sérialisation surveillance2
    demon.SurveillanceEvent.Start(bldr)
    demon.SurveillanceEvent.AddEventType(bldr, demon.Surveillance.Surveillance().Inotify)
    demon.SurveillanceEvent.AddEvent(bldr, inotify)
    surveillance2 = demon.SurveillanceEvent.End(bldr)


    #sérialisation de toWatch
    demon.RunCommand.StartToWatchVector(bldr, 2)
    bldr.PrependUOffsetTRelative(surveillance1)
    bldr.PrependUOffsetTRelative(surveillance2)
    to_watch = bldr.EndVector()


    #Serialisation RunCommand
    demon.RunCommand.Start(bldr)
    demon.RunCommand.AddPath(bldr, path_bldr)
    demon.RunCommand.AddArgs(bldr, args_bldr)
    demon.RunCommand.AddEnvp(bldr, envp_bldr)
    demon.RunCommand.AddFlags(bldr, flags)
    demon.RunCommand.AddStackSize(bldr, stack_size)
    demon.RunCommand.AddToWatch(bldr, to_watch)

    commande = demon.RunCommand.End(bldr)
    bldr.Finish(commande)

    buf = bldr.Output()

    #Pour tester l'objet envoyé
    #test = demon.RunCommand.RunCommand.GetRootAs(buf, 0)


