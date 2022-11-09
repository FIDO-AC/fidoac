package anon.fidoac

import android.content.Context
import android.os.Bundle
import android.os.StrictMode
import android.util.Log
import android.view.WindowManager
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import anon.fidoac.databinding.ActivityMainBinding
//import com.android.volley.Request
//import com.android.volley.Response
//import com.android.volley.toolbox.StringRequest
//import com.android.volley.toolbox.Volley
import java.net.HttpURLConnection
import java.net.URL
import java.util.*
import kotlin.concurrent.thread
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.json.Json
import okhttp3.OkHttpClient
import okhttp3.Request
import java.io.*

//Sources.
//https://fonts.google.com/icons?icon.query=home&icon.platform=android

class MainActivity : AppCompatActivity() {
    private val TAG:String = "fidoac_main"

    private lateinit var binding: ActivityMainBinding

    private var is_insession:Boolean = false
    public var session_server_challenge:ByteArray? = null

    var proving_key:ByteArray = ByteArray(0)
    var verfication_key:ByteArray = ByteArray(0)

    var origin = "None"
    var request = "None"

    var created =false

    //Called afer oncreate and resuming after onstop
    override fun onStart() {
        super.onStart()
        val challenge = this.intent?.data?.getQueryParameter("challenge")
        val received_origin = this.intent?.data?.getQueryParameter("origin")
        val received_request = "Age>=" + this.intent?.data?.getQueryParameter("ageQueryGT")
        if (challenge != null && !is_insession && received_origin!=null && received_request!=null) {
            Log.d(TAG,received_origin)
            Log.d(TAG,received_request)

            Log.d(TAG,challenge)
            val decodedChallengeByteArray: ByteArray =
                Base64.getDecoder().decode(challenge)
            session_server_challenge = decodedChallengeByteArray
            is_insession = true
            //Service will be started after completing all necessary procedures in the application.
            //val mIntent = Intent(this, FIDOACService::class.java)
            //mIntent.putExtra("challenge", challenge)
            //startService(mIntent)

            this.origin = received_origin
            this.request = received_request
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

        if (created){
            return
        }
        created= true

//        Integer.toString(snark_sha256())
        val sharedPref = this.getSharedPreferences("zkproof_crs", Context.MODE_PRIVATE)
        if (false){ //Key Generation for testing purpose only
            var prov_key_ba :ByteArray = ByteArray(100000000) //~57,019,458
            var verf_key_ba :ByteArray = ByteArray(100000) //~10,000
            var array_sizes: IntArray = IntArray(2)
            MainActivity.generate_trusted_setup(verf_key_ba,prov_key_ba,20, array_sizes)
            Log.d(TAG,"Saving key")
            val verf_key_ba_trimmed:ByteArray = ByteArray(array_sizes[0])
            System.arraycopy(verf_key_ba,0,verf_key_ba_trimmed,0,verf_key_ba_trimmed.size)
            val prov_key_ba_trimmed:ByteArray = ByteArray(array_sizes[1])
            System.arraycopy(prov_key_ba,0,prov_key_ba_trimmed,0,prov_key_ba_trimmed.size)
            assert( Arrays.equals( Base64.getDecoder().decode( String(Base64.getEncoder().encode(verf_key_ba_trimmed)) ), verf_key_ba_trimmed ) )
            assert( Arrays.equals( Base64.getDecoder().decode( String(Base64.getEncoder().encode(prov_key_ba_trimmed)) ), prov_key_ba_trimmed ) )
            sharedPref.edit().putString("VK", Base64.getEncoder().encodeToString(verf_key_ba_trimmed)).commit()
            sharedPref.edit().putString("PK", Base64.getEncoder().encodeToString(prov_key_ba_trimmed)).commit()
            assert(Arrays.equals( Base64.getDecoder().decode(sharedPref.getString("PK", "")!!), prov_key_ba_trimmed))
            assert(Arrays.equals( Base64.getDecoder().decode(sharedPref.getString("VK", "")!!), verf_key_ba_trimmed))
            //sharedPref.edit().putString("VK", Base64.getEncoder().encodeToString(verf_key_ba.slice(0..array_sizes[0]-1).toByteArray())).commit()
            //sharedPref.edit().putString("PK", Base64.getEncoder().encodeToString(prov_key_ba.slice(0..array_sizes[1]-1).toByteArray())).commit()
        }

        sharedPref?.let {
            if (sharedPref.contains("PK") && sharedPref.contains("VK")) { //
                this.proving_key = Base64.getDecoder().decode(sharedPref.getString("PK", "")!!)
                this.verfication_key = Base64.getDecoder().decode(sharedPref.getString("VK", "")!!)
            } else {
                // Instantiate the RequestQueue.
                val url = "https://fido.westeurope.cloudapp.azure.com/trustedsetup.json"
                //Synchronous Request for now.
                val policy = StrictMode.ThreadPolicy.Builder().permitAll().build()
                StrictMode.setThreadPolicy(policy)
                val client : OkHttpClient = OkHttpClient();
                val request = Request.Builder()
                    .url(url)
                    .build()
                client.newCall(request).execute().use { response ->
                    if (!response.isSuccessful) throw IOException("Unexpected code $response")
                    for ((name, value) in response.headers) {
                        Log.d(TAG,"$name: $value")
                    }
                    Log.d(TAG,"Parsing JSON")
                    val res = Json.decodeFromString<Map<String, String>>(response.body!!.string())
                    Log.d(TAG, response.body!!.string())
                    Log.d(TAG,"Saving to Shared Pref")
                    sharedPref.edit().putString("PK", res["PK"]).commit()
                    sharedPref.edit().putString("VK", res["VK"]).commit()
                    Log.d(TAG,"Testing Equality")
                    assert( Arrays.equals( Base64.getDecoder().decode(res["PK"]),  Base64.getDecoder().decode(sharedPref.getString("PK", "")!!)))
                    assert( Arrays.equals( Base64.getDecoder().decode(res["VK"]),  Base64.getDecoder().decode(sharedPref.getString("VK", "")!!)))
                }
                Log.d(TAG,"Requesting URL")
            }
            //Reset for testing purposes
            //this.proving_key = ByteArray(0)
            //this.verfication_key = ByteArray(0)
        }

        Log.d(TAG,"Proving Key Size:"+ this.proving_key.size)
    }


    /**
     * A native method that is implemented by the 'fidoac' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun snark_sha256(dg1:ByteArray,cleintNonce:ByteArray,ageLimit:Int, provingKey:ByteArray,
                              dg1_hash_for_testing:ByteArray, verf_key_for_testing: ByteArray): ByteArray



    companion object {
        // Used to load the 'fidoac' library on application startup.
        init {
            System.loadLibrary("fidoac")
        }
        external fun FIDO_AC_veirfy(zkproof:ByteArray, randomized_hash:ByteArray, ageLimit: Int, verificationKey:ByteArray) : Boolean

        //This is used to
        external fun generate_trusted_setup(verificationKey: ByteArray, provingKey: ByteArray, ageLimit: Int, arr_sizes:IntArray):Int
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