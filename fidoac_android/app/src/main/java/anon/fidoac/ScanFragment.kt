package anon.fidoac

import android.graphics.drawable.AnimatedVectorDrawable
import android.graphics.drawable.Drawable
import android.nfc.NfcAdapter
import android.nfc.Tag
import android.nfc.tech.IsoDep
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.content.ContextCompat
import androidx.core.content.ContextCompat.getColor
import androidx.core.content.ContextCompat.getDrawable
import androidx.core.graphics.drawable.toBitmap
import androidx.fragment.app.Fragment
import androidx.vectordrawable.graphics.drawable.Animatable2Compat
import androidx.vectordrawable.graphics.drawable.AnimatedVectorDrawableCompat
import anon.fidoac.databinding.FragmentScanBinding
import com.google.android.material.tabs.TabLayout
import org.jmrtd.BACKey
import org.jmrtd.BACKeySpec
import org.jmrtd.PACEKeySpec
import java.security.GeneralSecurityException


//TODO display information about incoming stuff, context sensitive button ->

//TODO


// TODO: Rename parameter arguments, choose names that match
// the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
private const val ARG_PARAM1 = "param1"
private const val ARG_PARAM2 = "param2"

/**
 * A simple [Fragment] subclass.
 * Use the [ScanFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class ScanFragment : Fragment(), NfcAdapter.ReaderCallback{
    // TODO: Rename and change types of parameters
    private var param1: String? = null
    private var param2: String? = null

    var passportnumgetter: (()->String)? = null
    var dobgetter: (()->String)? = null
    var expdategetter: (()->String)? = null
    var isDataValid: ((Boolean)->Boolean)? = null
    //Navigate back to when complete
    lateinit var mtab_layout: TabLayout

    private var stateBasket: StateBasket? = null
    private var mNfcAdapter: NfcAdapter? = null
    private val TAG:String = "scan"

    private var _binding: FragmentScanBinding? = null
    private val binding get() = _binding!!
    private lateinit var animatedVectorDrawable:AnimatedVectorDrawableCompat

    fun getPaceKey(documentNumber: String, birthDate: String, expiryDate: String): PACEKeySpec? {
        val bacKeySpec: BACKeySpec = object : BACKeySpec {
            override fun getDocumentNumber(): String {
                return documentNumber!!
            }
            override fun getDateOfBirth(): String {
                return birthDate!!
            }
            override fun getDateOfExpiry(): String {
                return expiryDate!!
            }
            override fun getAlgorithm(): String? {
                return null
            }
            override fun getKey(): ByteArray {
                return ByteArray(0)
            }
        }
        var paceKeySpec: PACEKeySpec? = null
        try {
            Log.i(TAG, bacKeySpec.toString())
            paceKeySpec = PACEKeySpec.createMRZKey(bacKeySpec)
        } catch (e: GeneralSecurityException) {
            Log.e(TAG, "Error initializing MRZKey")
            e.message?.let { Log.e(TAG, it) }
        }
        return paceKeySpec
    }
    fun getBacKey(documentNumber: String, birthDate: String, expiryDate: String): BACKey {
        return BACKey(documentNumber,birthDate, expiryDate)
    }

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
        _binding = FragmentScanBinding.inflate(inflater, container, false)
        val view = binding.root
//        binding.scanbutton.setOnClickListener {
//            if (isDataValid!!(true)){ startScanning() }
//            else{ mtab_layout.getTabAt(1)?.select() }
//        }
        binding.animatedscanBtn.setOnClickListener {
            if (isDataValid!!(true)){
                startScanning()
            }
            else{
                //Data not valid.
                mtab_layout.getTabAt(1)?.select()
            }
        }
        binding.rejectBtnTextview.setOnClickListener {
            stopScanningReject()
            rejectAndSendDataBack()

        }

//        NFC Logic
//        https://stackoverflow.com/questions/64920307/how-to-write-ndef-records-to-nfc-tag/64921434#64921434
        this.mNfcAdapter = NfcAdapter.getDefaultAdapter(this.context);

        return view
    }

    override fun onResume() {
        super.onResume()
    }

    override fun onPause() {
        super.onPause()
        stopScanning()
    }

    private var isScanning = false
    private fun startScanning(){
        if (isScanning) return

        isScanning = true
        animatedVectorDrawable = AnimatedVectorDrawableCompat.create(this.requireContext(), R.drawable.avd_anim)!!
        binding.scanImageView.setImageDrawable(animatedVectorDrawable)
        animatedVectorDrawable?.registerAnimationCallback(object : Animatable2Compat.AnimationCallback() {
            override fun onAnimationEnd(drawable: Drawable?) {
                binding.scanImageView.post { animatedVectorDrawable.start() }
            }
        })
        animatedVectorDrawable?.start()

        val paceKey = getPaceKey(passportnumgetter!!(), dobgetter!!(), expdategetter!!())
        val bacKey = getBacKey(passportnumgetter!!(), dobgetter!!(), expdategetter!!())
        Log.d(TAG,"Passport:"+ passportnumgetter!!() + "\tDOB:" + dobgetter!!() + "\tEXPDATE:"+expdategetter!!())
        this.stateBasket =
            StateBasket(
                bacKey, paceKey!!, this.requireContext()
            )
        if (mNfcAdapter != null) {
            val options = Bundle()
            // Work around for some broken Nfc firmware implementations that poll the card too fast
            options.putInt(NfcAdapter.EXTRA_READER_PRESENCE_CHECK_DELAY, 250)
            mNfcAdapter!!.enableReaderMode(
                this.requireActivity(),
                this,
                NfcAdapter.FLAG_READER_NFC_A or
                        NfcAdapter.FLAG_READER_NFC_B or
                        NfcAdapter.FLAG_READER_NFC_F or
                        NfcAdapter.FLAG_READER_NFC_V or
                        NfcAdapter.FLAG_READER_NFC_BARCODE,
//                        NfcAdapter.FLAG_READER_NO_PLATFORM_SOUNDS,
                options
            )
        }

        binding.instructionInfo.setText(getString(R.string.scanning))
        binding.animatedscanBtn.startAnimation()
    }
    private fun stopScanning(){
        if (!isScanning) return
        //Runned on NON-UI Thread

        if (mNfcAdapter != null) mNfcAdapter!!.disableReaderMode(this.requireActivity()) //Disable after we read the data
        this.requireActivity().runOnUiThread(Runnable {
            binding.instructionInfo.setText(getString(R.string.postscanning))
            binding.animatedscanBtn.doneLoadingAnimation(0, ContextCompat.getDrawable(this.requireContext(), R.drawable.ic_baseline_done_24_white)!!
            .toBitmap())
            binding.scanImageView.setImageDrawable(ContextCompat.getDrawable(this.requireContext(), R.drawable.ic_launcher_foreground_maincolor))
        })
        isScanning = false
    }
    private fun stopScanningReject(){
        binding.instructionInfo.setText("Rejected. Downgrade to FIDO")
//        binding.animatedscanBtn.doneLoadingAnimation(0, ContextCompat.getDrawable(this.requireContext(), R.drawable.ic_baseline_cancel_24)!!.toBitmap())
        binding.scanImageView.setImageDrawable(ContextCompat.getDrawable(this.requireContext(), R.drawable.ic_baseline_cancel_24))

        if (!isScanning) return
        if (mNfcAdapter != null) mNfcAdapter!!.disableReaderMode(this.requireActivity()) //Disable after we read the data
        isScanning = false
    }

    //Send reject information back to browser
    private fun rejectAndSendDataBack(){
        //TODO have a server to send back data to.

        exit()
    }

    private fun sendDataToHTMLServer(){
        //TODO have a server to send back data to.

        exit()
    }


    private fun exit(){
        Handler(Looper.getMainLooper()).postDelayed({
            this.requireActivity().finishAndRemoveTask()
        }, 1000)
    }

    // This method is run in another thread when a card is discovered
    // This method cannot cannot direct interact with the UI Thread
    // Use `runOnUiThread` method to change the UI from this method
    override fun onTagDiscovered(tag: Tag) {
        // Card should be an IsoDep Technology Type
        var mIsoDep = IsoDep.get(tag)
        mIsoDep.timeout = 5000

        // Check that it is an mIsoDep capable card
        if (mIsoDep != null) {
            Log.d(TAG, "Detected NFC device")
            this.stateBasket?.tag  = tag
            this.stateBasket?.let { it.eidInterface =
                it.tag?.let { it1 -> EIDEPassportInterface(it1, this.stateBasket!!) }
            }
            this.stateBasket?.eidInterface?.runToReadDG1(this.stateBasket!!.bacKey)
            stopScanning()
            sendDataToHTMLServer()
        }

//            try {
//                val reader = PassportReader()
//                mIsoDep.timeout = 10000
//                this.stateBasket?.let {
//                    it.tag = tag
//                    //At the moment for testing only run BAC.
//                    reader.establishBACWithPassport(it)
////                    if (!reader.establishPACEWithPassport(it)){
////                        //Failed PACE, run BAC
////                        reader.establishBACWithPassport(it)
////                    }
//                }
    }

    companion object {
        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param param1 Parameter 1.
         * @param param2 Parameter 2.
         * @return A new instance of fragment ScanFragment.
         */
        // TODO: Rename and change types and number of parameters
        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            ScanFragment().apply {
                arguments = Bundle().apply {
                    putString(ARG_PARAM1, param1)
                    putString(ARG_PARAM2, param2)
                }
            }
    }
}