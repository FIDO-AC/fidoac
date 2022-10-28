package anon.fidoac

import android.content.Intent
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import anon.fidoac.databinding.ActivityMainBinding
import anon.fidoac.service.FIDOACService
import java.util.*

//Sources.
//https://fonts.google.com/icons?icon.query=home&icon.platform=android

class MainActivity : AppCompatActivity() {
    private val TAG:String = "fidoac_main"

    private lateinit var binding: ActivityMainBinding

    private var is_insession:Boolean = false
    public var session_server_challenge:ByteArray? = null

    //Called afer oncreate and resuming after onstop
    override fun onStart() {
        super.onStart()
        val challenge = this.intent?.data?.getQueryParameter("challenge")
        if (challenge != null && !is_insession) {
            Log.d(TAG,challenge)
            val decodedChallengeByteArray: ByteArray =
                Base64.getDecoder().decode(challenge)
            session_server_challenge = decodedChallengeByteArray
            is_insession = true
            //Service will be started after completing all necessary procedures in the application.
            //val mIntent = Intent(this, FIDOACService::class.java)
            //mIntent.putExtra("challenge", challenge)
            //startService(mIntent)
        } else {
            Log.i("FIDOAC", "Challenge not found")
            if (!is_insession){
                //NOTE:!! Mocking Challenge..
                session_server_challenge = byteArrayOf(0x1, 0x20, 0x30, 0x4)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        //Prevent soft keyboard popup from adjusting viewport
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);

        // Example of a call to a native method
        //binding.sampleText.text = stringFromJNI()

        //Disable temporary because UI is not adaptive to dark mode.
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);

//        Integer.toString(snark_sha256())
    }


    /**
     * A native method that is implemented by the 'fidoac' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun snark_sha256(dg1:ByteArray,cleintNonce:ByteArray,ageLimit:Int, provingKey:ByteArray,
                              dg1_hash_for_testing:ByteArray): ByteArray



    companion object {
        // Used to load the 'fidoac' library on application startup.
        init {
            System.loadLibrary("fidoac")
        }
        external fun FIDO_AC_veirfy(zkproof:ByteArray, randomized_hash:ByteArray, ageLimit: Int, verificationKey:ByteArray) : Boolean
    }

//    Use Live Detection instead of Intent Detection.
//    override fun onNewIntent(intent: Intent) {
//        super.onNewIntent(intent)
//        Log.d(TAG,intent.toString())
//        if (NfcAdapter.ACTION_TECH_DISCOVERED == intent.action) {
//            val tag: Tag? = intent.getParcelableExtra(NfcAdapter.EXTRA_TAG)
//            Log.d(TAG,tag.toString())
////            val stateBasket = StateBasket(tag, getPaceKey(), iasRootStream, this)
//            //PassportReader().execute(stateBasket)
//        }
//    }
}