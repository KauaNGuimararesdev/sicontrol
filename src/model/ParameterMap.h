#pragma once
#include "../hiqnet/HiQnetTypes.h"
#include "../hiqnet/StateVarCatalog.h"
#include <QHash>

// ---------------------------------------------------------------------------
//  ParameterMap
//
//  The single most console-specific piece. HiQnet gives us the *transport*;
//  this map gives meaning: it resolves a logical request like
//      "channel 5, EQ band 2 gain, on Mix bus 3"
//  into a concrete HiQnet target = { Virtual Device, Object address, Param ID }.
//
//  The original ViSi_Listen has this table baked into C++ for each Si model.
//  It is NOT in the public HiQnet guide. You complete it per console using:
//    * the console's product description / SubscribeAll discovery (see README),
//    * Wireshark capture of the official editor talking to the desk, or
//    * Soundcraft documentation.
//
//  Until filled with verified values the app connects and speaks HiQnet
//  correctly but cannot move real parameters. The structure below makes that
//  completion mechanical rather than architectural.
// ---------------------------------------------------------------------------

namespace model {

// A fully-resolved address of one controllable value on the console.
struct ParamRef {
    hiqnet::Address  object;            // VD + object the SV lives in
    quint16          paramId = 0;       // SV ID within the object
    hiqnet::DataType type = hiqnet::DataType::LONG;
    sv::Kind         kind = sv::Kind::Unknown;
    // Raw<->engineering scaling (linear). value_eng = raw*scale + offset.
    double           scale = 1.0;
    double           offset = 0.0;
    double           minEng = 0.0, maxEng = 1.0;
    bool valid() const { return object.node != 0 || object.vdObject() != 0 || paramId != 0; }
};

// Identifies a logical control instance.
struct ParamKey {
    sv::Kind kind;
    int      channel = -1;   // input channel (0-based), -1 = n/a
    int      bus     = -1;   // mix bus / master index, -1 = n/a
    int      band    = -1;   // EQ band / matrix node, -1 = n/a
    bool operator==(const ParamKey& o) const {
        return kind == o.kind && channel == o.channel && bus == o.bus && band == o.band;
    }
};
inline uint qHash(const ParamKey& k, uint seed = 0) {
    return ::qHash(int(k.kind), seed) ^ ::qHash(k.channel) ^ (::qHash(k.bus) << 1) ^ (::qHash(k.band) << 2);
}

// Describes a console model: counts + the resolver that turns a ParamKey into
// a concrete ParamRef. Subclass / configure per Si model.
class ConsoleProfile {
public:
    QString modelName = "Soundcraft Si (generic)";
    quint16 classId   = 0;        // HiQnet product ClassID (e.g. Si Expression 2)
    int     inputChannels = 64;
    int     mixBuses      = 24;    // aux/group/matrix outputs
    int     matrixOuts    = 8;
    int     eqBands       = 4;     // parametric bands per channel
    bool    hasGeq        = true;  // graphic EQ on outputs

    // Register an explicit, verified mapping.
    void define(const ParamKey& key, const ParamRef& ref) {
        m_table.insert(key, ref);
        m_reverse.insert(code(ref.object, ref.paramId), key);  // keep reverse index in sync
    }

    // Reverse-resolve a wire address back to the logical control it drives.
    // Returns true and fills `out` if this (object,paramId) was mapped/learned.
    bool reverse(const hiqnet::Address& obj, quint16 paramId, ParamKey& out) const {
        auto it = m_reverse.find(code(obj, paramId));
        if (it == m_reverse.end()) return false;
        out = it.value();
        return true;
    }

    // Stable 64-bit key for (object address + param id).
    static quint64 code(const hiqnet::Address& a, quint16 paramId) {
        return (quint64(a.node) << 48) ^ (quint64(a.vdObject()) << 16) ^ quint64(paramId);
    }

    // Resolve. Returns an invalid ParamRef if unknown (UI shows it as unavailable).
    ParamRef resolve(const ParamKey& key) const {
        auto it = m_table.find(key);
        if (it != m_table.end()) return it.value();
        return computeFromPattern(key);   // optional algorithmic fallback
    }

protected:
    // Many consoles lay objects out on a regular stride. Once you know the
    // base object + stride for, say, "input channel processing", you can derive
    // most channels without enumerating each. Override per model.
    virtual ParamRef computeFromPattern(const ParamKey&) const { return ParamRef{}; }

    QHash<ParamKey, ParamRef> m_table;
    QHash<quint64, ParamKey>  m_reverse;   // (object,paramId) -> logical control
};

} // namespace model
