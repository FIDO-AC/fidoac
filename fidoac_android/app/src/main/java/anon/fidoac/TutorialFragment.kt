package anon.fidoac

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import androidx.fragment.app.Fragment
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import anon.fidoac.databinding.FragmentDataBinding
import anon.fidoac.databinding.FragmentTutorialBinding
import java.io.BufferedReader
import java.io.InputStreamReader
import java.lang.StringBuilder

// TODO: Rename parameter arguments, choose names that match
// the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
private const val ARG_PARAM1 = "param1"
private const val ARG_PARAM2 = "param2"

/**
 * A simple [Fragment] subclass.
 * Use the [TutorialFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class TutorialFragment : Fragment() {
    // TODO: Rename and change types of parameters
    private var param1: String? = null
    private var param2: String? = null

    private val TAG:String = "LogFragment"

    private var _binding: FragmentTutorialBinding? = null
    private val binding get() = _binding!!

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        arguments?.let {
            param1 = it.getString(ARG_PARAM1)
            param2 = it.getString(ARG_PARAM2)
        }
    }

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        // Inflate the layout for this fragment
        _binding =  FragmentTutorialBinding.inflate(inflater, container, false)
        Runtime.getRuntime().exec("logcat -c") //Clear logcat
        log2screenLoop()
        return binding.root
    }

    private fun log2screenLoop(){
        Handler(Looper.getMainLooper()).postDelayed({
            //Do something after 100ms
            log2screen()
            log2screenLoop()
        }, 1000)
    }
    private fun log2screen(){
        try {
            val targetTAG = "EIDEPassportInterface"
            val process = Runtime.getRuntime().exec("logcat -d -s $targetTAG")
            val bufferedReader = BufferedReader(
                InputStreamReader(process.inputStream)
            )
            val log = StringBuilder()
            var line: String? = ""
            while (bufferedReader.readLine().also { line = it } != null) {
                val newline = line?.slice(line!!.indexOf("$targetTAG")+TAG.length+1..line?.length!! -1)
//                val newline = line
                log.append(newline + System.getProperty("line.separator"))
            }
            val tv = binding.mainTextview as TextView
            tv.text = tv.text.toString() + log.toString()
            Runtime.getRuntime().exec("logcat -c") //Clear logcat
        } catch (e: Exception) {
            Log.i(TAG, e.toString())
            if (e.cause != null) {
                Log.i(TAG, e.cause.toString())
            }
        }
    }

    companion object {
        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param param1 Parameter 1.
         * @param param2 Parameter 2.
         * @return A new instance of fragment TutorialFragment.
         */
        // TODO: Rename and change types and number of parameters
        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            TutorialFragment().apply {
                arguments = Bundle().apply {
                    putString(ARG_PARAM1, param1)
                    putString(ARG_PARAM2, param2)
                }
            }
    }
}