package anon.fidoac

//import logoverwrite.Log

import android.content.Context
import android.nfc.Tag
import android.nfc.tech.IsoDep
import android.preference.PreferenceManager
import android.util.Log
import net.sf.scuba.smartcards.*
import org.jmrtd.BACKey
import org.jmrtd.PACEKeySpec
import org.jmrtd.PassportService
import org.jmrtd.Util
import org.jmrtd.lds.*
import org.jmrtd.lds.icao.DG14File
import org.jmrtd.lds.icao.DG1File
import org.jmrtd.protocol.EACCAAPDUSender
import org.jmrtd.protocol.EACCAProtocol
import org.jmrtd.protocol.SecureMessagingAPDUSender
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.IOException
import java.math.BigInteger
import java.security.KeyPairGenerator
import java.security.MessageDigest
import java.security.PublicKey
import java.security.interfaces.ECPublicKey
import java.security.spec.AlgorithmParameterSpec
import java.util.*
import javax.crypto.interfaces.DHPublicKey


/**
 * Abstract class for electronic IDs like ePassport or German eID national ID card
 */
abstract class EIDInterface(tag: android.nfc.Tag, val stateBasket: StateBasket) {
    private val TAG = this.javaClass.simpleName
    private val KEYSTORE_ALIAS = "anon.fidoac"

    var passportService: PassportService

    //Workflow from https://github.com/tananaev/passport-reader/blob/master/app/src/main/java/com/tananaev/passportreader/MainActivity.java
    fun runToReadDG1(bacKey:BACKey): Unit? {
        var paceSucceeded = false
        try {
            if (true){
                try {
                    val cardAccessFile:CardAccessFile = CardAccessFile(passportService.getInputStream(PassportService.EF_CARD_ACCESS))
                    for (securityInfo: SecurityInfo in cardAccessFile.securityInfos) {
                        if (securityInfo is PACEInfo) {
                            val paceInfo:PACEInfo = securityInfo
                            passportService.doPACE(this.stateBasket.paceKey, paceInfo.getObjectIdentifier(), PACEInfo.toParameterSpec(paceInfo.getParameterId()), null);
                            paceSucceeded = true;
                            Log.d(TAG, "PACE Done");
                        }
                    }
                } catch (e: Exception) {
                    Log.w(TAG, e);
                }

                try{
                    passportService.sendSelectApplet(paceSucceeded);
                }
                catch(e:Exception){
                    Log.e(TAG,"Cannot Select Applet. Returning\n\n")
                    Log.w(TAG,e)
                    return null
                }

                if (!paceSucceeded) {
                    try {
                        passportService.getInputStream(PassportService.EF_COM).read();
                        Log.d(TAG,"Done Commmon Read")
                    } catch (e: Exception) {
                        try{
                            passportService.doBAC(bacKey);
                            Log.d(TAG,"Done BAC")
                        }
                        catch (e: Exception){
                            Log.e(TAG,"BAC & PACE Failed. Returning\n\n")
                            Log.w(TAG,e)
                            return null
                        }
                    }
                }
            }
            else{
                Log.d(TAG,"Doing BAC")
                passportService.doBAC(bacKey);
                Log.d(TAG,"Done BAC")
            }

            Log.d(TAG,"Reading DG1")
            val dg1File = readDG1File(stateBasket.context)
            Log.d(TAG,dg1File.toString())
            Log.d(TAG,"Reading DG14")
            val dg14File = readDG14File(stateBasket.context)
            Log.d(TAG,dg14File.toString())
            Log.d(TAG,"Reading SOD")
            val sodFile = readSODFile(stateBasket.context)
            Log.d(TAG,sodFile.toString())

            // We perform Chip Authentication using Data Group 14
            if (paceSucceeded){
                dg14File?.let { doChipAuth(passportService, it) };
                //return final result here
            }
            else{
                Log.d(TAG,"BAC Chip Auth")
                dg14File?.let { doChipAuth(passportService, it) };
            }
//            doPassiveAuth();
        } catch (e: Exception) {
            Log.w(TAG, e);
        }
        return null;
    }
    private fun doChipAuth(service: PassportService, dg14File :DG14File) {
        try {
            //Assume only one algorithm is supported.
            val dg14FileSecurityInfos: Collection<SecurityInfo> = dg14File.getSecurityInfos()
            Log.d(TAG, dg14FileSecurityInfos.toString())
            var publicKeyInfo:SecurityInfo? = null
            var keyId: BigInteger? = null
            var publicKey: PublicKey? = null
            var oid:String = ""
            for (securityInfo in dg14FileSecurityInfos) {
                Log.d(TAG, "Checking security info")
                Log.d(TAG, securityInfo.toString())
                if (securityInfo is ChipAuthenticationPublicKeyInfo) {
                    publicKeyInfo = securityInfo
                    keyId = publicKeyInfo.keyId
                    publicKey = publicKeyInfo.subjectPublicKey
                    //oid = publicKeyInfo.objectIdentifier
                }
                if (securityInfo is ChipAuthenticationInfo){
                    publicKeyInfo = securityInfo
                    keyId = publicKeyInfo.keyId
                    oid = securityInfo.objectIdentifier
                }
            }

            try {
                var agreementAlg:String? = null
                agreementAlg = ChipAuthenticationInfo.toKeyAgreementAlgorithm(oid)

                var params: AlgorithmParameterSpec? = null
                if ("DH" == agreementAlg) {
                    val piccDHPublicKey = publicKey as DHPublicKey
                    params = piccDHPublicKey.params
                } else if ("ECDH" == agreementAlg) {
                    val piccECPublicKey = publicKey as ECPublicKey
                    params = piccECPublicKey.params
                }

                //passportService.doEACCA()
                /* Generate the ephemeral keypair based on received challenge from relying party with key attestation.*/
                val keyPairGenerator =
                    KeyPairGenerator.getInstance(agreementAlg,
                        Util.getBouncyCastleProvider())
                //Because IOS does not allow attestation on ECDH. But Android may use AndroidKeyStore.

                keyPairGenerator.initialize(params)
                val appKeyPair = keyPairGenerator.generateKeyPair()
                val appPublicKey = appKeyPair.public
                val appPrivateKey = appKeyPair.private

                val eaccaapduSender = EACCAAPDUSender(passportService)
                Log.d(TAG,"Sedning Public Key")
                EACCAProtocol.sendPublicKey(eaccaapduSender, passportService.wrapper, oid, keyId, appPublicKey)
                Log.d(TAG,"Sedning Public Key - done")
                val keyHash = EACCAProtocol.getKeyHash(agreementAlg, appPublicKey)
                val sharedSecret = EACCAProtocol.computeSharedSecret(
                    agreementAlg,
                    publicKey,
                    appPrivateKey
                )
                //Not necessary?
                val new_wrapper = EACCAProtocol.restartSecureMessaging(
                    oid,
                    sharedSecret,
                    255,
                    true
                )

                //Get challenge
                val capdu: CommandAPDU =
                    CommandAPDU(ISO7816.CLA_ISO7816.toUInt().toInt(), ISO7816.INS_GET_CHALLENGE.toUInt().toInt(), 0x00, 0x00,8)
                val secureMessagingSender = SecureMessagingAPDUSender(passportService)
                val rapdu: ResponseAPDU = secureMessagingSender.transmit(new_wrapper, capdu)
                Log.d(TAG,rapdu.toString())

//                val commandAPDU = CommandAPDU(this.stateBasket.relyingparty_challenge)
//                Log.d(TAG,"Sending random challenge")
//                val responseAPDU: ResponseAPDU = passportService.transmit(commandAPDU)
//                Log.d(TAG,"Sending random challenge -done. Reiceved responseAPDU")
//                Log.d(TAG,responseAPDU.toString())

                //TODO we should already have enough info here, transcript=appPublicKey,responseAPDU,sharedSecet,keyHash , passport_pa=dg1,dg14,SOD is enough
                //dg14 in plain. dg1 in hash. then we have dg1 hash as public value to snark proof.
                Log.d(TAG,"Left integration with SNARK and server")
                return
            }
            catch(e:Exception){
                Log.w(TAG, e);
            }

        } catch (e: java.lang.Exception) {
            Log.w(TAG, e)
        }
    }
    private fun passiveAuth(dg1File:DG1File, sodFile:SODFile){
        //Do on server

        //Verify dg1 (for data), dg14 for (publickey),
        val digest: MessageDigest = MessageDigest.getInstance(sodFile.getDigestAlgorithm())
        val dg1Hash: ByteArray = digest.digest(dg1File.getEncoded())
        val dataHashes = sodFile.dataGroupHashes

        //Verify SOD against DSC
        //Verify DSC against CSCA
        //Verify DSC not in CRL
        //Check that DG hash values match the hash values stored in SOD
    }

    /**
     * Runs PACE protocol with the initialized tag
     * @param paceKeySpec the PACE key, commonly made up of the MRZ data
     * @throws CardServiceException
     * @throws IOException
     */
    @Throws(CardServiceException::class, IOException::class)
    fun runPace(paceKeySpec: PACEKeySpec?, context: Context) {
        // Try with all returned PACE parameters
        try {
            // Read Card Access file to get supported protocols
            val cardAccessFile = readCardAccessFile(context)
            if (cardAccessFile ==null){
                throw CardServiceException("PACE failed! PACE is not supported")
            }

            val securityInfoCollection = cardAccessFile!!.securityInfos

            for (securityInfo in securityInfoCollection) {
                if (securityInfo is PACEInfo) {
                    passportService.doPACE(
                        paceKeySpec, securityInfo.getObjectIdentifier(), PACEInfo.toParameterSpec(
                            securityInfo.parameterId
                        ), null
                    )
                }
            }
        } catch (e: CardServiceException) {
            Log.e(TAG, "PACE failed!")
            throw e
        }
        Log.i(TAG, "PACE successful!")
        passportService.sendSelectApplet(true)
    }

    fun runBAC(bacKey: BACKey, context:Context): Boolean{
        try {
            Log.d(TAG, "Reading from EF_COM")
            passportService.getInputStream(PassportService.EF_COM).read()
            Log.d(TAG, "Read from EF_COM")
        } catch (e: java.lang.Exception) {
            Log.d(TAG, "Try BAC")
            passportService.doBAC(bacKey)
            Log.d(TAG, "BAC Done")
        }
        return true
    }

    /**
     * Read Card Access File from an eID
     * @param context App context - used for caching
     */
    fun readCardAccessFile(context: Context): CardAccessFile? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("cardAccessFileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            try {
                val fileInputStream = context.openFileInput("cardAccessFile")
                val bufferedInputStream = BufferedInputStream(fileInputStream)
                return CardAccessFile(bufferedInputStream)
            } catch (e: IOException) {
                Log.e(TAG, "Error reading cached file!")
            }
        }
        var file: CardAccessFile? = null
        try {
            file = CardAccessFile(passportService.getInputStream(PassportService.EF_CARD_ACCESS))
        } catch (e: CardServiceException) {
            Log.e(TAG, "Error reading passport file - CardServiceException\tMost likely PACE unavailable on device")
        } catch (e: IOException) {
            Log.e(TAG, "Error reading passport file - IO Exception")
            e.message?.let { Log.e(TAG, it) }
        }

        if (file !=null){
            try {
                val fileOutputStream = context.openFileOutput("cardAccessFile", Context.MODE_PRIVATE)
                val bufferedOutputStream = BufferedOutputStream(fileOutputStream)
                bufferedOutputStream.write(file!!.encoded)
                bufferedOutputStream.close()
                fileOutputStream.close()
                val editor = sharedPref.edit()
                editor.putBoolean("cardAccessFileCached", true)
                editor.apply()
            } catch (e: IOException) {
                Log.e(TAG, "Error caching file!")
            }
        }
        return file
    }

    abstract fun readDG1File(context: Context): DG1File?
    abstract fun readDG14File(context: Context): DG14File?
    abstract fun readSODFile(context: Context): SODFile?
    fun close() {
        passportService.close()
    }

    /**
     * Initializes connection with an NFC tag
     * @param tag the NFC tag, should support IsoDep
     * @throws IllegalArgumentException
     * @throws CardServiceException
     */
    init {
        require(
            Arrays.asList(*tag.techList).contains("android.nfc.tech.IsoDep")
        ) { "Tag is not of type IsoDep!" }
        val isoDep = IsoDep.get(tag)
        val cardService = CardService.getInstance(isoDep)
        cardService.open()
        passportService = PassportService(
            cardService,
            PassportService.NORMAL_MAX_TRANCEIVE_LENGTH,
            PassportService.DEFAULT_MAX_BLOCKSIZE,
            false,
            false
        )
        try {
            passportService.open()
        } catch (e: CardServiceException) {
            e.message?.let { Log.e(TAG, it) }
            throw e
        }
    }
}

internal class EIDEPassportInterface
/**
 * Initializes connection with an NFC tag
 *
 * @param tag the NFC tag, should support IsoDep
 * @throws IllegalArgumentException
 * @throws CardServiceException
 */
    (tag: Tag, stateBasket: StateBasket) : EIDInterface(tag, stateBasket) {
    /**
     * Read DG1 File from an eID
     * @param context App context - used for caching
     */
    //MODE_PRIVATE = overwrite
    override fun readDG1File(context: Context): DG1File? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("DG1FileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            try {
                val fileInputStream = context.openFileInput("DG1File")
                val bufferedInputStream = BufferedInputStream(fileInputStream)
                return DG1File(bufferedInputStream)
            } catch (e: IOException) {
                Log.e(TAG, "Error reading cached file!")
            }
        }
        var file: DG1File? = null
        try {
            file = DG1File(passportService.getInputStream(PassportService.EF_DG1))
        } catch (e: CardServiceException) {
            Log.e(TAG, "Error reading passport file - CardServiceError")
            e.message?.let { Log.e(TAG, it) }
            Log.e(TAG,e.stackTraceToString())
        } catch (e: IOException) {
            Log.e(TAG, "Error reading passport file")
            e.message?.let { Log.e(TAG, it) }
            Log.e(TAG,e.stackTraceToString())
        }
        try {
            val fileOutputStream = context.openFileOutput("DG1File", Context.MODE_PRIVATE)
            val bufferedOutputStream = BufferedOutputStream(fileOutputStream)
            bufferedOutputStream.write(file!!.encoded)
            bufferedOutputStream.close()
            if (fileOutputStream != null) {
                fileOutputStream.close()
            }
            val editor = sharedPref.edit()
            editor.putBoolean("DG1FileCached", true)
            editor.apply()
        } catch (e: IOException) {
            Log.e(TAG, "Error caching file!")
        }
        return file
    }

    /**
     * Read DG14 File from an eID
     * @param context App context - used for caching
     */
    override fun readDG14File(context: Context): DG14File? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("DG14FileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            try {
                val fileInputStream = context.openFileInput("DG14File")
                val bufferedInputStream = BufferedInputStream(fileInputStream)
                return DG14File(bufferedInputStream)
            } catch (e: IOException) {
                Log.e(TAG, "Error reading cached file!")
            }
        }
        var file: DG14File? = null
        try {
            file = DG14File(passportService.getInputStream(PassportService.EF_DG14))
        } catch (e: CardServiceException) {
            Log.e(TAG, "Error reading passport file")
        } catch (e: IOException) {
            Log.e(TAG, "Error reading passport file")
        }
        try {
            val fileOutputStream = context.openFileOutput("DG14File", Context.MODE_PRIVATE)
            val bufferedOutputStream = BufferedOutputStream(fileOutputStream)
            bufferedOutputStream.write(file!!.encoded)
            bufferedOutputStream.close()
            if (fileOutputStream != null) {
                fileOutputStream.close()
            }
            val editor = sharedPref.edit()
            editor.putBoolean("DG14FileCached", true)
            editor.apply()
        } catch (e: IOException) {
            Log.e(TAG, "Error caching file!")
        }
        return file
    }

    /**
     * Read SOD File from an eID
     * @param context App context - used for caching
     */
    override fun readSODFile(context: Context): SODFile? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("SODFileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            val files = context.fileList()
            if (files != null) {
                for (fileName in files) {
                    Log.i(TAG, "Available file: $fileName")
                }
            }
            try {
                val fileInputStream = context.openFileInput("SODFile")
                val bufferedInputStream = BufferedInputStream(fileInputStream)
                return SODFile(bufferedInputStream)
            } catch (e: IOException) {
                Log.e(TAG, "Error reading cached file!")
            }
        }
        var file: SODFile? = null
        try {
            file = SODFile(passportService.getInputStream(PassportService.EF_SOD))
        } catch (e: CardServiceException) {
            Log.e(TAG, "Error reading passport file")
        } catch (e: IOException) {
            Log.e(TAG, "Error reading passport file")
        }
        try {
            val fileOutputStream = context?.openFileOutput("SODFile", Context.MODE_PRIVATE)
            val bufferedOutputStream = BufferedOutputStream(fileOutputStream)
            bufferedOutputStream.write(file!!.encoded)
            bufferedOutputStream.close()
            if (fileOutputStream != null) {
                fileOutputStream.close()
            }
            val editor = sharedPref.edit()
            editor.putBoolean("SODFileCached", true)
            editor.apply()
        } catch (e: IOException) {
            Log.e(TAG, "Error caching file!")
        }
        return file
    }

    companion object {
        private val TAG = MainActivity::class.java.simpleName
    }
}


