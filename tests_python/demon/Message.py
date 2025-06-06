# automatically generated by the FlatBuffers compiler, do not modify

# namespace: demon

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class Message(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = Message()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsMessage(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # Message
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # Message
    def EventsType(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint8Flags, o + self._tab.Pos)
        return 0

    # Message
    def Events(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(6))
        if o != 0:
            from flatbuffers.table import Table
            obj = Table(bytearray(), 0)
            self._tab.Union(obj, o)
            return obj
        return None

def MessageStart(builder):
    builder.StartObject(2)

def Start(builder):
    MessageStart(builder)

def MessageAddEventsType(builder, eventsType):
    builder.PrependUint8Slot(0, eventsType, 0)

def AddEventsType(builder, eventsType):
    MessageAddEventsType(builder, eventsType)

def MessageAddEvents(builder, events):
    builder.PrependUOffsetTRelativeSlot(1, flatbuffers.number_types.UOffsetTFlags.py_type(events), 0)

def AddEvents(builder, events):
    MessageAddEvents(builder, events)

def MessageEnd(builder):
    return builder.EndObject()

def End(builder):
    return MessageEnd(builder)
