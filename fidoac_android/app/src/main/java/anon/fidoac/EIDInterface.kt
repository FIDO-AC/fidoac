package anon.fidoac

//import logoverwrite.Log

import android.content.Context
import android.nfc.Tag
import android.nfc.tech.IsoDep
import android.preference.PreferenceManager
import android.util.Log
import net.sf.scuba.smartcards.CardFileInputStream
import net.sf.scuba.smartcards.CardService
import net.sf.scuba.smartcards.CardServiceException
import net.sf.scuba.smartcards.ISO7816
import org.jmrtd.BACKey
import org.jmrtd.PACEKeySpec
import org.jmrtd.PassportService
import org.jmrtd.PassportService.DEFAULT_MAX_BLOCKSIZE
import org.jmrtd.PassportService.NORMAL_MAX_TRANCEIVE_LENGTH
import org.jmrtd.lds.CardAccessFile
import org.jmrtd.lds.PACEInfo
import org.jmrtd.lds.SODFile
import org.jmrtd.lds.SecurityInfo
import org.jmrtd.lds.icao.DG14File
import org.jmrtd.lds.icao.DG1File
import org.jmrtd.lds.icao.DG2File
import java.io.BufferedInputStream
import java.io.BufferedOutputStream
import java.io.IOException
import java.util.*


/**
 * Abstract class for electronic IDs like ePassport or German eID national ID card
 */
abstract class EIDInterface(tag: android.nfc.Tag, val stateBasket: StateBasket) {
    private val TAG = this.javaClass.simpleName
    var passportService: PassportService

    //Workflow from https://github.com/tananaev/passport-reader/blob/master/app/src/main/java/com/tananaev/passportreader/MainActivity.java
    fun runToReadDG1(bacKey:BACKey): Unit? {
        try {
            if (false){
                var paceSucceeded = false
                try {
                    val cardAccessFile:CardAccessFile = CardAccessFile(passportService.getInputStream(PassportService.EF_CARD_ACCESS))
                    for (securityInfo: SecurityInfo in cardAccessFile.securityInfos) {
                        if (securityInfo is PACEInfo) {
                            val paceInfo:PACEInfo = securityInfo
                            passportService.doPACE(bacKey, paceInfo.getObjectIdentifier(), PACEInfo.toParameterSpec(paceInfo.getParameterId()), null);
                            paceSucceeded = true;
                        }
                    }
                } catch (e: Exception) {
                    Log.w(TAG, e);
                }

                passportService.sendSelectApplet(paceSucceeded);

                if (!paceSucceeded) {
                    try {
                        passportService.getInputStream(PassportService.EF_COM).read();
                        Log.d(TAG,"Done Commmon Read")
                    } catch (e: Exception) {
                        passportService.doBAC(bacKey);
                        Log.d(TAG,"Done BAC")
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

            //            var dg1In:CardFileInputStream = passportService.getInputStream(PassportService.EF_DG1);
//            val dg1File = DG1File(dg1In)
//            Log.d(TAG,dg1File.toString())
//            val dg2In: CardFileInputStream = passportService.getInputStream(PassportService.EF_DG2);
//            val dg2File = DG2File(dg2In);
//            Log.d(TAG,dg2File.toString())
//            val sodIn: CardFileInputStream = passportService.getInputStream(PassportService.EF_SOD);
//            val sodFile = SODFile(sodIn);
//            Log.d(TAG,sodFile.toString())

//            // We perform Chip Authentication using Data Group 14
//            doChipAuth(service);
//            // Then Passive Authentication using SODFile
//            doPassiveAuth();

        } catch (e: Exception) {
            Log.w(TAG, e);

        }
        return null;
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

    abstract fun readDG1File(context: Context?): DG1File?
    abstract fun readDG14File(context: Context?): DG14File?
    abstract fun readSODFile(context: Context?): SODFile?
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
    override fun readDG1File(context: Context?): DG1File? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("DG1FileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            try {
                val fileInputStream = context?.openFileInput("DG1File")
                val bufferedInputStream = BufferedInputStream(fileInputStream)
//                return DG1File(bufferedInputStream)
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
            val fileOutputStream = context?.openFileOutput("DG1File", Context.MODE_PRIVATE)
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
    override fun readDG14File(context: Context?): DG14File? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("DG14FileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            try {
                val fileInputStream = context?.openFileInput("DG14File")
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
            val fileOutputStream = context?.openFileOutput("DG14File", Context.MODE_PRIVATE)
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
    override fun readSODFile(context: Context?): SODFile? {
        val sharedPref = PreferenceManager.getDefaultSharedPreferences(context)
        if (sharedPref.getBoolean("SODFileCached", false)) {
            Log.i(TAG, "Accessing cached DG.")
            val files = context?.fileList()
            if (files != null) {
                for (fileName in files) {
                    Log.i(TAG, "Available file: $fileName")
                }
            }
            try {
                val fileInputStream = context?.openFileInput("SODFile")
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


