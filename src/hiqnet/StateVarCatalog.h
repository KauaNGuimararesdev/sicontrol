#pragma once
#include <QString>
#include <QMap>

// ---------------------------------------------------------------------------
//  State-Variable class catalogue.
//
//  These names were recovered from the original ViSi_Listen native library and
//  the wider Harman HiQnet SV class library. A StateVar's *ClassID* (a number)
//  tells the app how to interpret/scale a raw parameter value; the *name* below
//  is the human label for that class. Faders, gain, mute, pan, EQ, dynamics,
//  matrix and delay are all represented.
//
//  NOTE: the numeric ClassID <-> name binding and the per-console Object/SV-ID
//  address map are model specific. Fill verified IDs in ParameterMap as you
//  confirm them against your console's product description (see README).
// ---------------------------------------------------------------------------

namespace sv {

// Logical parameter "kinds" the UI cares about, each backed by an SV class.
enum class Kind {
    // levels / routing
    Fader, FaderAttn, Gain, InputGain, AnalogInGain, MatrixGain, MixGain, MixInput,
    Pan, Mute, OnOff, Level, Decibels,
    // input head-amp
    PhantomPower48V, PhaseInvert, Polarity, InputSelect, AnalogSourceSelect,
    // EQ (parametric)
    EqOnOff, EqType, EqFreq, EqGain, EqQ, EqSlope,
    PeqFreq, PeqLevel, PeqQ, PeqFilter, PeqHighShelfFreq, PeqLowShelfFreq, PeqShelfSlope,
    HpFreq, FilterTypeHP, FilterTypeLP, FilterTypeEQ,
    // graphic EQ
    GeqFc, GeqGain, GeqFlatRestore,
    // dynamics: compressor
    CompThreshold, CompRatio, CompAttack, CompRelease, CompHold, CompGain, CompAutoOnOff,
    CompKnee, CompSoftKneeWidth,
    // dynamics: gate
    Gate, GateThreshold, GateRatio, GateAttack, GateHold, GateRelease, GateMaxAtten,
    // delay / taps
    Delay, DelayOnOff, DelayUnits, SignalDelayCourse, SignalDelayFine,
    // bus / master
    ListenBusSelect, StereoMono, MixMode,
    // meters (sensor)
    LevelMeter, GainReduction, OutputLevelMeter,
    Unknown
};

inline const QString& className(Kind k) {
    static const QMap<Kind, QString> names = {
        {Kind::Fader,"SVFader"}, {Kind::FaderAttn,"SVFaderAttn"},
        {Kind::Gain,"SVClassGain"}, {Kind::InputGain,"SVClassInputGain"},
        {Kind::AnalogInGain,"SVClassAnalogInGain"}, {Kind::MatrixGain,"SVClassMatrixGain"},
        {Kind::MixGain,"SVClassMixGain"}, {Kind::MixInput,"SVClassMixInput"},
        {Kind::Pan,"SVClassPan"}, {Kind::Mute,"SVClassMute"}, {Kind::OnOff,"SVClassOnOff"},
        {Kind::Level,"SVClassLevel"}, {Kind::Decibels,"SVClassDecibels"},
        {Kind::PhantomPower48V,"SVClassParamEnable"}, {Kind::PhaseInvert,"SVClassPhaseInv"},
        {Kind::Polarity,"SVClassPolarity"}, {Kind::InputSelect,"SVClassInputSelect"},
        {Kind::AnalogSourceSelect,"SVAnalogSourceSelect"},
        {Kind::EqOnOff,"SVClassEQOnOff"}, {Kind::EqType,"SVClassEqType"},
        {Kind::EqFreq,"SVClassEqFreq"}, {Kind::EqGain,"SVClassEqGain"},
        {Kind::EqQ,"SVClassEqQ"}, {Kind::EqSlope,"SVClassEqSlope"},
        {Kind::PeqFreq,"SVClassPEQFreq"}, {Kind::PeqLevel,"SVClassPEQLevel"},
        {Kind::PeqQ,"SVClassPEQQ"}, {Kind::PeqFilter,"SVClassPEQFilter"},
        {Kind::PeqHighShelfFreq,"SVClassPEQHighShelfFreq"}, {Kind::PeqLowShelfFreq,"SVClassPEQLowShelfFreq"},
        {Kind::PeqShelfSlope,"SVClassPEQShelfSlope"},
        {Kind::HpFreq,"SVClassHPFreq"}, {Kind::FilterTypeHP,"SVFilterTypeHP"},
        {Kind::FilterTypeLP,"SVFilterTypeLP"}, {Kind::FilterTypeEQ,"SVFilterTypeEQ"},
        {Kind::GeqFc,"SVClassGEQFc"}, {Kind::GeqGain,"SVClassGEQGain"},
        {Kind::GeqFlatRestore,"SVClassGEQFlatRestore"},
        {Kind::CompThreshold,"SVCompressorThreshold"}, {Kind::CompRatio,"SVCompressorRatio"},
        {Kind::CompAttack,"SVCompressorAttack"}, {Kind::CompRelease,"SVCompressorRelease"},
        {Kind::CompHold,"SVCompressorHoldTime"}, {Kind::CompGain,"SVClassCompGain"},
        {Kind::CompAutoOnOff,"SVClassCompAutoOnOff"}, {Kind::CompKnee,"SVClassKneeSwitch"},
        {Kind::CompSoftKneeWidth,"SVCompressorSoftKneeWidth"},
        {Kind::Gate,"SVClassGate"}, {Kind::GateThreshold,"SVClassGateThreshold"},
        {Kind::GateRatio,"SVClassGateRatio"}, {Kind::GateAttack,"SVClassGateAttack"},
        {Kind::GateHold,"SVClassGateHold"}, {Kind::GateRelease,"SVClassGateRelease"},
        {Kind::GateMaxAtten,"SVClassGateMaxAtten"},
        {Kind::Delay,"SVClassDelay"}, {Kind::DelayOnOff,"SVClassDelayOnOff"},
        {Kind::DelayUnits,"SVClassDelayUnits"},
        {Kind::SignalDelayCourse,"SVSignalDelayCourse"}, {Kind::SignalDelayFine,"SVSignalDelayFine"},
        {Kind::ListenBusSelect,"SVListenBusSelect8"}, {Kind::StereoMono,"SVStereoMono"},
        {Kind::MixMode,"SVMixMode"},
        {Kind::LevelMeter,"SVLevelMeter"}, {Kind::GainReduction,"SVGainReduction"},
        {Kind::OutputLevelMeter,"SVOutputLevelMeter"},
        {Kind::Unknown,"SVClassUnknown"},
    };
    static const QString unknown = "SVClassUnknown";
    auto it = names.find(k);
    return it != names.end() ? it.value() : unknown;
}

// Reverse: a short UI token (e.g. "Fader", "EqGain") or full SV class name
// (e.g. "SVClassEqGain") -> Kind. Used by the in-app Learn mode.
inline Kind kindFromName(const QString& nameOrToken) {
    const QString n = nameOrToken.trimmed();
    for (int i = 0; i <= int(Kind::Unknown); ++i) {
        const Kind k = Kind(i);
        const QString full = className(k);
        if (full.compare(n, Qt::CaseInsensitive) == 0) return k;
        // also match the bare token after the SV/SVClass prefix
        QString token = full;
        token.remove(QStringLiteral("SVClass")).remove(QStringLiteral("SV"));
        if (token.compare(n, Qt::CaseInsensitive) == 0) return k;
    }
    return Kind::Unknown;
}

} // namespace sv
