import flatbuffers
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../demon'))

import demon.Event
import demon.Inotify
import demon.EstablishTCPConnection
import demon.EstablishUnixConnection
import demon.Message
import demon.RunCommand
import demon.InotifyEvent
import demon.Event


def serialisation(path, args, envp, flags, stack_size):
    #Variables de RunCommand à envoyer au démon
    args = args.split()
    envp = envp.split()


    #Construction du builder
    bldr = flatbuffers.Builder(1024)

    path_bldr = bldr.CreateString(path)

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


    #Serialisation RunCommand
    demon.RunCommand.Start(bldr)
    demon.RunCommand.AddPath(bldr, path_bldr)
    demon.RunCommand.AddArgs(bldr, args_bldr)
    demon.RunCommand.AddEnvp(bldr, envp_bldr)
    demon.RunCommand.AddFlags(bldr, flags)
    demon.RunCommand.AddStackSize(bldr, stack_size)

    commande = demon.RunCommand.End(bldr)
    bldr.Finish(commande)

    buf = bldr.Output()

    #Pour tester l'objet envoyé
    test = demon.RunCommand.RunCommand.GetRootAs(buf, 0)
    print(test.Path())
    print(test.Envp(0))


serialisation("echo", "test bonjour", "oui non", 0, 0)
