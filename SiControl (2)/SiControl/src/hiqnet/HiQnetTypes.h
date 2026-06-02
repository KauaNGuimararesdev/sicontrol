#pragma once
#include <cstdint>
#include <QByteArray>
#include <QString>

// ---------------------------------------------------------------------------
//  HiQnet protocol primitives.
//
//  Grounded in the Harman "HiQnet Third Party Programmers Guide".
//  All multi-byte fields travel big-endian (network byte order).
//  Devices listen on TCP port 3804.
// ---------------------------------------------------------------------------

namespace hiqnet {

static constexpr quint16 kTcpPort      = 3804;
static constexpr quint8  kVersion      = 2;
static constexpr quint8  kHopCount      = 5;
static constexpr quint8  kBaseHeaderLen = 25;   // header with no variable extension

// Message IDs (methods the destination node must perform).
enum class MessageId : quint16 {
    Discovery        = 0x0000,
    Hello            = 0x0008,
    MultiParamSet    = 0x0100,
    MultiParamGet    = 0x0103,
    MultiParamSubscribe   = 0x010F,
    ParamSubscribeAll     = 0x0113,
    ParamUnsubscribeAll   = 0x0114,
    GetAttributes    = 0x010D,
};

// Header flag bits (see guide). Values are the *whole* flags field.
namespace Flags {
    static constexpr quint16 None        = 0x0000;
    static constexpr quint16 Error       = 0x0008; // bit 3
    static constexpr quint16 Information = 0x0010; // bit 4  (a.k.a. "Info")
    static constexpr quint16 Guaranteed  = 0x0020; // bit 5
}

// HiQnet datatype enumeration (payload value encoding).
enum class DataType : quint8 {
    BYTE = 0, UBYTE = 1, WORD = 2, UWORD = 3,
    LONG = 4, ULONG = 5, FLOAT32 = 6, FLOAT64 = 7
};

// A full HiQnet address: a 16-bit Node plus a 32-bit VD-Object field.
//   VD-Object high byte  = Virtual Device index
//   VD-Object low 3 bytes= Object address
struct Address {
    quint16 node = 0;
    quint8  virtualDevice = 0;
    quint32 object = 0;          // 24-bit object address

    quint32 vdObject() const {
        return (quint32(virtualDevice) << 24) | (object & 0x00FFFFFF);
    }
    static Address fromVdObject(quint16 n, quint32 vdObj) {
        return Address{ n, quint8((vdObj >> 24) & 0xFF), vdObj & 0x00FFFFFF };
    }
    bool operator==(const Address& o) const {
        return node == o.node && virtualDevice == o.virtualDevice && object == o.object;
    }
};

// Decoded HiQnet message header.
struct Header {
    quint8   version      = kVersion;
    quint8   headerLength = kBaseHeaderLen;
    quint32  messageLength = 0;
    Address  source;
    Address  destination;
    quint16  messageId    = 0;
    quint16  flags        = Flags::Guaranteed;
    quint8   hopCount     = kHopCount;
    quint16  sequence     = 0;
};

} // namespace hiqnet
