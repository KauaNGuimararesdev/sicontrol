// AndroidBridge.cpp
// -----------------------------------------------------------------------------
// JNI seam between the Kotlin transport (NetworkService.kt) and the C++ engine.
//
// Two directions:
//   Kotlin -> C++ : the four native callbacks declared in NetworkService's
//                   companion object (nativeOnConnected / nativeOnData /
//                   nativeOnDisconnected / nativeOnLocalAddress).
//   C++ -> Kotlin : AndroidTransport wraps a NetworkService instance and calls
//                   acquireWifi / connect / send / disconnect on it.
//
// This file is compiled only for Android. On desktop the app uses Qt's
// QTcpSocket directly (see HiQnetClient), so none of this is needed there.
//
// The callbacks forward raw bytes to a std::function sink. Wire that sink to
// HiQnetClient (feed incoming bytes to its frame parser, and route its outgoing
// frames to AndroidTransport::send) to switch the app from the Qt socket to the
// Kotlin socket. That wiring is intentionally left as a one-line hook in
// MixerModel so both transports can coexist during bring-up.
// -----------------------------------------------------------------------------
#ifdef Q_OS_ANDROID

#include <jni.h>
#include <QtCore/QCoreApplication>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <functional>
#include <mutex>

namespace platform {

// Sinks the engine can install to observe transport events. Default no-ops.
struct Bridge {
    std::function<void()>                       onConnected;
    std::function<void(const QByteArray&)>      onData;
    std::function<void(const QString&)>         onDisconnected;
    std::function<void(quint32, int, QByteArray)> onLocalAddress;
    std::mutex mtx;
};

Bridge& bridge() { static Bridge b; return b; }

} // namespace platform

// ---- Kotlin -> C++ -----------------------------------------------------------
extern "C" {

JNIEXPORT void JNICALL
Java_com_sicontrol_app_NetworkService_00024Companion_nativeOnConnected(JNIEnv*, jobject)
{
    auto& b = platform::bridge();
    std::lock_guard<std::mutex> lk(b.mtx);
    if (b.onConnected) b.onConnected();
}

JNIEXPORT void JNICALL
Java_com_sicontrol_app_NetworkService_00024Companion_nativeOnData(JNIEnv* env, jobject, jbyteArray data)
{
    const jsize n = env->GetArrayLength(data);
    QByteArray bytes(n, Qt::Uninitialized);
    env->GetByteArrayRegion(data, 0, n, reinterpret_cast<jbyte*>(bytes.data()));
    auto& b = platform::bridge();
    std::lock_guard<std::mutex> lk(b.mtx);
    if (b.onData) b.onData(bytes);
}

JNIEXPORT void JNICALL
Java_com_sicontrol_app_NetworkService_00024Companion_nativeOnDisconnected(JNIEnv* env, jobject, jstring reason)
{
    const char* c = env->GetStringUTFChars(reason, nullptr);
    QString r = QString::fromUtf8(c);
    env->ReleaseStringUTFChars(reason, c);
    auto& b = platform::bridge();
    std::lock_guard<std::mutex> lk(b.mtx);
    if (b.onDisconnected) b.onDisconnected(r);
}

JNIEXPORT void JNICALL
Java_com_sicontrol_app_NetworkService_00024Companion_nativeOnLocalAddress(
    JNIEnv* env, jobject, jint ipv4, jint prefixLen, jbyteArray mac)
{
    const jsize n = env->GetArrayLength(mac);
    QByteArray macBytes(n, Qt::Uninitialized);
    env->GetByteArrayRegion(mac, 0, n, reinterpret_cast<jbyte*>(macBytes.data()));
    auto& b = platform::bridge();
    std::lock_guard<std::mutex> lk(b.mtx);
    if (b.onLocalAddress)
        b.onLocalAddress(static_cast<quint32>(ipv4), static_cast<int>(prefixLen), macBytes);
}

} // extern "C"

#endif // Q_OS_ANDROID
