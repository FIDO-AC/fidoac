package anon.fidoac

import android.content.Context
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import anon.fidoac.databinding.ActivityMainBinding
import com.android.volley.Request
import com.android.volley.Response
import com.android.volley.toolbox.StringRequest
import com.android.volley.toolbox.Volley
import java.io.BufferedInputStream
import java.io.BufferedReader
import java.io.InputStream
import java.io.InputStreamReader
import java.net.HttpURLConnection
import java.net.URL
import java.util.*
import kotlin.concurrent.thread

//Sources.
//https://fonts.google.com/icons?icon.query=home&icon.platform=android

class MainActivity : AppCompatActivity() {
    private val TAG:String = "fidoac_main"

    private lateinit var binding: ActivityMainBinding

    private var is_insession:Boolean = false
    public var session_server_challenge:ByteArray? = null

    var proving_key:ByteArray = ByteArray(0)
    var verfication_key:ByteArray = ByteArray(0)

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
        val sharedPref = this.getSharedPreferences("zkproof_crs", Context.MODE_PRIVATE)
        sharedPref?.let {
            if (sharedPref.contains("PK") && sharedPref.contains("VK")) {
                this.proving_key = Base64.getDecoder().decode(sharedPref.getString("PK", "")!!)
                this.verfication_key = Base64.getDecoder().decode(sharedPref.getString("VK", "")!!)
            } else {
                // Instantiate the RequestQueue.
                val queue = Volley.newRequestQueue(this)
                val url = "https://fido.westeurope.cloudapp.azure.com/fidoac-server/trustedSetup.json"

                // Request a string response from the provided URL.
                val stringRequest = StringRequest(
                    Request.Method.GET, url,
                    Response.Listener<String> { response ->
                        // Display the first 500 characters of the response string.
                        Log.d(TAG,"Response is: ${response.substring(0, 500)}")
                    },
                    Response.ErrorListener {
                        Log.e(TAG,"That didn't work!" + it.message + " \n" + it.stackTraceToString())
                    }

                )

                //The above wont work because the server dint setup the certificate correctly. Instead we use hardcorded value here WLOG.

            }
        }

        Log.d(TAG,"Proving Key Size:"+ this.proving_key.size)
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