package anon.fidoac

//import logoverwrite.Log

import android.content.Context
import android.nfc.Tag
import android.nfc.tech.IsoDep
import android.preference.PreferenceManager
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Log
import anon.fidoac.certverifier.AndroidKeyAttestationVerifier
import com.google.android.gms.common.util.IOUtils
import net.sf.scuba.smartcards.*
import org.bouncycastle.asn1.ASN1InputStream
import org.bouncycastle.asn1.ASN1Primitive
import org.bouncycastle.asn1.ASN1Sequence
import org.bouncycastle.asn1.ASN1Set
import org.bouncycastle.asn1.x509.Certificate
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
import java.io.ByteArrayInputStream
import java.io.IOException
import java.math.BigInteger
import java.security.*
import java.security.cert.*
import java.security.interfaces.ECPublicKey
import java.security.spec.AlgorithmParameterSpec
import java.security.spec.ECGenParameterSpec
import java.security.spec.MGF1ParameterSpec
import java.security.spec.PSSParameterSpec
import java.util.*
import javax.crypto.interfaces.DHPublicKey
import kotlin.collections.ArrayList
import kotlin.system.measureTimeMillis


/**
 * Abstract class for electronic IDs like ePassport or German eID national ID card
 */
fun ByteArray.toHex(): String = joinToString(separator = "") { eachByte -> "%02x".format(eachByte) }

fun calculateSD(numArray: DoubleArray): Double {
    var sum = 0.0
    var standardDeviation = 0.0

    for (num in numArray) {
        sum += num
    }

    val mean = sum / numArray.size
    Log.d("FIDO-AC","mean:" + mean)

    for (num in numArray) {
        standardDeviation += Math.pow(num - mean, 2.0)
    }

    return Math.sqrt(standardDeviation / numArray.size)
}

var numArray:DoubleArray = DoubleArray(0)
var totalTime = 0.0

abstract class EIDInterface(tag: android.nfc.Tag, val stateBasket: StateBasket) {
    private val TAG = this.javaClass.simpleName
    private val KEYSTORE_ALIAS = "anon.fidoac"

    var passportService: PassportService
    //Workflow from https://github.com/tananaev/passport-reader/blob/master/app/src/main/java/com/tananaev/passportreader/MainActivity.java
    fun runReadPassportAndMediatorAttestation(): Pair<ByteArray,ArrayList<ByteArray>> {
        var paceSucceeded = false
        assert (stateBasket.relyingparty_challenge !=null && stateBasket.context!=null && stateBasket.bacKey!=null && stateBasket.client_challenge!=null
            && stateBasket.paceKey!=null)

        try {
            val time = measureTimeMillis{
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
                    throw java.lang.Exception("Cannot Select Applet.")
                }

                if (!paceSucceeded) {
                    try {
                        passportService.getInputStream(PassportService.EF_COM).read();
                        Log.d(TAG,"Done Commmon Read")
                    } catch (e: Exception) {
                        try{
                            passportService.doBAC(stateBasket.bacKey);
                            Log.d(TAG,"Done BAC")
                        }
                        catch (e: Exception){
                            Log.e(TAG,"BAC & PACE Failed. Returning\n\n")
                            Log.w(TAG,e)
                            throw java.lang.Exception("BAC & PACE Failed.")
                        }
                    }
                }
            }
            Log.d(TAG,"PACE Time:" + time.toString()+ "ms")

            var dg1File:DG1File? = null
            var dg14File:DG14File? = null
            var sodFile:SODFile? = null

            val time_read = measureTimeMillis {
                Log.d(TAG, "Reading SOD")
                sodFile = readSODFile(stateBasket.context!!)
                stateBasket.sodFile= sodFile
                //                Log.d(TAG, sodFile.toString())
                Log.d(TAG, "Reading DG1")
                dg1File = readDG1File(stateBasket.context!!)
//                Log.d(TAG, dg1File.toString())
                Log.d(TAG, "Reading DG14")
                dg14File = readDG14File(stateBasket.context!!)
//                Log.d(TAG, dg14File.toString())
            }
//            totalTime+= time_read*1.0
//            numArray += time_read*1.0
//            Log.d(TAG,"Time:" + time_read.toString()+ "ms")
//            Log.d(TAG,"Average Time:" + totalTime/loopCount+ "ms")
//            Log.d(TAG,"Standard Deviation:" + calculateSD(numArray)+ "ms")

            val time_check = measureTimeMillis {
                //NOTE: The following code scope is the Mediator part.

                // We perform Chip Authentication using Data Group 14, and ensure the same DG14 is used in PA and CA
                val passedPassive = passiveAuth(dg1File!!,dg14File!!, sodFile!!)
                if (passedPassive) {
                    Log.d(TAG, "Mediator: Passive OK, Do CA")
                    if (doChipAuth(passportService, dg14File!!)){
                        Log.d(TAG, "Mediator: CA OK")

                        val attestation = attest(sodFile!!.dataGroupHashes, this.stateBasket.client_challenge!!,
                            this.stateBasket.relyingparty_challenge!!
                        )
                        Log.d(TAG, "Mediator: Mediator Attestation Generated ")
                        return attestation
                    } else{
                        Log.d(TAG, "Mediator: CA Failed")
                    }

                } else {
                    Log.d(TAG, "Passive Failed")
                }
            }
            totalTime+= time_check*1.0 + time*1.0
            numArray += time_check*1.0 + time*1.0
            Log.d(TAG,"Time:" + time_check.toString()+ "ms")
            Log.d(TAG,"Average Time:" + totalTime/numArray.size+ "ms")
            Log.d(TAG,"Standard Deviation:" + calculateSD(numArray)+ "ms")
        } catch (e: Exception) {
            Log.w(TAG, e);
        }
        return Pair(ByteArray(0), ArrayList<ByteArray>())
    }

    private fun doChipAuth(service: PassportService, dg14File :DG14File): Boolean {
        //Here should reuse DG14 from PA, for the sake of simplicity, here the DG14 is passed as function parameter

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

                val keyPairGenerator =
                    KeyPairGenerator.getInstance(agreementAlg,
                        Util.getBouncyCastleProvider())

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

                //Can now read everything
                Log.d(TAG,"Left integration with SNARK and server")
                return true
            }
            catch(e:Exception){
                Log.w(TAG, e);
            }

        } catch (e: java.lang.Exception) {
            Log.w(TAG, e)
        }
        return false
    }
    private fun passiveAuth(dg1File:DG1File, dg14File:DG14File,sodFile:SODFile):Boolean{
        //Mediator PassiveAuth Part
        try {
            //Important: Check hash(dg14) == sodFile.dataGroupHashes's DG14
            val digest = MessageDigest.getInstance(sodFile.digestAlgorithm)
            val dataHashes = sodFile.dataGroupHashes
            var dg14Hash: ByteArray? = ByteArray(0)
            dg14Hash = digest.digest(stateBasket.dg14_raw)

            Log.d(TAG, dg14Hash.toHex())
            Log.d(TAG, dataHashes.get(14)!!.toHex())

            if (Arrays.equals(dg14Hash, dataHashes.get(14))){
                //Then perform PA as normal.
                //We also need to reconstruct SoDFile from the hashes in practice, notably sodFile.encryptedDigest.
                // For simplicity, the SoDFile is passed in here.

                // We retrieve the CSCA from the master list
                val asn1InputStream = ASN1InputStream(stateBasket.context!!.assets.open("masterList"))
                var p: ASN1Primitive?
                val keystore: KeyStore = KeyStore.getInstance(KeyStore.getDefaultType())
                keystore.load(null, null)
                val cf: CertificateFactory = CertificateFactory.getInstance("X.509")
                p= asn1InputStream.readObject()
                while ( p!= null) {
                    val asn1 = ASN1Sequence.getInstance(p)
                    require(!(asn1 == null || asn1.size() == 0)) { "null or empty sequence passed." }
                    require(asn1.size() == 2) { "Incorrect sequence size: " + asn1.size() }
                    val certSet = ASN1Set.getInstance(asn1.getObjectAt(1))
                    for (i in 0 until certSet.size()) {
                        val certificate: Certificate =
                            Certificate.getInstance(certSet.getObjectAt(i))
                        val pemCertificate: ByteArray = certificate.getEncoded()
                        val javaCertificate: java.security.cert.Certificate? =
                            cf.generateCertificate(ByteArrayInputStream(pemCertificate))
                        keystore.setCertificateEntry(i.toString(), javaCertificate)
                    }
                    p= asn1InputStream.readObject()
                }
                val docSigningCertificates: List<X509Certificate> = sodFile.docSigningCertificates
                for (docSigningCertificate in docSigningCertificates) {
                    docSigningCertificate.checkValidity()
                }

                // We check if the certificate is signed by a trusted CSCA
                // TODO: verify if certificate is revoked
                val cp: CertPath = cf.generateCertPath(docSigningCertificates)
                val pkixParameters = PKIXParameters(keystore)
                pkixParameters.setRevocationEnabled(false)
                val cpv: CertPathValidator =
                    CertPathValidator.getInstance(CertPathValidator.getDefaultType())
                cpv.validate(cp, pkixParameters)
                var sodDigestEncryptionAlgorithm = sodFile.docSigningCertificate.sigAlgName
                var isSSA = false
                if (sodDigestEncryptionAlgorithm == "SSAwithRSA/PSS") {
                    sodDigestEncryptionAlgorithm = "SHA256withRSA/PSS"
                    isSSA = true
                }
                val sign: Signature = Signature.getInstance(sodDigestEncryptionAlgorithm)
                if (isSSA) {
                    sign.setParameter(
                        PSSParameterSpec(
                            "SHA-256",
                            "MGF1",
                            MGF1ParameterSpec.SHA256,
                            32,
                            1
                        )
                    )
                }
                sign.initVerify(sodFile.docSigningCertificate)
                sign.update(sodFile.eContent)
                val passiveAuthSuccess = sign.verify(sodFile.encryptedDigest)
                return passiveAuthSuccess
            }
            else{
                Log.d(TAG, "Different DG14")
            }
        } catch (e: java.lang.Exception) {
            Log.w(TAG, e)
        }
        return false
    }
    private fun attest(dataHashes: MutableMap<Int, ByteArray>, clientNonce: ByteArray, serverChallenge: ByteArray ): Pair<ByteArray,ArrayList<ByteArray>>{
        //Sign( H(DG_H||nonce) || challenge)
        var certArray:ArrayList<ByteArray> = ArrayList<ByteArray>()
        try {
            //H(DG_H||nonce)
            val randomized_hash =
                MessageDigest.getInstance("SHA-256").digest(dataHashes[1]!! + clientNonce)
            val final_message = randomized_hash + serverChallenge

            val keypairgen = KeyPairGenerator.getInstance(
                KeyProperties.KEY_ALGORITHM_EC, "AndroidKeyStore"
            )
            // Alias for the AndroidKeyStore to retrieve
            val newSpec = KeyGenParameterSpec.Builder(
                KEYSTORE_ALIAS,
                KeyProperties.PURPOSE_SIGN or KeyProperties.PURPOSE_VERIFY
            )
                .setAlgorithmParameterSpec(ECGenParameterSpec("secp256r1"))
                .setDigests(KeyProperties.DIGEST_SHA256)
                .setAttestationChallenge(serverChallenge)
                .build()
            Log.i(TAG, "KeyPair newSpec")
            keypairgen.initialize(newSpec)
            val kp = keypairgen.generateKeyPair()
            Log.i(TAG, "KeyPair Gen Completed")

            val ks = KeyStore.getInstance("AndroidKeyStore")
            ks.load(null, null)
            val privateKey = ( ks.getEntry(KEYSTORE_ALIAS, null) as KeyStore.PrivateKeyEntry).privateKey
            //Log.i(TAG, "PrivateKey ${privateKey}")
            val signature: ByteArray = Signature.getInstance("SHA256withECDSA").run {
                initSign(privateKey)
                update(final_message)
                sign()
            }
            Log.i(TAG, "Signed")
            val cert = ks.getCertificateChain(KEYSTORE_ALIAS)
            val cert_x509 = arrayOfNulls<X509Certificate>(cert.size)
            for (i in cert.indices) {
                val ct = cert[i]
                //Log.i(TAG, "Cert: " + ct.type)
                //Log.i(TAG, "Public: " + (ct as X509Certificate).toString())
                cert_x509[i] = ct as X509Certificate
                certArray.add(cert_x509[i]!!.encoded)
            }
            //Log.i(TAG, "Public: $cert_x509")

            //val publicKey: PublicKey = cert.publicKey
            //Log.i(TAG, "Cert -> Public Key ${cert}")
            //   Log.d(TAG, cert.toString())

            return Pair(signature, certArray)
        }
        catch(e: Exception){
            Log.w(TAG, e)
        }

        return Pair(ByteArray(0), certArray)
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
                val dg1Encoded = fileInputStream.readBytes()
                stateBasket.dg1_raw = dg1Encoded
                val dg1InByte = ByteArrayInputStream(dg1Encoded)
                stateBasket.dg1_File = DG1File(dg1InByte)
                return stateBasket.dg1_File
            } catch (e: IOException) {
                Log.e(TAG, "Error reading cached file!")
            }
        }
        var file: DG1File? = null
        try {
            val dg1In: CardFileInputStream = passportService.getInputStream(PassportService.EF_DG1)
            val dg1Encoded = IOUtils.toByteArray(dg1In)
            stateBasket.dg1_raw = dg1Encoded
            val dg1InByte = ByteArrayInputStream(dg1Encoded)

            file = DG1File(dg1InByte)
            stateBasket.dg1_File = file
            //file = DG1File(passportService.getInputStream(PassportService.EF_DG1))
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
            bufferedOutputStream.write(stateBasket.dg1_raw)
            bufferedOutputStream.close()
            fileOutputStream?.close()
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
                val dg14Encoded = fileInputStream.readBytes()
                stateBasket.dg14_raw = dg14Encoded
                val dg14InByte = ByteArrayInputStream(dg14Encoded)
                return DG14File(dg14InByte)
            } catch (e: IOException) {
                Log.e(TAG, "Error reading cached file!")
            }
        }
        var file: DG14File? = null
        try {
            val dg14In: CardFileInputStream = passportService.getInputStream(PassportService.EF_DG14)
            val dg14Encoded = IOUtils.toByteArray(dg14In)
            stateBasket.dg14_raw = dg14Encoded
            val dg14InByte = ByteArrayInputStream(dg14Encoded)
            file = DG14File(dg14InByte)
        } catch (e: CardServiceException) {
            Log.e(TAG, "Error reading passport file")
        } catch (e: IOException) {
            Log.e(TAG, "Error reading passport file")
        }
        try {
            val fileOutputStream = context.openFileOutput("DG14File", Context.MODE_PRIVATE)
            val bufferedOutputStream = BufferedOutputStream(fileOutputStream)
            bufferedOutputStream.write(stateBasket.dg14_raw)
            bufferedOutputStream.close()
            fileOutputStream?.close()
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


