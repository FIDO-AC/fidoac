package anon.fidoac

import android.app.Activity
import android.content.Context
import android.nfc.Tag
import android.os.AsyncTask
import android.os.Build
import android.preference.PreferenceManager
import android.util.Log
import android.view.View
import android.widget.CheckBox
import android.widget.EditText
import androidx.annotation.RequiresApi
import androidx.core.util.Pair
import com.google.protobuf.ByteString
import com.google.protobuf.InvalidProtocolBufferException
//import logoverwrite.Log
import net.sf.scuba.smartcards.CardServiceException
import net.sf.scuba.smartcards.CommandAPDU
import net.sf.scuba.smartcards.ResponseAPDU;

import org.bouncycastle.jce.ECPointUtil
import org.jmrtd.BACKey
import org.jmrtd.BACKeySpec
import org.jmrtd.PACEKeySpec
import org.jmrtd.Util
import org.jmrtd.lds.ChipAuthenticationPublicKeyInfo
import org.jmrtd.lds.DataGroup
import org.jmrtd.lds.SODFile
import org.jmrtd.lds.icao.DG14File
import org.jmrtd.lds.icao.DG1File
import org.jmrtd.protocol.EACCAAPDUSender
import org.jmrtd.protocol.EACCAProtocol
import org.json.JSONException
import org.json.JSONObject
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.math.BigInteger
import java.net.InetSocketAddress
import java.nio.ByteBuffer
import java.nio.charset.StandardCharsets
import java.security.*
import java.security.cert.CertificateException
import java.security.interfaces.ECPublicKey
import java.security.spec.ECPublicKeySpec
import java.security.spec.InvalidKeySpecException
import java.time.Duration
import java.util.*
import javax.crypto.interfaces.DHPublicKey
import javax.net.ssl.SSLContext
import javax.net.ssl.SSLSocket
import javax.net.ssl.TrustManager
import kotlin.experimental.and


/**
 * Storage object passed along the call chain of the entire reader process.
 * Functions may add values or access previously set ones.
 */
class StateBasket(
    val bacKey: BACKey,
    val paceKey: PACEKeySpec, // for reading in the Intel Attestation Service root certificate
    context: Context
) {
    var tag: Tag? = null
    val context: Context
    val webSocketServerPort = 11111 // websocket server port to listen to
    var eidInterface: EIDInterface? = null
    var sodFile // Document Security Object read from ePassport
            : SODFile? = null
    var dg1File // DG1 (personal data) read from ePassport
            : DG1File? = null
    var dg14File // DG14 (security infos) read from ePassport
            : DG14File? = null
    var collectedClientData: JSONObject? = null
    var caOID // enclave-selected CA cipher OID
            : String? = null
    var caKeyID // enclave-selected CA key ID
            = 0
    var PKSGX // CA public key of Enclave
            : ByteString? = null
    var passportChallenge: ByteString? = null

    /**
     * Initializes a stateBasket for EFIDO.
     * @param tag An NFC Tag
     * @param paceKey A PACE key for use with an ePassport
     * @param context AppContext used for caching
     */
    init {
        this.context = context
        val TAG = this.javaClass.simpleName
        Log.i(TAG, "Initialized stateBasket with paceKey:$paceKey")
    }
}

/**
 * Simple implementation of Reader.
 */
class PassportReader :
    AsyncTask<StateBasket?, Pair<String?, StateBasket?>?, Void?>() {
    private val TAG = this.javaClass.simpleName
    private val timeMeasurement: MutableList<Pair<String, Long>> = ArrayList()

//    Websocket Server. Later initialize at constructor
//    https://github.com/koush/AndroidAsync#androidasync-also-lets-you-create-simple-http-servers

    /**
     * Start the Passport reading chain - called via .execute(...)
     * @param stateBaskets An initialized stateBasket to start the chain.
     */
    protected override fun doInBackground(vararg stateBaskets: StateBasket?): Void? {
        Log.i(TAG, "Starting FIDO-AC call chain.")
        stateBaskets[0]?.let { establishPACEWithPassport(it) }
        return null
    }

    /**
     * Initialize a connection with the ePassport and run the PACE protocol.
     * @param st The stateBasket passed along the call chain.
     */
    @RequiresApi(api = Build.VERSION_CODES.O)
    fun establishPACEWithPassport(st: StateBasket): Boolean {
        try {
            st.eidInterface = st.tag?.let { EIDEPassportInterface(it, st) }
            Log.i(TAG, "Initialized EIDInterface for communication with ePassport.")
            (st.eidInterface as EIDEPassportInterface?)?.runPace(st.paceKey, st.context)
            Log.i(TAG, "Successfully established PACE with ePassport.")
        } catch (e: CardServiceException) {
            Log.e(TAG, "Initializing EIDInterface failed!")
            e.message?.let { Log.e(TAG, it) }
            shutdownSession(st)
            shutdownFinal(st)
            return false
        } catch (e: IOException) {
            Log.e(TAG, "PACE failed!")
            e.message?.let { Log.e(TAG, it) }
            shutdownSession(st)
            shutdownFinal(st)
            return false
        }
        Log.i(TAG, "Established PACE with passport.")
        readInitialDGsFromPassport(st)
        return true
    }

    fun establishBACWithPassport(st:StateBasket): Boolean{
        try {
            st.eidInterface = st.tag?.let { EIDEPassportInterface(it, st) }
            Log.i(TAG, "Initialized EIDInterface for communication with ePassport.")
            (st.eidInterface as EIDEPassportInterface?)?.runBAC(st.bacKey, st.context)
            Log.i(TAG, "Successfully established BAC with ePassport.")
        } catch (e: CardServiceException) {
            Log.e(TAG, "Initializing EIDInterface failed!")
            e.message?.let { Log.e(TAG, it) }
            return false
        } catch (e: IOException) {
            Log.e(TAG, "BAC failed!")
            e.message?.let { Log.e(TAG, it) }
            return false
        }
        Log.i(TAG, "Established BAC with passport.")
        //Read only DG1
        st.dg1File = st.eidInterface!!.readDG1File(st.context)
        Log.i(TAG, "DG1: " + st.dg1File.toString())
        return true
    }

    /**
     * Read the needed DataGroups from the ePassport (DG1 - MRZ, DG14 and SOD).
     * @param st The stateBasket passed along the call chain.
     */
    @RequiresApi(api = Build.VERSION_CODES.O)
    fun readInitialDGsFromPassport(st: StateBasket): Boolean {
        Log.i(TAG, "Reading DataGroups from ePassport.")
        st.sodFile = st.eidInterface?.readSODFile(st.context);
        Log.i(TAG, "SOD: " + st.sodFile.toString());
        st.dg1File = st.eidInterface!!.readDG1File(st.context)
        Log.i(TAG, "DG1: " + st.dg1File.toString())
        st.dg14File = st.eidInterface!!.readDG14File(st.context)
        Log.i(TAG, "DG14: " + st.dg14File.toString())
        Log.i(TAG, "Successfully read DataGroups from ePassport.")
        return true
    }

    /**
     * Shutdown a LinearEFIDO session (WebSocket, Enclave Connection)
     * Use after finishing/failing an authentication
     * @param st The stateBasket passed along the call chain.
     */
    fun shutdownSession(st: StateBasket) {
        try {
//            Thread.sleep(1000)
            Log.i(TAG, "Shutting down WebSocket Server.")
//            st.clientCommunicationWebsocketServer.stop(1000)
        } catch (e: InterruptedException) {
            Log.e(TAG, "Error shutting down WSS!")
            Log.e(TAG, e.toString())
        }
        timeMeasurement.clear()
        st.eidInterface!!.close()
    }

    /**
     * Shutdown the eidInterface.
     * Use when exiting the app.
     * @param st The stateBasket passed along the call chain.
     */
    fun shutdownFinal(st: StateBasket) {
        st.eidInterface!!.close()
    }

    companion object {
        // src: https://stackoverflow.com/questions/9655181/how-to-convert-a-byte-array-to-a-hex-string-in-java
        private val HEX_ARRAY = "0123456789ABCDEF".toCharArray()
        private fun bytesToHex(bytes: ByteArray): String {
            val hexChars = CharArray(bytes.size * 2)
            for (j in bytes.indices) {
                val v: Int = (bytes[j] and 0xFF.toByte()).toUByte().toInt()
                hexChars[j * 2] = HEX_ARRAY[v ushr 4]
                hexChars[j * 2 + 1] = HEX_ARRAY[v and 0x0F]
            }
            return String(hexChars)
        }
    }
}
