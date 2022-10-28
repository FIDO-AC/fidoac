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
class StateBasket
{
    var tag: Tag? = null
    var context: Context? = null
    var eidInterface: EIDInterface? = null
    var sodFile // Document Security Object read from ePassport
            : SODFile? = null
    var dg1_raw // DG1 (personal data) read from ePassport (Raw Byte)
            : ByteArray? = null
    var dg1_File: DG1File? = null
    var dg14_raw // DG14 (security infos) read from ePassport (Raw Byte)
            : ByteArray? = null
    var collectedClientData: JSONObject? = null
    var relyingparty_challenge:ByteArray? = null
    var client_challenge:ByteArray? = null
    var proof_data:ByteArray? = null
    var ranomized_hash:ByteArray? = null
    var mediator_sign:ByteArray? = null
    var mediator_cert:ArrayList<ByteArray>? = null
    var bacKey: BACKey? = null
    var paceKey: PACEKeySpec? = null // for reading in the Intel Attestation Service root certificate

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
