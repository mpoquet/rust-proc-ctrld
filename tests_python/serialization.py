import flatbuffers
from typing import List, Optional, Any
from dataclasses import dataclass
from enum import Enum, auto
import demon.ExecveTerminated
import demon.RunCommand
import demon.Message
import demon.KillProcess
import demon.ProcessLaunched
import demon.ChildCreationError
import demon.ProcessTerminated
import demon.TCPSocketListening
import demon.InotifyPathUpdated
import demon.InotifyWatchListUpdated
import demon.SocketWatched
import demon.SocketWatchTerminated
import demon.Event
import demon.Inotify 
import demon.Surveillance
import demon.TCPSocket
import demon.SurveillanceEvent 

from dataclasses import dataclass
from typing import List, Optional, Any
from enum import Enum, auto

@dataclass
class TCPSocket:
    destport: int

@dataclass
class BufferInfo:
    buffer: bytes
    size: int

@dataclass
class ExecveInfo:
    pid: int
    command_name: str
    success: bool

@dataclass
class ProcessTerminatedInfo:
    pid: int
    error_code: int

class SocketState(Enum):
    SOCKET_UNKNOWN = 0
    SOCKET_LISTENING = auto()
    SOCKET_CREATED = auto()

@dataclass
class SocketWatchInfo:
    port: int
    state: SocketState

class InotifyEvent(Enum):
    MODIFIED = 0
    CREATED = auto()
    SIZE_REACHED = auto()
    DELETED = auto()
    ACCESSED = auto()

@dataclass
class InotifyParameters:
    root_paths: str
    mask: int
    size: int

class SurveillanceEventType(Enum):
    INOTIFY = 0
    WATCH_SOCKET = auto()

@dataclass
class Surveillance:
    event: SurveillanceEventType
    ptr_event: Any  # Can be InotifyParameters or TCPSocket

@dataclass
class InotifyPathUpdated:
    path: str
    event: InotifyEvent
    size: int
    size_limit: int

@dataclass
class Command:
    path: str
    args: List[str]
    envp: List[str]
    flags: int
    stack_size: int
    to_watch: List[Surveillance]

class Event(Enum):
    NONE = -1
    RUN_COMMAND = 0
    KILL_PROCESS = 1
    ESTABLISH_TCP_CONNECTION = 2
    ESTABLISH_UNIX_CONNECTION = 3
    PROCESS_LAUNCHED = 4
    CHILD_CREATION_ERROR = 5
    PROCESS_TERMINATED = 6
    TCP_SOCKET_LISTENING = 7
    INOTIFY_PATH_UPDATED = 8
    INOTIFY_WATCH_LIST_UPDATED = 9
    SOCKET_WATCHED = 10
    SOCKET_WATCH_TERMINATED = 11

def send_command_to_demon(cmd: Command) -> BufferInfo:
    builder = flatbuffers.Builder(1024)
    
    # Create string arrays
    args_offsets = [builder.CreateString(arg) for arg in cmd.args]
    envp_offsets = [builder.CreateString(env) for env in cmd.envp]
    path_offset = builder.CreateString(cmd.path)
    
    # Create surveillances vector first to avoid side effects
    surveillance_offsets = []
    for surv in cmd.to_watch:
        if surv.event == SurveillanceEventType.INOTIFY:
            inotify = surv.ptr_event
            root_paths_offset = builder.CreateString(inotify.root_paths)
            
            # Create Inotify table
            demon.Inotify.Start(builder)
            demon.Inotify.AddRootPaths(builder, root_paths_offset)
            demon.Inotify.AddMask(builder, inotify.mask)
            demon.Inotify.AddSize(builder, inotify.size)
            inotify_offset = demon.Inotify.End(builder)
            
            # Create SurveillanceEvent
            demon.SurveillanceEvent.Start(builder)
            demon.SurveillanceEvent.AddEventType(builder, demon.Surveillance.Surveillance.Inotify)
            demon.SurveillanceEvent.AddEvent(builder, inotify_offset)
            surveillance_offsets.append(demon.SurveillanceEvent.End(builder))
            
        elif surv.event == SurveillanceEventType.WATCH_SOCKET:
            socket = surv.ptr_event
            
            # Create TCPSocket table
            demon.TCPSocket.Start(builder)
            demon.TCPSocket.AddDestport(builder, socket.destport)
            socket_offset = demon.TCPSocket.End(builder)
            
            # Create SurveillanceEvent
            demon.SurveillanceEvent.Start(builder)
            demon.SurveillanceEvent.AddEventType(builder, demon.Surveillance.Surveillance.TCPSocket)
            demon.SurveillanceEvent.AddEvent(builder, socket_offset)
            surveillance_offsets.append(demon.SurveillanceEvent.End(builder))
            
    # Create vectors
    demon.RunCommand.RunCommandStartArgsVector(builder, len(args_offsets))
    for arg in reversed(args_offsets):
        builder.PrependUOffsetTRelative(arg)
    args_vector = builder.EndVector()
    
    demon.RunCommand.RunCommandStartEnvpVector(builder, len(envp_offsets))
    for env in reversed(envp_offsets):
        builder.PrependUOffsetTRelative(env)
    envp_vector = builder.EndVector()
    
    demon.RunCommand.RunCommandStartToWatchVector(builder, len(surveillance_offsets))
    for surv in reversed(surveillance_offsets):
        builder.PrependUOffsetTRelative(surv)
    surveillance_vector = builder.EndVector()
    
    # Create RunCommand with all fields
    demon.RunCommand.Start(builder)
    demon.RunCommand.AddPath(builder, path_offset)
    demon.RunCommand.AddArgs(builder, args_vector)
    demon.RunCommand.AddEnvp(builder, envp_vector)
    demon.RunCommand.AddFlags(builder, cmd.flags)
    demon.RunCommand.AddStackSize(builder, cmd.stack_size)
    demon.RunCommand.AddToWatch(builder, surveillance_vector)
    run_command = demon.RunCommand.End(builder)
    
    # Create Message
    demon.Message.Start(builder)
    demon.Message.AddEventsType(builder, demon.Event.Event.RunCommand)
    demon.Message.AddEvents(builder, run_command)
    message = demon.Message.End(builder)
    
    builder.Finish(message)
    buf = builder.Output()
    return BufferInfo(buffer=buf, size=len(buf))

def send_kill_to_demon(pid: int) -> BufferInfo:
    builder = flatbuffers.Builder(1024)
    
    demon.KillProcess.Start(builder)
    demon.KillProcess.AddPid(builder, pid)
    kill_process = demon.KillProcess.End(builder)
    
    demon.Message.Start(builder)
    demon.Message.AddEventsType(builder, demon.Event.Event.KillProcess)
    demon.Message.AddEvents(builder, kill_process)
    message = demon.Message.End(builder)
    
    builder.Finish(message)
    buf = builder.Output()
    return BufferInfo(buffer=buf, size=len(buf))

def receive_command(buffer: bytes, size: int) -> Optional[Command]:
    """Deserialize a RunCommand message from a FlatBuffer"""
    if not buffer or size <= 0:
        return None

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        if message.EventsType() != demon.Event.Event.RunCommand:
            return None

        # Get RunCommand table
        run_command = demon.RunCommand.RunCommand()
        run_command.Init(message.Events().Bytes, message.Events().Pos)

        try:
            # Create Command object
            final_command = Command(
                path=run_command.Path().decode('utf-8'),
                args=[arg.decode('utf-8') for arg in run_command.ArgsAsNumpy()],
                envp=[env.decode('utf-8') for env in run_command.EnvpAsNumpy()],
                flags=run_command.Flags(),
                stack_size=run_command.StackSize(),
                to_watch=[]
            )

            # Process surveillances if present
            surveillances = run_command.ToWatch()
            if surveillances:
                for i in range(surveillances.Length()):
                    surveillance_event = surveillances.Get(i)
                    
                    if surveillance_event.EventType() == demon.Surveillance.Surveillance.Inotify:
                        inotify_event = surveillance_event.Event()
                        inotify_params = InotifyParameters(
                            root_paths=inotify_event.RootPaths().decode('utf-8'),
                            mask=inotify_event.Mask(),
                            size=inotify_event.Size()
                        )
                        final_command.to_watch.append(
                            Surveillance(event="INOTIFY", ptr_event=inotify_params)
                        )
                    
                    elif surveillance_event.EventType() == demon.Surveillance.Surveillance.TCPSocket:
                        socket_event = surveillance_event.Event()
                        socket_params = TCPSocket(
                            destport=socket_event.Destport()
                        )
                        final_command.to_watch.append(
                            Surveillance(event="WATCH_SOCKET", ptr_event=socket_params)
                        )

            return final_command

        except Exception as e:
            print(f"Error deserializing command data: {e}")
            return None
        
    except Exception as e:
        print(f"Error deserializing command buffer: {e}")
        return None

def receive_kill(buffer: bytes, size: int) -> int:
    """Deserialize a KillProcess message from a FlatBuffer"""
    if not buffer or size <= 0:
        return -1

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        if message.EventsType() != demon.Event.Event.KillProcess:
            return -1

        # Get KillProcess table and return pid
        kill_process = demon.KillProcess.KillProcess()
        kill_process.Init(message.Events().Bytes, message.Events().Pos)
        
        return kill_process.Pid()
    
    except Exception as e:
        print(f"Error deserializing kill: {e}")
        return -1

def receive_message_from_user(buffer: bytes, size: int) -> Event:
    """Determine the type of message received from user"""
    if not buffer or size <= 0:
        return Event.NONE

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
    
        if message.EventsType() == demon.Event.Event.RunCommand:
            return Event.RUN_COMMAND
        elif message.EventsType() == demon.Event.Event.KillProcess:
            return Event.KILL_PROCESS
    
        return Event.NONE
    
    except Exception as e:
        print(f"Buffer error in receive_message_from_user: {e}")
        return -1

def send_processlaunched_to_user(pid: int) -> BufferInfo:
    """Serialize a ProcessLaunched message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create ProcessLaunched table
        demon.ProcessLaunched.Start(builder)
        demon.ProcessLaunched.AddPid(builder, pid)
        process_launched = demon.ProcessLaunched.End(builder)
        
        # Create Message with ProcessLaunched
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.ProcessLaunched)
        demon.Message.AddEvents(builder, process_launched)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing ProcessLaunched: {e}")
        return None
    
def send_childcreationerror_to_user(error_code: int) -> BufferInfo:
    """Serialize a ChildCreationError message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create ChildCreationError table
        demon.ChildCreationError.Start(builder)
        demon.ChildCreationError.AddErrorCode(builder, error_code)
        child_creation_error = demon.ChildCreationError.End(builder)
        
        # Create Message with ChildCreationError
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.ChildCreationError)
        demon.Message.AddEvents(builder, child_creation_error)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing ChildCreationError: {e}")
        return None
    
def send_processterminated_to_user(pid: int, error_code: int) -> BufferInfo:
    """Serialize a ProcessTerminated message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create ProcessTerminated table
        demon.ProcessTerminated.Start(builder)
        demon.ProcessTerminated.AddPid(builder, pid)
        demon.ProcessTerminated.AddErrorCode(builder, error_code)
        process_terminated = demon.ProcessTerminated.End(builder)
        
        # Create Message with ProcessTerminated
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.ProcessTerminated)
        demon.Message.AddEvents(builder, process_terminated)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing ProcessTerminated: {e}")
        return None
    
def send_tcpsocketlistening_to_user(port: int) -> BufferInfo:
    """Serialize a TCPSocketListening message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create TCPSocketListening table
        demon.TCPSocketListening.Start(builder)
        demon.TCPSocketListening.AddPort(builder, port)
        tcp_socket_listening = demon.TCPSocketListening.End(builder)
        
        # Create Message with TCPSocketListening
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.TCPSocketListening)
        demon.Message.AddEvents(builder, tcp_socket_listening)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing TCPSocketListening: {e}")
        return None
    
def send_inotifypathupdated_to_user(inotify: InotifyPathUpdated) -> BufferInfo:
    """Serialize an InotifyPathUpdated message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create string for path
        path_str = builder.CreateString(inotify.path)
        
        # Create InotifyPathUpdated table
        demon.InotifyPathUpdated.Start(builder)
        demon.InotifyPathUpdated.AddPath(builder, path_str)
        demon.InotifyPathUpdated.AddTriggerEvents(
            builder, 
            demon.InotifyEvent(inotify.event)
        )
        demon.InotifyPathUpdated.AddSize(builder, inotify.size)
        demon.InotifyPathUpdated.AddSizeLimit(builder, inotify.size_limit)
        inotify_path_updated = demon.InotifyPathUpdated.End(builder)
        
        # Create Message with InotifyPathUpdated
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.InotifyPathUpdated)
        demon.Message.AddEvents(builder, inotify_path_updated)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing InotifyPathUpdated: {e}")
        return None
    
def send_inotifywatchlistupdated_to_user(path: str) -> BufferInfo:
    """Serialize an InotifyWatchListUpdated message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create string for path
        path_str = builder.CreateString(path)
        
        # Create InotifyWatchListUpdated table
        demon.InotifyWatchListUpdated.Start(builder)
        demon.InotifyWatchListUpdated.AddPath(builder, path_str)
        inotify_watch_list_updated = demon.InotifyWatchListUpdated.End(builder)
        
        # Create Message with InotifyWatchListUpdated
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.InotifyWatchListUpdated)
        demon.Message.AddEvents(builder, inotify_watch_list_updated)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing InotifyWatchListUpdated: {e}")
        return None
    
def send_socketwatched_to_user(port: int) -> BufferInfo:
    """Serialize a SocketWatched message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create SocketWatched table
        demon.SocketWatched.Start(builder)
        demon.SocketWatched.AddPort(builder, port)
        socket_watched = demon.SocketWatched.End(builder)
        
        # Create Message with SocketWatched
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.SocketWatched)
        demon.Message.AddEvents(builder, socket_watched)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing SocketWatched: {e}")
        return None
    
def send_socketwatchterminated_to_user(socket_info: SocketWatchInfo) -> BufferInfo:
    """Serialize a SocketWatchTerminated message to a FlatBuffer"""
    builder = flatbuffers.Builder(1024)
    
    try:
        # Create SocketWatchTerminated table
        socket_watch_terminated = demon.CreateSocketWatchTerminated(
            builder,
            socket_info.port,
            demon.SocketState(socket_info.state.value)  # Convert Python enum to FlatBuffer enum
        )
        
        # Create Message with SocketWatchTerminated
        demon.Message.Start(builder)
        demon.Message.AddEventsType(builder, demon.Event.Event.SocketWatchTerminated)
        demon.Message.AddEvents(builder, socket_watch_terminated)
        message = demon.Message.End(builder)
        
        builder.Finish(message)
        
        # Get the built buffer
        buf = builder.Output()
        return BufferInfo(buffer=buf, size=len(buf))
        
    except Exception as e:
        print(f"Error serializing SocketWatchTerminated: {e}")
        return None
    
def receive_execveterminated(buffer: bytes, size: int) -> Optional[ExecveInfo]:
    """Deserialize an ExecveTerminated message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        ExecveInfo object or None if error
    """
    if not buffer or size <= 0:
        return None

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        # print(f"ExecveTerminated event type (numeric): {message.EventsType()}")
        # print(f"Expected event type (numeric): {demon.Event.Event.ExecveTerminated}")
        # print(f"ProcessTerminated event type (string): {demon.Event.Event.ProcessTerminated}")
        if message.EventsType() != demon.Event.Event.ExecveTerminated:
            print("la")
            return None

        # Get ExecveTerminated table and create info
        execve_terminated = demon.ExecveTerminated.ExecveTerminated()
        execve_terminated.Init(message.Events().Bytes, message.Events().Pos)
        
        return ExecveInfo(
            pid=execve_terminated.Pid(),
            command_name=execve_terminated.CommandName().decode('utf-8'),
            success=execve_terminated.Success()
        )
    
    except Exception as e:
        print(f"Error deserializing ExecveTerminated: {e}")
        return None
    
def receive_processlaunched(buffer: bytes, size: int) -> int:
    """Deserialize a ProcessLaunched message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        Process ID or -1 if error
    """
    if not buffer or size <= 0:
        return -1

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        # actual_type = message.EventsType()
        # print(f"Actual event type (numeric): {actual_type}")
        # print(f"ProcessLaunched type (numeric): {demon.Event.Event.ProcessLaunched}")
        if message.EventsType() != demon.Event.Event.ProcessLaunched:
            return -1

        # Get ProcessLaunched table and return pid
        process_launched = demon.ProcessLaunched.ProcessLaunched()
        process_launched.Init(message.Events().Bytes, message.Events().Pos)
        
        return process_launched.Pid()
    
    except Exception as e:
        print(f"Error deserializing ProcessLaunched: {e}")
        return -1

def receive_childcreationerror(buffer: bytes, size: int) -> int:
    """Deserialize a ChildCreationError message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        Error code or -1 if error
    """
    if not buffer or size <= 0:
        return -1

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        if message.EventsType() != demon.Event.Event.ChildCreationError:
            return -1

        # Get ChildCreationError table and return error code
        child_creation_error = demon.ChildCreationError.ChildCreationError()
        child_creation_error.Init(message.Events().Bytes, message.Events().Pos)
        
        return child_creation_error.ErrorCode()
    
    except Exception as e:
        print(f"Error deserializing ChildCreationError: {e}")
        return -1
    
def receive_processterminated(buffer: bytes, size: int) -> Optional[ProcessTerminatedInfo]:
    """Deserialize a ProcessTerminated message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        ProcessTerminatedInfo object or None if error
    """
    if not buffer or size <= 0:
        return None

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        # print(f"Actual event type (numeric): {message.EventsType()}")
        # print(f"ProcessLaunched type (numeric): {demon.Event.Event.ProcessLaunched}")
        if message.EventsType() != demon.Event.Event.ProcessTerminated:
            return None

        # Get ProcessTerminated table and return info
        process_terminated = demon.ProcessTerminated.ProcessTerminated()
        process_terminated.Init(message.Events().Bytes, message.Events().Pos)

        return ProcessTerminatedInfo(
            pid=process_terminated.Pid(),
            error_code=process_terminated.ErrorCode()
        )
    
    except Exception as e:
        print(f"Error deserializing ProcessTerminated: {e}")
        return -1

def receive_TCPSocketListening(buffer: bytes, size: int) -> int:
    """Deserialize a TCPSocketListening message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        Port number or -1 if error
    """
    if not buffer or size <= 0:
        return -1

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        actual_type = message.EventsType()
        print(f"Actual event type (numeric): {actual_type}")
        print(f"TCPSocketListening type (numeric): {demon.Event.Event.TCPSocketListening}")
        if message.EventsType() != demon.Event.Event.TCPSocketListening:
            return -1

        # Get TCPSocketListening table and return port
        tcp_socket_listening = demon.TCPSocketListening.TCPSocketListening()
        tcp_socket_listening.Init(message.Events().Bytes, message.Events().Pos)
        
        return tcp_socket_listening.Port()
    
    except Exception as e:
        print(f"Error deserializing TCPSocketListening: {e}")
        return -1

def receive_inotifypathupdated(buffer: bytes, size: int) -> Optional[InotifyPathUpdated]:
    """Deserialize an InotifyPathUpdated message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        InotifyPathUpdated object or None if error
    """
    if not buffer or size <= 0:
        return None

    try:   
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        actual_type = message.EventsType()
        print(f"Actual event type (numeric): {actual_type}")
        print(f"InotifyPathUpdated type (numeric): {demon.Event.Event.InotifyPathUpdated}")
        if message.EventsType() != demon.Event.Event.InotifyPathUpdated:
            return None

        # Get InotifyPathUpdated table
        inotify_path_updated = demon.InotifyPathUpdated.InotifyPathUpdated()
        inotify_path_updated.Init(message.Events().Bytes, message.Events().Pos)
        
        # Create and return InotifyPathUpdated object
        return InotifyPathUpdated(
            path=inotify_path_updated.Path().decode('utf-8'),
            event=InotifyEvent(inotify_path_updated.TriggerEvents()),
            size=inotify_path_updated.Size(),
            size_limit=inotify_path_updated.SizeLimit()
        )
    
    except Exception as e:
        print(f"Error deserializing InotifyPathUpdated: {e}")
        return None
    
def receive_inotifywatchlistupdated(buffer: bytes, size: int) -> Optional[str]:
    """Deserialize an InotifyWatchListUpdated message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        Path string or None if error
    """
    if not buffer or size <= 0:
        return None

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        actual_type = message.EventsType()
        print(f"Actual event type (numeric): {actual_type}")
        print(f"InotifyWatchListUpdated type (numeric): {demon.Event.Event.InotifyWatchListUpdated}")
        if message.EventsType() != demon.Event.Event.InotifyWatchListUpdated:
            return None

        # Get InotifyWatchListUpdated table and return path
        inotify_watch_list_updated = demon.InotifyWatchListUpdated.InotifyWatchListUpdated()
        inotify_watch_list_updated.Init(message.Events().Bytes, message.Events().Pos)
        
        return inotify_watch_list_updated.Path().decode('utf-8')

    except Exception as e:
        print(f"Error deserializing InotifyWatchListUpdated: {e}")
        return None

def receive_socketwatched(buffer: bytes, size: int) -> int:
    """Deserialize a SocketWatched message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        Port number or -1 if error
    """
    if not buffer or size <= 0:
        return -1

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        actual_type = message.EventsType()
        print(f"Actual event type (numeric): {actual_type}")
        print(f"socketwatched type (numeric): {demon.Event.Event.SocketWatched}")
        if message.EventsType() != demon.Event.Event.SocketWatched:
            return -1

        # Get SocketWatched table and return port
        socket_watched = demon.SocketWatched.SocketWatched()
        socket_watched.Init(message.Events().Bytes, message.Events().Pos)
        
        return socket_watched.Port()
    
    except Exception as e:
        print(f"Error deserializing SocketWatched: {e}")
        return -1

def receive_socketwatchterminated(buffer: bytes, size: int) -> Optional[SocketWatchInfo]:
    """Deserialize a SocketWatchTerminated message from a FlatBuffer
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        SocketWatchInfo object or None if error
    """
    if not buffer or size <= 0:
        return None

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
        actual_type = message.EventsType()
        print(f"Actual event type (numeric): {actual_type}")
        print(f"socketwatchterminated type (numeric): {demon.Event.Event.SocketWatchTerminated}")
        if message.EventsType() != demon.Event.Event.SocketWatchTerminated:
            return None

        # Get SocketWatchTerminated table and create info
        socket_watch_terminated = demon.SocketWatchTerminated.SocketWatchTerminated()
        socket_watch_terminated.Init(message.Events().Bytes, message.Events().Pos)

        return SocketWatchInfo(
            port=socket_watch_terminated.Port(),
            state=SocketState(socket_watch_terminated.State())
        )
    
    except Exception as e:
        print(f"Error deserializing SocketWatchTerminated: {e}")
        return None

def receive_message_from_demon(buffer: bytes, size: int) -> Event:
    """Determine the type of message received from demon
    
    Args:
        buffer: Bytes containing the FlatBuffer message
        size: Size of the buffer
        
    Returns:
        Event enum indicating the message type
    """
    if not buffer or size <= 0:
        return Event.NONE

    try:
        # Get message and check type
        message = demon.Message.Message.GetRootAsMessage(buffer, 0)
    
        # Map FlatBuffer event types to our Event enum
        event_map = {
            demon.Event.Event.ProcessLaunched: Event.PROCESS_LAUNCHED,
            demon.Event.Event.ChildCreationError: Event.CHILD_CREATION_ERROR,
            demon.Event.Event.ProcessTerminated: Event.PROCESS_TERMINATED,
            demon.Event.Event.TCPSocketListening: Event.TCP_SOCKET_LISTENING,
            demon.Event.Event.InotifyPathUpdated: Event.INOTIFY_PATH_UPDATED,
            demon.Event.Event.InotifyWatchListUpdated: Event.INOTIFY_WATCH_LIST_UPDATED,
            demon.Event.Event.SocketWatched: Event.SOCKET_WATCHED,
            demon.Event.Event.SocketWatchTerminated: Event.SOCKET_WATCH_TERMINATED
        }

        return event_map.get(message.EventsType(), Event.NONE)
    
    except Exception as e:
        print(f"Buffer error in receive_message_from_demon: {e}")
        return -1