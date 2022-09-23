package anon.fidoac


import android.content.Context
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import android.view.View.OnTouchListener
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.widget.TextView.OnEditorActionListener
import android.widget.Toast
import androidx.camera.view.PreviewView
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.fragment.app.Fragment
import anon.fidoac.databinding.FragmentDataBinding
import com.google.android.material.datepicker.CalendarConstraints
import com.google.android.material.datepicker.MaterialDatePicker
import com.google.android.material.tabs.TabLayout
import java.text.SimpleDateFormat
import java.util.*

//TODO hide data all.
//TODO clean data all.
//TODO encrypt at rest

// TODO: Rename parameter arguments, choose names that match
// the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
private const val ARG_PARAM1 = "param1"
private const val ARG_PARAM2 = "param2"

/**
 * A simple [Fragment] subclass.
 * Use the [DataFragment.newInstance] factory method to
 * create an instance of this fragment.
 */
class DataFragment : Fragment() {
    // TODO: Rename and change types of parameters
    private var param1: String? = null
    private var param2: String? = null

    //Semaphore - Prevent DatePicker from crashing when tabbed on twice.
    private var expdate_dialogcount = 0
    private var dob_dialogcount = 0

    //Navigate back to when complete
    private lateinit var mtab_layout:TabLayout
    private lateinit var cameraOCR:DataOCR

    private val displayDF = "dd/MM/yyyy"
    private val displayDFFormatter = SimpleDateFormat(displayDF)
    private val processDF = "yyMMdd"
    private val processDFFormatter = SimpleDateFormat(processDF)

    private var _binding: FragmentDataBinding? = null
    // This property is only valid between onCreateView and
    // onDestroyView.
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
        // return inflater.inflate(R.layout.fragment_data, container, false)
        _binding = FragmentDataBinding.inflate(inflater, container, false)
        val view = binding.root

        //Showing Date in more readable form. Need to be parsed to yymmdd later.
        val formattedDate = "01/01/2000"
        val date = displayDFFormatter.parse(formattedDate)
        val timeInMillis = date.time
        val constraintBuilder = CalendarConstraints.Builder().setOpenAt(
            timeInMillis //pass time in milli seconds
        ).build()
        val expdate_picker = MaterialDatePicker.Builder.datePicker()
            .setTitleText("Expiration Date")
            .setCalendarConstraints(constraintBuilder)
            .build()
        // Do what you want
        expdate_picker.addOnPositiveButtonClickListener {
            val date = Date(it)
            val formattedDate = displayDFFormatter.format(date)
            expdate_dialogcount=0
            binding.textinputlayoutExpdate.editText?.setText(formattedDate)
            binding.textinputlayoutExpdate.error= null
        }
        expdate_picker.addOnNegativeButtonClickListener(){
            expdate_dialogcount=0
        }
        expdate_picker.addOnCancelListener {
            expdate_dialogcount=0
        }
        expdate_picker.addOnDismissListener {
            expdate_dialogcount=0
        }

        val dobdate_picker = MaterialDatePicker.Builder.datePicker()
            .setTitleText("Date of Birth")
            .setCalendarConstraints(constraintBuilder)
            .build()
        // Do what you want
        dobdate_picker.addOnPositiveButtonClickListener {
            val date = Date(it)
            val formattedDate = displayDFFormatter.format(date)
            dob_dialogcount=0
            binding.textinputlayoutDob.editText?.setText(formattedDate)
            binding.textinputlayoutDob.error= null
        }
        dobdate_picker.addOnNegativeButtonClickListener(){
            dob_dialogcount=0
        }
        dobdate_picker.addOnCancelListener {
            dob_dialogcount=0
        }
        dobdate_picker.addOnDismissListener {
            dob_dialogcount=0
        }

        binding.textinputExpdate.setOnTouchListener(OnTouchListener { v, event ->
            if (event.action == MotionEvent.ACTION_UP) {
                binding.textinputlayoutPassnum.editText?.clearFocus()
                if (expdate_dialogcount == 0){
                    expdate_dialogcount=1
                    expdate_picker.show(requireActivity().supportFragmentManager, "materialDatePicker")
                }
                true
            } else false
        })
        binding.textinputDob.setOnTouchListener(OnTouchListener { v, event ->
            if (event.action == MotionEvent.ACTION_UP) {
                binding.textinputlayoutPassnum.editText?.clearFocus()
                if (dob_dialogcount == 0){
                    dob_dialogcount=1
                    dobdate_picker.show(requireActivity().supportFragmentManager, "materialDatePicker")
                }
                true
            } else false
        })
        binding.textinputPassnum.setOnFocusChangeListener { view, hasFocus ->
            if (!hasFocus){
                //Losing Focus
                if (!binding.textinputlayoutPassnum.editText!!.text.toString().isEmpty()){
                    //Request Input Passport Number
                    binding.textinputlayoutPassnum.error = null
                }
            }
        }
        binding.textinputPassnum.setOnEditorActionListener(OnEditorActionListener { v, actionId, event ->
            if (actionId == EditorInfo.IME_ACTION_DONE) {
                binding.textinputPassnum.clearFocus()
                true
            }
            false
        })

        binding.buttonConfirm.setOnClickListener(View.OnClickListener {
            if (isDataValid(true)){
                //Save to shared preferences, then shift to scan
                //TODO save to preferences encrypted
                val sharedPref = activity?.getSharedPreferences("Data", Context.MODE_PRIVATE)
                if (sharedPref != null) {
                    with (sharedPref.edit()) {
                        putString("DOB", dob_pretty )
                        putString("Expdate", expdate_pretty)
                        putString("Passnum",passportnum_pretty)
                        apply()
                    }
                }

                mtab_layout.getTabAt(0)?.select()
            }
        })

        cameraOCR = DataOCR(this.requireActivity(), binding.previewView as PreviewView, this.requireActivity(),
            this::onSuccesfullyScan
            )
        binding.cameraBtn.setOnClickListener {
            if (allPermissionsGranted()) {
                cameraOCR.startCamera()
            } else {
                ActivityCompat.requestPermissions(
                    this.requireActivity(),
                    REQUIRED_PERMISSIONS,
                    REQUEST_CODE_PERMISSIONS
                )
            }
        }

        //Load Data from Storage on Startup
        val sharedPref = activity?.getSharedPreferences("Data", Context.MODE_PRIVATE)
        sharedPref?.let {
            this.dob_pretty = sharedPref.getString("DOB", "")!!
            this.expdate_pretty = sharedPref.getString("Expdate", "")!!
            this.passportnum_pretty = sharedPref.getString("Passnum", "")!!
        }

        return view
    }
    private fun onSuccesfullyScan( documentNumber:String, expiryDate:Date?, birthDate:Date?) {
        this.passportnum_pretty = documentNumber
        this.dob_pretty = displayDFFormatter.format((birthDate))
        this.expdate_pretty = displayDFFormatter.format(expiryDate)
    }

    fun setTabLayout(tabLayout: TabLayout){
        //From Dependency injection
        mtab_layout = tabLayout
    }

    var passportnum: () -> String = { "" }
        get() = { binding.textinputlayoutPassnum.editText!!.text.toString().trim()  }
    var expdate: () -> String = { "" }
        get() = {
            if (binding.textinputlayoutExpdate.editText!!.text.toString().trim().isEmpty()){
                ""
            }
            else{
                processDFFormatter.format(displayDFFormatter.parse(binding.textinputlayoutExpdate.editText!!.text.toString().trim()))
            }
        }
    var dob: () -> String = { "" }
        get() = {
            if (binding.textinputlayoutDob.editText!!.text.toString().trim().isEmpty()){
                ""
            }
            else{
                processDFFormatter.format(displayDFFormatter.parse(binding.textinputlayoutDob.editText!!.text.toString().trim()))
            }
        }

    private var passportnum_pretty:String
        get() { return binding.textinputlayoutPassnum.editText!!.text.toString().trim() }
        set(value) {
            binding.textinputlayoutPassnum.editText?.setText(value)
        }
    private var expdate_pretty: String
        get() {
            if (binding.textinputlayoutExpdate.editText!!.text.toString().trim().isEmpty()){
                return ""
            }
            else{
                return binding.textinputlayoutExpdate.editText!!.text.toString().trim()
            }
        }
        set(value) {
            binding.textinputlayoutExpdate.editText?.setText(value) //Does not check format, meant to be called for loading from file only
        }

    private var dob_pretty: String
        get() {
            if (binding.textinputlayoutDob.editText!!.text.toString().trim().isEmpty()){
                return ""
            }
            else{
                return binding.textinputlayoutDob.editText!!.text.toString().trim()
            }
        }
        set(value) {
            binding.textinputlayoutDob.editText?.setText(value) //Does not check format, meant to be called for loading from file only
        }


    fun isDataValid(isSetError:Boolean = true) : Boolean{
        var isValid = true
        if (binding.textinputlayoutPassnum.editText != null){
            if (binding.textinputlayoutPassnum.editText!!.text.toString().isEmpty()){
                //Request Input Passport Number
                isValid = false
                if (isSetError)
                    binding.textinputlayoutPassnum.error = ("Please type in your document number")
            }
            if(binding.textinputlayoutExpdate.editText!!.text.toString().isEmpty()){
                isValid = false
                if (isSetError)
                    binding.textinputlayoutExpdate.error = ("Please select your document expiration date")
            }
            if(binding.textinputlayoutDob.editText!!.text.toString().isEmpty()){
                isValid = false
                if (isSetError)
                    binding.textinputlayoutDob.error = ("Please select your date of birth")
            }
        }
        else{
            isValid = false
        }

        return isValid
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        //Once initialize every other fragment. Check whether we have required data input already, if no then go to it
        //Skipped because user will have the chance to downgrade before doing anything
        //        if (!isDataValid(true)){
        //            this.mtab_layout.getTabAt(1)?.select()
        //        }
    }


    private fun allPermissionsGranted() =
        REQUIRED_PERMISSIONS.all {
            ContextCompat.checkSelfPermission(this.requireContext(), it) == PackageManager.PERMISSION_GRANTED
    }
    companion object {
        private const val REQUEST_CODE_PERMISSIONS = 10
        private val REQUIRED_PERMISSIONS = arrayOf(
            android.Manifest.permission.CAMERA
        )

        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param param1 Parameter 1.
         * @param param2 Parameter 2.
         * @return A new instance of fragment DataFragment.
         */
        // TODO: Rename and change types and number of parameters
        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            DataFragment().apply {
                arguments = Bundle().apply {
                    putString(ARG_PARAM1, param1)
                    putString(ARG_PARAM2, param2)
                }
            }
    }
}