# automatically generated by the FlatBuffers compiler, do not modify

# namespace: demon

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class ExecveTerminated(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = ExecveTerminated()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsExecveTerminated(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # ExecveTerminated
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # ExecveTerminated
    def Pid(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Int32Flags, o + self._tab.Pos)
        return 0

    # ExecveTerminated
    def CommandName(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            return self._tab.String(o + self._tab.Pos)
        return None

    # ExecveTerminated
    def Success(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(8))
        if o != 0:
            return bool(self._tab.Get(flatbuffers.number_types.BoolFlags, o + self._tab.Pos))
        return False

def ExecveTerminatedStart(builder):
    builder.StartObject(3)

def Start(builder):
    ExecveTerminatedStart(builder)

def ExecveTerminatedAddPid(builder, pid):
    builder.PrependInt32Slot(0, pid, 0)

def AddPid(builder, pid):
    ExecveTerminatedAddPid(builder, pid)

def ExecveTerminatedAddCommandName(builder, commandName):
    builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(commandName), 0)

def AddCommandName(builder, commandName):
    ExecveTerminatedAddCommandName(builder, commandName)

def ExecveTerminatedAddSuccess(builder, success):
    builder.PrependBoolSlot(2, success, 0)

def AddSuccess(builder, success):
    ExecveTerminatedAddSuccess(builder, success)

def ExecveTerminatedEnd(builder):
    return builder.EndObject()

def End(builder):
    return ExecveTerminatedEnd(builder)
