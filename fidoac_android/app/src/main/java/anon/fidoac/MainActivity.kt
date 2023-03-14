package anon.fidoac

import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import anon.fidoac.databinding.ActivityMainBinding
import java.security.SecureRandom
import java.util.*

interface RustCallBack {
    fun setProof (proof: ByteArray)
    fun setDigest(digest:ByteArray) //For testing
}

class MainActivity : AppCompatActivity(), RustCallBack {
    private lateinit var binding: ActivityMainBinding
    private val TAG = "FIDOAC"

    private lateinit var proof: ByteArray
    private lateinit var digest: ByteArray

    private var is_insession:Boolean = false
    private var is_rustinit:Boolean = false
    public var session_server_challenge:ByteArray? = null
    override fun onStart() {
        super.onStart()

        //Try to parse challenge. Challenge is passed by calling the application. OnStart let us parse when app resumed/created.
        val challenge = this.intent?.data?.getQueryParameter("challenge")
        if (challenge != null && !is_insession) {
            Log.d(TAG,challenge)
            val decodedChallengeByteArray: ByteArray =
                Base64.getDecoder().decode(challenge)
            session_server_challenge = decodedChallengeByteArray
            is_insession = true
        } else {
            Log.i("FIDOAC", "Challenge not found")
            if (!is_insession){
                //NOTE: Mocking Challenge for testing purpose. [Open in standalone mode].
                session_server_challenge = byteArrayOf(0x1, 0x20, 0x30, 0x4)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val to_reset = false
        FileManager.init(this, to_reset)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        //Prevent soft keyboard popup from adjusting viewport
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
        //Disabled because UI is not adaptive to dark mode.
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);

        //Rust
        if (!is_rustinit){
            rust_init()
//            //For testing only
//            val (pk,vk) = FileManager.Companion.loadPVKeys()!!
//            val age_minreq = 20;
//            val cur_year = 23;
//            val client_nonce:ByteArray = ByteArray(32);
//            val random = SecureRandom()
//            random.nextBytes(client_nonce)
//            val dg1:ByteArray = ByteArray(32);
//            val is_valid_proof = rust_fidoacprove(this as Any,pk,vk,dg1,client_nonce,age_minreq,cur_year);
//            Log.d(TAG,String.format("Age:%d, AgeMinReq:%d ,isProofValid:%b",28,age_minreq, is_valid_proof))
//            FileManager.saveProof(proof,digest,age_minreq,cur_year)
//            is_rustinit= true
        }
    }

    /**
     * A native method that is implemented by the 'rusttest' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun rust_init() : Unit
    external fun rust_fidoacprove(obj:Any, provKey: ByteArray, verfKey: ByteArray,dg1:ByteArray,client_nonce: ByteArray, age_gt: Int, cur_year: Int) : Boolean
    private external fun greeting(pattern: String): String?
    fun sayHello(to: String): String? {
        return greeting(to)
    }

    companion object {
        // Used to load the 'rusttest' library on application startup.
        init {
//            System.loadLibrary("rusttest"); //C++ library
            System.loadLibrary("rust"); //rust library
        }
    }

    override fun setProof (proof: ByteArray){
        Log.d(TAG, "Proof from MainAct")
        this.proof = proof
    }
    override fun setDigest(digest: ByteArray) {
        this.digest = digest
        Log.d(TAG, "Digest from MainAct")
        Log.d(TAG, digest.toHexString())
    }
    fun getProof(): ByteArray{
        return this.proof
    }
    fun getDigest(): ByteArray{
        return this.digest
    }
    fun ByteArray.toHexString() : String {
        return this.joinToString("") {
            java.lang.String.format("%02x", it)
        }
    }
}