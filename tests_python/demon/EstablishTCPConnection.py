# automatically generated by the FlatBuffers compiler, do not modify

# namespace: demon

import flatbuffers
from flatbuffers.compat import import_numpy
np = import_numpy()

class EstablishTCPConnection(object):
    __slots__ = ['_tab']

    @classmethod
    def GetRootAs(cls, buf, offset=0):
        n = flatbuffers.encode.Get(flatbuffers.packer.uoffset, buf, offset)
        x = EstablishTCPConnection()
        x.Init(buf, n + offset)
        return x

    @classmethod
    def GetRootAsEstablishTCPConnection(cls, buf, offset=0):
        """This method is deprecated. Please switch to GetRootAs."""
        return cls.GetRootAs(buf, offset)
    # EstablishTCPConnection
    def Init(self, buf, pos):
        self._tab = flatbuffers.table.Table(buf, pos)

    # EstablishTCPConnection
    def Destport(self):
        o = flatbuffers.number_types.UOffsetTFlags.py_type(self._tab.Offset(4))
        if o != 0:
            return self._tab.Get(flatbuffers.number_types.Uint32Flags, o + self._tab.Pos)
        return 0

def EstablishTCPConnectionStart(builder):
    builder.StartObject(1)

def Start(builder):
    EstablishTCPConnectionStart(builder)

def EstablishTCPConnectionAddDestport(builder, destport):
    builder.PrependUint32Slot(0, destport, 0)

def AddDestport(builder, destport):
    EstablishTCPConnectionAddDestport(builder, destport)

def EstablishTCPConnectionEnd(builder):
    return builder.EndObject()

def End(builder):
    return EstablishTCPConnectionEnd(builder)
