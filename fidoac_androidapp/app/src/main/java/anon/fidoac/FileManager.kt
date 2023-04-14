package anon.fidoac

import android.content.Context
import android.util.Log
import com.google.gson.JsonObject
import org.json.JSONObject
import java.util.*


fun ByteArray.toBase64(): String = String(Base64.getUrlEncoder().encode(this))
fun String.toByteArrayFromBase64(): ByteArray = Base64.getUrlDecoder().decode(this)

//Not Thread Safe
class FileManager {
    companion object {
        private val TAG = "FileManager"

        private const val CONFIG : String = "CONFIG"

        private const val CRS_PK_FILE : String = "CRS_PK"
        private const val CRS_VK_FILE : String = "CRS_VK"
        private const val PROOF_TEST_FILE : String = "PROOF"

        const val PROOF_ID : String = "PROOF_ID"
        const val HASH_ID : String = "HASH_ID"
        const val SERVER_CHALLENGE_ID : String = "SERVER_CHALLENGE_ID"
        const val MEDIATOR_SIGNATURE_ID : String = "MEDIATOR_SIGNATURE_ID"
        const val MEDIATOR_CERT_ID : String = "MEDIATOR_CERT_ID"
        const val AGEGT_ID : String = "AGEGT_ID"
        const val CURYEAR_ID : String = "CURYEAR_ID"
        const val INIT_ID : String = "INIT_ID"

        private lateinit var ctx: Context

        fun init(context: Context, to_reset: Boolean = false) {
            ctx = context

            val sharedPref = ctx.getSharedPreferences(CONFIG, Context.MODE_PRIVATE)
            with (sharedPref.edit()) {
                if (!sharedPref.getBoolean(INIT_ID,false)){
                    putBoolean(INIT_ID, true)
                    resetStorage()
                    commit()
                }
            }

            if (to_reset){
                resetStorage()
            }
        }
        fun resetStorage(){
            //Will create if none exist.
            ctx.getSharedPreferences(PROOF_TEST_FILE, Context.MODE_PRIVATE).edit().clear().commit()

            //Read from APK, could be remote as well
            val proving_key_bytes = ctx.getResources().openRawResource(R.raw.provingkey).readBytes()
            val verf_key_bytes = ctx.getResources().openRawResource(R.raw.verificationkey).readBytes()
            savePVKeys(proving_key_bytes, verf_key_bytes)
        }

        fun savePVKeys(prove_key : ByteArray, verf_key : ByteArray) : Boolean {
            ctx.openFileOutput(CRS_PK_FILE, Context.MODE_PRIVATE).use {
                it.write(prove_key)
            }
            ctx.openFileOutput(CRS_VK_FILE, Context.MODE_PRIVATE).use{
                it.write(verf_key)
            }

            return true
        }
        fun loadPVKeys() : Pair<ByteArray, ByteArray>? {
            val start = System.nanoTime()
            val proving_key = ctx.openFileInput(CRS_PK_FILE).readBytes()
            val verification_key = ctx.openFileInput(CRS_VK_FILE).readBytes()

            val duration = System.nanoTime() - start
            Log.d(TAG, String.format("Time Elapsed for Load PV from LocalStorage:%.3f ms", duration/1000000.0))

            return Pair<ByteArray, ByteArray>(proving_key,verification_key)
        }

        //For testing only
        fun saveProof(proof : ByteArray, hashValue : ByteArray, ageGt: Int, curYear: Int): Boolean{
            val sharedPref = ctx.getSharedPreferences(PROOF_TEST_FILE, Context.MODE_PRIVATE) ?: return false
            //Commit => syn, apply => asyn
            with (sharedPref.edit()) {
                putString(PROOF_ID, proof.toBase64())
                putString(HASH_ID, hashValue.toBase64())
                putInt(AGEGT_ID, ageGt)
                putInt(CURYEAR_ID, curYear)
                commit()
            }

            val jsonObject: JSONObject = JSONObject();
            jsonObject.put(PROOF_ID, proof.toBase64());
            jsonObject.put(HASH_ID, hashValue.toBase64());
            jsonObject.put(CURYEAR_ID, curYear);
            jsonObject.put(AGEGT_ID, ageGt);
            val jsonStringValue: String = jsonObject.toString()
            ctx.openFileOutput(PROOF_TEST_FILE, Context.MODE_PRIVATE).use {
                it.write(jsonStringValue.toByteArray())
            }

            return true
        }
    }
}