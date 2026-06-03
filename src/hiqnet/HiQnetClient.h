#pragma once
#include "HiQnetCodec.h"
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>

// ---------------------------------------------------------------------------
//  HiQnetClient
//
//  Owns the TCP session to one console. Implements the full handshake:
//    1. connect to <ip>:3804
//    2. send Discovery  -> device replies Discovery (Info bit set) = recognised
//    3. device may send Hello (0x08) -> we reply with an Error header to run
//       session-less.
//    4. keepalive Discovery every 5 s.
//    5. SubscribeAll on objects of interest; consume pushed MultiParamSets.
//
//  Frames are length-delimited by the 4-byte MESSAGE LENGTH at offset 2, so we
//  buffer the stream and slice out complete messages.
// ---------------------------------------------------------------------------

namespace hiqnet {

class HiQnetClient : public QObject {
    Q_OBJECT
public:
    enum class State { Disconnected, Connecting, Handshaking, Online };
    Q_ENUM(State)

    explicit HiQnetClient(QObject* parent = nullptr) : QObject(parent) {
        connect(&m_sock, &QTcpSocket::connected,    this, &HiQnetClient::onConnected);
        connect(&m_sock, &QTcpSocket::readyRead,    this, &HiQnetClient::onReadyRead);
        connect(&m_sock, &QTcpSocket::disconnected, this, &HiQnetClient::onDisconnected);
        connect(&m_sock, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError){
            emit errorText(m_sock.errorString());
        });
        m_keepAlive.setInterval(5000);
        connect(&m_keepAlive, &QTimer::timeout, this, &HiQnetClient::sendKeepAlive);
    }

    // Local identity. node must be unique on the HiQnet network (1..65534).
    void configureSelf(quint16 node, const quint8 mac[6], quint32 ipv4,
                       quint32 subnet, quint32 gateway = 0) {
        m_self.node = node; m_self.virtualDevice = 0; m_self.object = 0;
        std::memcpy(m_mac, mac, 6); m_ip = ipv4; m_subnet = subnet; m_gw = gateway;
    }

    void connectToConsole(const QString& ip, quint16 deviceNode) {
        m_deviceIp = ip; m_device = Address{ deviceNode, 0, 0 };
        setState(State::Connecting);
        m_rx.clear();
        m_sock.connectToHost(QHostAddress(ip), kTcpPort);
    }
    void disconnectFromConsole() { m_keepAlive.stop(); m_sock.disconnectFromHost(); }

    State state() const { return m_state; }

    // Public send helpers used by the model -------------------------------
    void setParam(const Address& objAddr, const ParamValue& v) {
        QVector<ParamValue> p{ v };
        send(Codec::buildMultiParamSet(m_self, objAddr, p, nextSeq()));
    }
    void setParams(const Address& objAddr, const QVector<ParamValue>& v) {
        send(Codec::buildMultiParamSet(m_self, objAddr, v, nextSeq()));
    }
    void getParams(const Address& objAddr, const QVector<quint16>& ids) {
        send(Codec::buildMultiParamGet(m_self, objAddr, ids, nextSeq()));
    }
    void subscribeAll(const Address& objAddr) {
        send(Codec::buildSubscribeAll(m_self, objAddr, nextSeq(), true));
    }

signals:
    void stateChanged(State s);
    void deviceRecognised();
    void paramsReceived(const Address& source, const QVector<ParamValue>& values);
    void errorText(const QString& msg);

private slots:
    void onConnected() {
        setState(State::Handshaking);
        Header tmpl;                                   // discovery template
        send(Codec::buildDiscovery(m_self, tmpl, m_mac, m_ip, m_subnet, m_gw, false));
    }
    void onDisconnected() { m_keepAlive.stop(); setState(State::Disconnected); }

    void onReadyRead() {
        m_rx.append(m_sock.readAll());
        // Slice complete frames using the message-length field.
        while (m_rx.size() >= kBaseHeaderLen) {
            quint32 len = Codec::getU32(m_rx, 2);
            if (len < kBaseHeaderLen || len > 4u*1024u*1024u) { m_rx.clear(); break; }
            if (quint32(m_rx.size()) < len) break;     // wait for the rest
            QByteArray frame = m_rx.left(int(len));
            m_rx.remove(0, int(len));
            dispatch(frame);
        }
    }
    void sendKeepAlive() {
        Header tmpl;
        send(Codec::buildDiscovery(m_self, tmpl, m_mac, m_ip, m_subnet, m_gw, true));
    }

private:
    void dispatch(const QByteArray& frame) {
        auto hOpt = Codec::readHeader(frame);
        if (!hOpt) return;
        const Header& h = *hOpt;
        const auto id = MessageId(h.messageId);

        if (id == MessageId::Discovery) {
            if (h.flags & Flags::Information) {        // device acknowledged us
                emit deviceRecognised();
                if (m_state != State::Online) { setState(State::Online); m_keepAlive.start(); }
            }
        } else if (id == MessageId::Hello) {
            if (!(h.flags & Flags::Information))        // it's a request -> refuse
                send(Codec::buildHelloRefusal(m_self, h.source, h.sequence));
        } else if (id == MessageId::MultiParamSet) {
            auto vals = Codec::parseMultiParamSet(frame);
            if (!vals.isEmpty()) emit paramsReceived(h.source, vals);
        }
    }

    void send(const QByteArray& bytes) {
        if (m_sock.state() == QAbstractSocket::ConnectedState) m_sock.write(bytes);
    }
    quint16 nextSeq() { return ++m_seq ? m_seq : ++m_seq; }
    void setState(State s) { if (s != m_state) { m_state = s; emit stateChanged(s); } }

    QTcpSocket m_sock;
    QTimer     m_keepAlive;
    QByteArray m_rx;
    Address    m_self, m_device;
    QString    m_deviceIp;
    quint8     m_mac[6]{};
    quint32    m_ip = 0, m_subnet = 0, m_gw = 0;
    quint16    m_seq = 0;
    State      m_state = State::Disconnected;
};

} // namespace hiqnet
