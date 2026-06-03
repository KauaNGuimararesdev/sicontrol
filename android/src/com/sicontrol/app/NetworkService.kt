package com.sicontrol.app

import android.content.Context
import android.net.ConnectivityManager
import android.net.LinkProperties
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.wifi.WifiManager
import java.net.InetAddress
import java.net.InetSocketAddress
import java.net.Socket
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.concurrent.thread

/**
 * Android-side wireless/network layer for the HiQnet client.
 *
 * Responsibilities the JVM side is better at than C++ on Android:
 *   - holding a Wi-Fi MulticastLock so UDP/broadcast discovery isn't filtered,
 *   - reading the active Wi-Fi link's IPv4 + prefix length (needed to build the
 *     HiQnet Discovery payload's IP ADDRESS / SUBNET MASK fields),
 *   - keeping the radio awake during a session,
 *   - running a raw TCP socket to the console on port 3804 and shuttling bytes
 *     to/from the native engine.
 *
 * Bytes are handed to C++ through the JNI callbacks at the bottom. The C++
 * HiQnetClient builds/parses frames; this class is just the transport on
 * Android. (On desktop, Qt's QTcpSocket is used directly and this file is
 * unused.)
 */
class NetworkService(private val ctx: Context) {

    companion object {
        const val HIQNET_TCP_PORT = 3804
        // implemented in libSiControl.so
        @JvmStatic external fun nativeOnConnected()
        @JvmStatic external fun nativeOnData(data: ByteArray)
        @JvmStatic external fun nativeOnDisconnected(reason: String)
        @JvmStatic external fun nativeOnLocalAddress(ipv4: Int, prefixLen: Int, mac: ByteArray)
    }

    private val wifi = ctx.applicationContext
        .getSystemService(Context.WIFI_SERVICE) as WifiManager
    private val cm = ctx.applicationContext
        .getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager

    private var multicastLock: WifiManager.MulticastLock? = null
    private var wifiLock: WifiManager.WifiLock? = null
    private var socket: Socket? = null
    private val running = AtomicBoolean(false)
    private var writer: Thread? = null
    private val outbox = ArrayDeque<ByteArray>()

    /** Acquire Wi-Fi locks and report the local IPv4/prefix/MAC to native. */
    fun acquireWifi() {
        multicastLock = wifi.createMulticastLock("sicontrol-hiqnet").apply {
            setReferenceCounted(true); acquire()
        }
        wifiLock = wifi.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, "sicontrol").apply {
            setReferenceCounted(true); acquire()
        }
        publishLocalAddress()
    }

    fun releaseWifi() {
        multicastLock?.let { if (it.isHeld) it.release() }
        wifiLock?.let { if (it.isHeld) it.release() }
        multicastLock = null; wifiLock = null
    }

    /** Find the active Wi-Fi link's IPv4 address + prefix and report to native. */
    private fun publishLocalAddress() {
        val request = NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build()
        cm.requestNetwork(request, object : ConnectivityManager.NetworkCallback() {
            override fun onLinkPropertiesChanged(network: Network, lp: LinkProperties) {
                for (la in lp.linkAddresses) {
                    val addr = la.address
                    if (addr is java.net.Inet4Address) {
                        val bytes = addr.address
                        val ipv4 = ((bytes[0].toInt() and 0xFF) shl 24) or
                                   ((bytes[1].toInt() and 0xFF) shl 16) or
                                   ((bytes[2].toInt() and 0xFF) shl 8)  or
                                   (bytes[3].toInt() and 0xFF)
                        // best-effort MAC (modern Android randomises / hides this;
                        // a locally-administered MAC derived from the node is fine)
                        val mac = byteArrayOf(0x02, 0, 0, 0, 0,
                            (ipv4 and 0xFF).toByte())
                        nativeOnLocalAddress(ipv4, la.prefixLength, mac)
                        return
                    }
                }
            }
        })
    }

    /** Open the TCP session to the console. */
    fun connect(ip: String) {
        disconnect("reconnect")
        running.set(true)
        thread(name = "hiqnet-rx") {
            try {
                val s = Socket()
                s.tcpNoDelay = true
                s.connect(InetSocketAddress(InetAddress.getByName(ip), HIQNET_TCP_PORT), 5000)
                socket = s
                startWriter(s)
                nativeOnConnected()
                val input = s.getInputStream()
                val buf = ByteArray(8192)
                while (running.get()) {
                    val n = input.read(buf)
                    if (n <= 0) break
                    nativeOnData(buf.copyOf(n))
                }
            } catch (e: Exception) {
                nativeOnDisconnected(e.message ?: "io error")
            } finally {
                disconnect("closed")
            }
        }
    }

    private fun startWriter(s: Socket) {
        writer = thread(name = "hiqnet-tx") {
            val out = s.getOutputStream()
            try {
                while (running.get()) {
                    val frame: ByteArray? = synchronized(outbox) {
                        if (outbox.isEmpty()) { (outbox as Object).wait(1000); null }
                        else outbox.removeFirst()
                    }
                    if (frame != null) { out.write(frame); out.flush() }
                }
            } catch (_: Exception) { /* writer ends on disconnect */ }
        }
    }

    /** Called from native (C++) to enqueue an outgoing HiQnet frame. */
    fun send(frame: ByteArray) {
        synchronized(outbox) { outbox.addLast(frame); (outbox as Object).notifyAll() }
    }

    fun disconnect(reason: String) {
        if (!running.getAndSet(false)) return
        try { socket?.close() } catch (_: Exception) {}
        socket = null
        synchronized(outbox) { (outbox as Object).notifyAll() }
        nativeOnDisconnected(reason)
    }
}
