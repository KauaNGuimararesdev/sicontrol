#pragma once
#include "HiQnetTypes.h"
#include <QVector>
#include <optional>

namespace hiqnet {

// One parameter value inside a MultiParamSet/Get payload.
struct ParamValue {
    quint16  paramId = 0;
    DataType type    = DataType::LONG;
    qint64   raw     = 0;     // integer payloads
    double   real    = 0.0;   // FLOAT32/FLOAT64 payloads
    bool     isFloat() const { return type == DataType::FLOAT32 || type == DataType::FLOAT64; }
};

// Codec: turns logical HiQnet messages into wire bytes and back.
// Static, stateless helpers; the client owns sequencing.
class Codec {
public:
    // ---- low level big-endian writers -------------------------------------
    static void putU8 (QByteArray& b, quint8  v) { b.append(char(v)); }
    static void putU16(QByteArray& b, quint16 v) { b.append(char(v >> 8)); b.append(char(v)); }
    static void putU32(QByteArray& b, quint32 v) {
        b.append(char(v >> 24)); b.append(char(v >> 16));
        b.append(char(v >> 8));  b.append(char(v));
    }
    static quint16 getU16(const QByteArray& b, int o) {
        return (quint16(quint8(b[o])) << 8) | quint8(b[o + 1]);
    }
    static quint32 getU32(const QByteArray& b, int o) {
        return (quint32(quint8(b[o]))   << 24) | (quint32(quint8(b[o+1])) << 16) |
               (quint32(quint8(b[o+2])) << 8)  |  quint32(quint8(b[o+3]));
    }

    // Writes the 25-byte base header. messageLength is patched afterwards by
    // finalize() once the payload length is known.
    static void writeHeader(QByteArray& out, const Header& h) {
        putU8 (out, h.version);
        putU8 (out, h.headerLength);
        putU32(out, 0);                         // placeholder message length
        putU16(out, h.source.node);      putU32(out, h.source.vdObject());
        putU16(out, h.destination.node); putU32(out, h.destination.vdObject());
        putU16(out, h.messageId);
        putU16(out, h.flags);
        putU8 (out, h.hopCount);
        putU16(out, h.sequence);
    }

    // Patch the 4-byte message-length field (offset 2) to the final size.
    static void finalize(QByteArray& out) {
        quint32 len = quint32(out.size());
        out[2] = char(len >> 24); out[3] = char(len >> 16);
        out[4] = char(len >> 8);  out[5] = char(len);
    }

    // Decode just the header from an incoming buffer. Returns nullopt if the
    // buffer is too short to hold a base header.
    static std::optional<Header> readHeader(const QByteArray& b) {
        if (b.size() < kBaseHeaderLen) return std::nullopt;
        Header h;
        h.version      = quint8(b[0]);
        h.headerLength = quint8(b[1]);
        h.messageLength = getU32(b, 2);
        h.source       = Address::fromVdObject(getU16(b, 6),  getU32(b, 8));
        h.destination  = Address::fromVdObject(getU16(b, 12), getU32(b, 14));
        h.messageId    = getU16(b, 18);
        h.flags        = getU16(b, 20);
        h.hopCount     = quint8(b[22]);
        h.sequence     = getU16(b, 23);
        return h;
    }

    // ---- message builders --------------------------------------------------

    // Discovery / KeepAlive share one payload. Used as first message after
    // TCP connect, and re-sent periodically (with Info+Guaranteed) as keepalive.
    static QByteArray buildDiscovery(const Address& self, const Header& tmpl,
                                     quint8 macAddr[6], quint32 ipv4,
                                     quint32 subnet, quint32 gateway,
                                     bool keepAlive) {
        Header h = tmpl;
        h.source = self;
        h.destination = Address{};            // broadcast / unspecified
        h.messageId = quint16(MessageId::Discovery);
        h.flags = keepAlive ? (Flags::Guaranteed | Flags::Information)
                            :  Flags::Guaranteed;
        QByteArray out;
        writeHeader(out, h);
        // Discovery payload
        putU16(out, self.node);               // NODE
        putU8 (out, 1);                        // COST (1 over Ethernet)
        putU16(out, 16);                       // SERIAL NUMBER LENGTH
        for (int i = 0; i < 10; ++i) putU8(out, 0);
        for (int i = 0; i < 6;  ++i) putU8(out, macAddr[i]);   // serial = MAC tail
        putU32(out, 1048576);                  // MAX MESSAGE SIZE (1 MiB)
        putU16(out, 10000);                    // KEEPALIVE PERIOD (ms)
        putU8 (out, 1);                        // NETWORK ID (Ethernet)
        for (int i = 0; i < 6; ++i) putU8(out, macAddr[i]);    // MAC ADDRESS
        putU8 (out, 1);                        // DHCP
        putU32(out, ipv4);                     // IP ADDRESS
        putU32(out, subnet);                   // SUBNET MASK
        putU32(out, gateway);                  // GATEWAY
        finalize(out);
        return out;
    }

    // Refuse the device's Hello session so we can run session-less.
    static QByteArray buildHelloRefusal(const Address& self, const Address& device,
                                        quint16 sequence) {
        Header h;
        h.source = self;
        h.destination = device;
        h.messageId = quint16(MessageId::Hello);
        h.flags = Flags::Guaranteed | Flags::Error | Flags::Information;
        h.sequence = sequence;
        QByteArray out;
        writeHeader(out, h);
        putU16(out, 2);          // ERROR LENGTH
        putU16(out, 0x0000);     // ERROR MESSAGE (no error code)
        finalize(out);
        return out;
    }

    // Set one or more parameters living in a single Object (the destination).
    static QByteArray buildMultiParamSet(const Address& self, const Address& target,
                                         const QVector<ParamValue>& params,
                                         quint16 sequence) {
        Header h;
        h.source = self;
        h.destination = target;
        h.messageId = quint16(MessageId::MultiParamSet);
        h.flags = Flags::Guaranteed;
        h.sequence = sequence;
        QByteArray out;
        writeHeader(out, h);
        putU16(out, quint16(params.size()));   // NUMBER OF PARAMETERS
        for (const auto& p : params) {
            putU16(out, p.paramId);
            putU8 (out, quint8(p.type));
            writeValue(out, p);
        }
        finalize(out);
        return out;
    }

    // Ask the device to send back the current values of given parameter IDs.
    static QByteArray buildMultiParamGet(const Address& self, const Address& target,
                                         const QVector<quint16>& ids, quint16 sequence) {
        Header h;
        h.source = self; h.destination = target;
        h.messageId = quint16(MessageId::MultiParamGet);
        h.flags = Flags::Guaranteed; h.sequence = sequence;
        QByteArray out;
        writeHeader(out, h);
        putU16(out, quint16(ids.size()));
        for (auto id : ids) putU16(out, id);
        finalize(out);
        return out;
    }

    // Subscribe to every SV on an Object; device then pushes MultiParamSets on
    // change. changeType=5 for Soundcraft (non-sensor SVs + attributes).
    static QByteArray buildSubscribeAll(const Address& self, const Address& target,
                                        quint16 sequence, bool initialUpdate = true) {
        Header h;
        h.source = self; h.destination = target;
        h.messageId = quint16(MessageId::ParamSubscribeAll);
        h.flags = Flags::Guaranteed; h.sequence = sequence;
        QByteArray out;
        writeHeader(out, h);
        putU16(out, self.node);            // subscriber NODE ID
        putU32(out, target.vdObject());    // VD-OBJECT being subscribed
        putU8 (out, 5);                    // Change Type (Soundcraft = 5)
        putU16(out, 50);                   // Sensor Rate (ms)
        putU16(out, initialUpdate ? 1 : 0);// Initial Update
        finalize(out);
        return out;
    }

    // ---- payload value helpers --------------------------------------------
    static void writeValue(QByteArray& out, const ParamValue& p) {
        switch (p.type) {
        case DataType::BYTE:    case DataType::UBYTE:  putU8 (out, quint8(p.raw));  break;
        case DataType::WORD:    case DataType::UWORD:  putU16(out, quint16(p.raw)); break;
        case DataType::LONG:    case DataType::ULONG:  putU32(out, quint32(p.raw)); break;
        case DataType::FLOAT32: {
            float f = float(p.real); quint32 bits; std::memcpy(&bits, &f, 4); putU32(out, bits);
        } break;
        case DataType::FLOAT64: {
            double d = p.real; quint64 bits; std::memcpy(&bits, &d, 8);
            putU32(out, quint32(bits >> 32)); putU32(out, quint32(bits)); 
        } break;
        }
    }

    static int valueSize(DataType t) {
        switch (t) {
        case DataType::BYTE: case DataType::UBYTE: return 1;
        case DataType::WORD: case DataType::UWORD: return 2;
        case DataType::LONG: case DataType::ULONG:
        case DataType::FLOAT32: return 4;
        case DataType::FLOAT64: return 8;
        }
        return 4;
    }

    // Parse a MultiParamSet payload arriving from the device.
    static QVector<ParamValue> parseMultiParamSet(const QByteArray& msg) {
        QVector<ParamValue> out;
        int o = quint8(msg[1]);                 // skip variable-length header
        if (o + 2 > msg.size()) return out;
        quint16 count = getU16(msg, o); o += 2;
        for (int i = 0; i < count && o + 3 <= msg.size(); ++i) {
            ParamValue p;
            p.paramId = getU16(msg, o); o += 2;
            p.type = DataType(quint8(msg[o])); o += 1;
            int sz = valueSize(p.type);
            if (o + sz > msg.size()) break;
            switch (p.type) {
            case DataType::BYTE:  p.raw = qint8(msg[o]); break;
            case DataType::UBYTE: p.raw = quint8(msg[o]); break;
            case DataType::WORD:  p.raw = qint16(getU16(msg, o)); break;
            case DataType::UWORD: p.raw = getU16(msg, o); break;
            case DataType::LONG:  p.raw = qint32(getU32(msg, o)); break;
            case DataType::ULONG: p.raw = getU32(msg, o); break;
            case DataType::FLOAT32: { quint32 b = getU32(msg, o); float f; std::memcpy(&f,&b,4); p.real=f; } break;
            case DataType::FLOAT64: { quint64 b = (quint64(getU32(msg,o))<<32)|getU32(msg,o+4); double d; std::memcpy(&d,&b,8); p.real=d; } break;
            }
            o += sz;
            out.push_back(p);
        }
        return out;
    }
};

} // namespace hiqnet
