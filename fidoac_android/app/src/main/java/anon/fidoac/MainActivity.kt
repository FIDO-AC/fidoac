package anon.fidoac

import android.content.Intent
import android.nfc.NfcAdapter
import android.nfc.Tag
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.View
import android.view.WindowManager
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import anon.fidoac.databinding.ActivityMainBinding
import java.io.BufferedReader
import java.io.InputStreamReader
import java.lang.StringBuilder

//Sources.
//https://fonts.google.com/icons?icon.query=home&icon.platform=android

class MainActivity : AppCompatActivity() {
    private val TAG:String = "fidoac_main"

    private lateinit var binding: ActivityMainBinding

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
    }


    /**
     * A native method that is implemented by the 'fidoac' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'fidoac' library on application startup.
        init {
            System.loadLibrary("fidoac")
        }
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