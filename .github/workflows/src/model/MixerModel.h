#pragma once
#include "Channel.h"
#include "ParameterMap.h"
#include "../hiqnet/HiQnetClient.h"
#include <QObject>
#include <QQmlListProperty>
#include <QVector>
#include <QVariant>
#include <cmath>

// ---------------------------------------------------------------------------
//  MixerModel  (registered as a QML singleton: "Mixer")
//
//  The bridge between QML and the console:
//    * holds the Channel objects, selected bus, console profile
//    * owns the HiQnetClient
//    * translates UI pushes -> MultiParamSet, and incoming MultiParamSet ->
//      Channel.apply* setters
//    * exposes engineer features (EQ/dynamics/matrix/mute groups) via
//      Q_INVOKABLE setters that resolve through ParameterMap
// ---------------------------------------------------------------------------

namespace model {

class MixerModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionChanged)
    Q_PROPERTY(int     selectedBus READ selectedBus WRITE setSelectedBus NOTIFY busChanged)
    Q_PROPERTY(bool    busStereo   READ busStereo   NOTIFY busChanged)
    Q_PROPERTY(bool    learnArmed  READ learnArmed  NOTIFY learnStateChanged)
    Q_PROPERTY(QQmlListProperty<model::Channel> channels READ channels NOTIFY channelsChanged)
public:
    explicit MixerModel(QObject* parent = nullptr) : QObject(parent) {
        for (int i = 0; i < m_profile.inputChannels; ++i) {
            auto* ch = new Channel(i, this);
            wireChannel(ch);
            m_channels.push_back(ch);
        }
        connect(&m_client, &hiqnet::HiQnetClient::stateChanged, this,
                [this](hiqnet::HiQnetClient::State){ emit connectionChanged(); });
        connect(&m_client, &hiqnet::HiQnetClient::paramsReceived,
                this, &MixerModel::onParamsReceived);
        connect(&m_client, &hiqnet::HiQnetClient::errorText, this,
                [this](const QString& e){ emit logMessage(e); });
    }

    // --- connection -------------------------------------------------------
    Q_INVOKABLE void configureSelf(int node, const QString& ip, const QString& mask) {
        quint8 mac[6]{0x02,0x00,0x00,0x00,0x00,quint8(node & 0xFF)};   // locally-administered
        m_client.configureSelf(quint16(node), mac, ipToU32(ip), ipToU32(mask), 0);
    }
    Q_INVOKABLE void connectConsole(const QString& ip, int deviceNode) {
        m_client.connectToConsole(ip, quint16(deviceNode));
    }
    Q_INVOKABLE void disconnectConsole() { m_client.disconnectFromConsole(); }

    QString connectionState() const {
        switch (m_client.state()) {
        case hiqnet::HiQnetClient::State::Disconnected: return "Disconnected";
        case hiqnet::HiQnetClient::State::Connecting:   return "Connecting";
        case hiqnet::HiQnetClient::State::Handshaking:  return "Handshaking";
        case hiqnet::HiQnetClient::State::Online:       return "Online";
        }
        return "Unknown";
    }

    // --- bus selection (ViSi flow) ---------------------------------------
    int selectedBus() const { return m_selectedBus; }
    void setSelectedBus(int b) {
        if (b == m_selectedBus) return;
        m_selectedBus = b; emit busChanged();
        subscribeBus(b);
    }
    bool busStereo() const { return m_busStereo.value(m_selectedBus, false); }

    QQmlListProperty<Channel> channels() {
        return QQmlListProperty<Channel>(this, &m_channels);
    }
    Q_INVOKABLE model::Channel* channel(int i) const {
        return (i >= 0 && i < m_channels.size()) ? m_channels[i] : nullptr;
    }

    // --- engineer features: resolve via ParameterMap then push -----------
    Q_INVOKABLE void setEq(int ch, int band, double freq, double gain, double q) {
        pushEng({sv::Kind::PeqFreq,  ch, m_selectedBus, band}, freq);
        pushEng({sv::Kind::PeqLevel, ch, m_selectedBus, band}, gain);
        pushEng({sv::Kind::PeqQ,     ch, m_selectedBus, band}, q);
    }
    Q_INVOKABLE void setEqOn(int ch, bool on) { pushEngBool({sv::Kind::EqOnOff, ch}, on); }
    Q_INVOKABLE void setHpf(int ch, double freq, bool on) {
        pushEng({sv::Kind::HpFreq, ch}, freq);
        pushEngBool({sv::Kind::FilterTypeHP, ch}, on);
    }
    Q_INVOKABLE void setCompressor(int ch, double thr, double ratio, double att, double rel, double makeup) {
        pushEng({sv::Kind::CompThreshold, ch}, thr);
        pushEng({sv::Kind::CompRatio,     ch}, ratio);
        pushEng({sv::Kind::CompAttack,    ch}, att);
        pushEng({sv::Kind::CompRelease,   ch}, rel);
        pushEng({sv::Kind::CompGain,      ch}, makeup);
    }
    Q_INVOKABLE void setGate(int ch, double thr, double range, double att, double hold, double rel) {
        pushEng({sv::Kind::GateThreshold, ch}, thr);
        pushEng({sv::Kind::GateMaxAtten,  ch}, range);
        pushEng({sv::Kind::GateAttack,    ch}, att);
        pushEng({sv::Kind::GateHold,      ch}, hold);
        pushEng({sv::Kind::GateRelease,   ch}, rel);
    }
    // Matrix node send level (input/source -> matrix output).
    Q_INVOKABLE void setMatrix(int matrixOut, int source, double levelDb) {
        pushEng({sv::Kind::MatrixGain, source, matrixOut, -1}, levelDb);
    }
    Q_INVOKABLE void setMatrixOn(int matrixOut, int source, bool on) {
        pushEngBool({sv::Kind::OnOff, source, matrixOut, -1}, on);
    }
    Q_INVOKABLE void setMuteGroup(int group, bool active) {
        pushEngBool({sv::Kind::Mute, -1, -1, group}, active);
    }
    // Master LR / Mono enable, delay/tap on outputs.
    Q_INVOKABLE void setMasterOn(int busIndex, bool on) { pushEngBool({sv::Kind::OnOff, -1, busIndex}, on); }
    Q_INVOKABLE void setOutputDelay(int busIndex, double ms, bool on) {
        pushEng({sv::Kind::Delay, -1, busIndex}, ms);
        pushEngBool({sv::Kind::DelayOnOff, -1, busIndex}, on);
    }

    ConsoleProfile& profile() { return m_profile; }

    // ---- Runtime discovery / Learn ----------------------------------------
    // The Si console streams the address of every parameter it exposes (this is
    // how the original ViSi Listen builds its map - it does NOT ship a static
    // table). Learn mode binds the next console-originated change to a chosen
    // logical control, so the user maps a control by simply moving it on the
    // desk. `kindToken` is e.g. "Fader", "EqGain", "Mute" (see StateVarCatalog).
    Q_INVOKABLE void armLearn(const QString& kindToken, int channel = -1,
                              int bus = -1, int band = -1) {
        const sv::Kind k = sv::kindFromName(kindToken);
        if (k == sv::Kind::Unknown) { emit logMessage(QStringLiteral("learn: unknown control '%1'").arg(kindToken)); return; }
        if (bus < 0) bus = m_selectedBus;
        m_learnKey = ParamKey{k, channel, bus, band};
        m_learnArmed = true;
        emit learnStateChanged();
        emit logMessage(QStringLiteral("learn armed: %1 (ch %2, bus %3, band %4) - now move it on the console")
                        .arg(sv::className(k)).arg(channel).arg(bus).arg(band));
    }
    Q_INVOKABLE void cancelLearn() { m_learnArmed = false; emit learnStateChanged(); }
    bool learnArmed() const { return m_learnArmed; }

    // Snapshot of everything the console has reported, for the Settings list.
    Q_INVOKABLE QVariantList discoveredParams() const { return m_discovered; }
    Q_INVOKABLE void clearDiscovered() { m_discovered.clear(); emit discoveredChanged(); }

signals:
    void connectionChanged();
    void busChanged();
    void channelsChanged();
    void logMessage(const QString& msg);
    void discoveredChanged();
    void learnStateChanged();

private:
    void wireChannel(Channel* ch) {
        connect(ch, &Channel::pushFader, this, [this](int c, double db){ pushContribution(c, db); });
        connect(ch, &Channel::pushPan,   this, [this](int c, double p){ pushEng({sv::Kind::Pan, c, m_selectedBus}, p); });
        connect(ch, &Channel::pushMute,  this, [this](int c, bool m){ pushEngBool({sv::Kind::Mute, c}, m); });
        connect(ch, &Channel::pushOn,    this, [this](int c, bool o){ pushEngBool({sv::Kind::OnOff, c, m_selectedBus}, o); });
        connect(ch, &Channel::pushGain,  this, [this](int c, double db){ pushEng({sv::Kind::Gain, c}, db); });
        connect(ch, &Channel::pushPhantom,this,[this](int c, bool o){ pushEngBool({sv::Kind::PhantomPower48V, c}, o); });
        connect(ch, &Channel::pushPhase, this, [this](int c, bool o){ pushEngBool({sv::Kind::PhaseInvert, c}, o); });
    }

    // ViSi-Listen core: send a channel's contribution level to the current bus.
    void pushContribution(int ch, double db) {
        pushEng({sv::Kind::Fader, ch, m_selectedBus}, db);
    }

    void subscribeBus(int bus) {
        // Subscribe to the bus's contribution object so the console pushes the
        // current state of every channel send on this bus.
        ParamKey k{sv::Kind::Fader, 0, bus};
        ParamRef ref = m_profile.resolve(k);
        if (ref.valid()) m_client.subscribeAll(ref.object);
    }

    void pushEng(const ParamKey& key, double engValue) {
        ParamRef ref = m_profile.resolve(key);
        if (!ref.valid()) { emit logMessage(QStringLiteral("unmapped: %1").arg(sv::className(key.kind))); return; }
        hiqnet::ParamValue v;
        v.paramId = ref.paramId; v.type = ref.type;
        double raw = (engValue - ref.offset) / (ref.scale == 0 ? 1.0 : ref.scale);
        if (ref.type == hiqnet::DataType::FLOAT32 || ref.type == hiqnet::DataType::FLOAT64) v.real = raw;
        else v.raw = qint64(std::llround(raw));
        m_client.setParam(ref.object, v);
    }
    void pushEngBool(const ParamKey& key, bool on) {
        ParamRef ref = m_profile.resolve(key);
        if (!ref.valid()) { emit logMessage(QStringLiteral("unmapped: %1").arg(sv::className(key.kind))); return; }
        hiqnet::ParamValue v; v.paramId = ref.paramId;
        v.type = hiqnet::DataType::LONG; v.raw = on ? 1 : 0;
        m_client.setParam(ref.object, v);
    }

    void onParamsReceived(const hiqnet::Address& src, const QVector<hiqnet::ParamValue>& vals) {
        for (const auto& v : vals) {
            const double eng = v.isFloat() ? v.real : double(v.raw);

            // 1) Learn mode: bind the chosen control to whatever just moved.
            if (m_learnArmed) {
                ParamRef ref;
                ref.object = src;
                ref.paramId = v.paramId;
                ref.type = v.type;
                ref.kind = m_learnKey.kind;
                m_profile.define(m_learnKey, ref);
                m_learnArmed = false;
                emit learnStateChanged();
                emit logMessage(QStringLiteral("learned %1 -> vd=%2 obj=%3 id=%4")
                    .arg(sv::className(m_learnKey.kind))
                    .arg(src.virtualDevice).arg(src.object).arg(v.paramId));
            }

            // 2) If already mapped, update the matching UI control silently.
            ParamKey key;
            if (m_profile.reverse(src, v.paramId, key))
                applyToModel(key, eng);

            // 3) Always record for the live discovery list (capped).
            QVariantMap row;
            row["vd"]  = src.virtualDevice;
            row["obj"] = src.object;
            row["id"]  = v.paramId;
            row["val"] = eng;
            m_discovered.prepend(row);
            if (m_discovered.size() > 400) m_discovered.removeLast();

            emit logMessage(QStringLiteral("rx vd=%1 obj=%2 id=%3 val=%4")
                .arg(src.virtualDevice).arg(src.object).arg(v.paramId).arg(eng));
        }
        emit discoveredChanged();
    }

    // Push a console-originated value into the right Channel without echoing
    // it back out (uses the silent apply* setters).
    void applyToModel(const ParamKey& key, double eng) {
        if (key.channel < 0 || key.channel >= m_channels.size()) return;
        Channel* ch = m_channels[key.channel];
        switch (key.kind) {
            case sv::Kind::Fader:        ch->applyFader(eng); break;
            case sv::Kind::Pan:          ch->applyPan(eng);   break;
            case sv::Kind::Mute:         ch->applyMute(eng != 0.0); break;
            case sv::Kind::LevelMeter:   ch->applyMeter(eng, 0.0);  break;
            case sv::Kind::GainReduction:ch->applyMeter(ch->meter(), eng); break;
            default: break;
        }
    }

    static quint32 ipToU32(const QString& ip) {
        const auto parts = ip.split('.');
        if (parts.size() != 4) return 0;
        return (parts[0].toUInt() << 24) | (parts[1].toUInt() << 16)
             | (parts[2].toUInt() << 8)  |  parts[3].toUInt();
    }

    hiqnet::HiQnetClient   m_client;
    ConsoleProfile         m_profile;
    QVector<Channel*>      m_channels;
    QHash<int,bool>        m_busStereo;
    int                    m_selectedBus = 0;
    bool                   m_learnArmed = false;
    ParamKey               m_learnKey{};
    QVariantList           m_discovered;
};

} // namespace model
