#pragma once
#include <QObject>
#include <QString>

// One input channel's controllable state, exposed to QML. The model writes
// incoming HiQnet values here; QML reads them and calls back into MixerModel
// to push changes out.
namespace model {

class Channel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int     index   READ index   CONSTANT)
    Q_PROPERTY(QString name    READ name    WRITE setName    NOTIFY changed)
    Q_PROPERTY(double  fader   READ fader   WRITE setFader   NOTIFY changed)   // dB
    Q_PROPERTY(double  pan     READ pan     WRITE setPan     NOTIFY changed)   // -1..1
    Q_PROPERTY(bool    mute    READ mute    WRITE setMute    NOTIFY changed)
    Q_PROPERTY(bool    on      READ on      WRITE setOn      NOTIFY changed)
    Q_PROPERTY(double  gain    READ gain    WRITE setGain    NOTIFY changed)   // dB head-amp
    Q_PROPERTY(bool    phantom READ phantom WRITE setPhantom NOTIFY changed)
    Q_PROPERTY(bool    phase   READ phase   WRITE setPhase   NOTIFY changed)
    Q_PROPERTY(double  meter   READ meter   NOTIFY meterChanged)               // 0..1
    Q_PROPERTY(double  gainReduction READ gainReduction NOTIFY meterChanged)   // dB
    Q_PROPERTY(bool    inViewGroup READ inViewGroup WRITE setInViewGroup NOTIFY changed)
public:
    explicit Channel(int idx, QObject* parent = nullptr) : QObject(parent), m_index(idx) {
        m_name = QStringLiteral("CH %1").arg(idx + 1);
    }
    int index() const { return m_index; }
    QString name() const { return m_name; }
    double fader() const { return m_fader; }
    double pan() const { return m_pan; }
    bool mute() const { return m_mute; }
    bool on() const { return m_on; }
    double gain() const { return m_gain; }
    bool phantom() const { return m_phantom; }
    bool phase() const { return m_phase; }
    double meter() const { return m_meter; }
    double gainReduction() const { return m_gr; }
    bool inViewGroup() const { return m_inViewGroup; }

    void setName(const QString& v){ if(v!=m_name){m_name=v; emit changed();} }
    void setFader(double v){ if(v!=m_fader){m_fader=v; emit changed(); emit pushFader(m_index,v);} }
    void setPan(double v){ if(v!=m_pan){m_pan=v; emit changed(); emit pushPan(m_index,v);} }
    void setMute(bool v){ if(v!=m_mute){m_mute=v; emit changed(); emit pushMute(m_index,v);} }
    void setOn(bool v){ if(v!=m_on){m_on=v; emit changed(); emit pushOn(m_index,v);} }
    void setGain(double v){ if(v!=m_gain){m_gain=v; emit changed(); emit pushGain(m_index,v);} }
    void setPhantom(bool v){ if(v!=m_phantom){m_phantom=v; emit changed(); emit pushPhantom(m_index,v);} }
    void setPhase(bool v){ if(v!=m_phase){m_phase=v; emit changed(); emit pushPhase(m_index,v);} }
    void setInViewGroup(bool v){ if(v!=m_inViewGroup){m_inViewGroup=v; emit changed();} }

    // Silent setters used when the *console* reports a value (no echo back out).
    void applyFader(double v){ m_fader=v; emit changed(); }
    void applyPan(double v){ m_pan=v; emit changed(); }
    void applyMute(bool v){ m_mute=v; emit changed(); }
    void applyMeter(double v, double gr){ m_meter=v; m_gr=gr; emit meterChanged(); }

signals:
    void changed();
    void meterChanged();
    void pushFader(int ch, double db);
    void pushPan(int ch, double pan);
    void pushMute(int ch, bool m);
    void pushOn(int ch, bool on);
    void pushGain(int ch, double db);
    void pushPhantom(int ch, bool on);
    void pushPhase(int ch, bool on);

private:
    int m_index;
    QString m_name;
    double m_fader = -90.0, m_pan = 0.0, m_gain = 0.0, m_meter = 0.0, m_gr = 0.0;
    bool m_mute = false, m_on = true, m_phantom = false, m_phase = false, m_inViewGroup = true;
};

} // namespace model
