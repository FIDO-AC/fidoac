package anon.fidoac

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.core.content.res.ResourcesCompat
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentManager
import androidx.lifecycle.Lifecycle
import androidx.viewpager2.adapter.FragmentStateAdapter
import androidx.viewpager2.widget.ViewPager2
import anon.fidoac.databinding.ActivityMainBinding
import anon.fidoac.databinding.FragmentHomeBinding
import com.google.android.material.tabs.TabLayout
import com.google.android.material.tabs.TabLayout.OnTabSelectedListener
import com.google.android.material.tabs.TabLayoutMediator
import java.io.BufferedReader
import java.io.InputStreamReader
import java.lang.StringBuilder


// TODO: Rename parameter arguments, choose names that match
// the fragment initialization parameters, e.g. ARG_ITEM_NUMBER
private const val ARG_PARAM1 = "param1"
private const val ARG_PARAM2 = "param2"

/**
 * Manager. Have access to all handle.
 * Facilitate Fragment communication
 */
class HomeFragment : Fragment() {
//    ViewModel allow data to be carried across activity lifecycle until activity is destroyed.
//    private lateinit var homeViewModel: HomeFragmentViewModel
//    private var myContext: FragmentActivity? = null

    private var param1: String? = null
    private var param2: String? = null

    private val TAG = "HomeFragment"

    private lateinit var tabs:TabLayout
    private lateinit var mydf:DataFragment
    private var _binding: FragmentHomeBinding? = null
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
        _binding= FragmentHomeBinding.inflate(inflater, container, false)
        val viewpagger: ViewPager2 = binding.homefragPager
        viewpagger.offscreenPageLimit = 1
        viewpagger.isSaveEnabled=false
        tabs = binding.homefragTabLayout
        tabs.addOnTabSelectedListener(object : OnTabSelectedListener {
            override fun onTabSelected(tab: TabLayout.Tab) {}
            override fun onTabUnselected(tab: TabLayout.Tab) {
                mydf.closeCamera()
            }
            override fun onTabReselected(tab: TabLayout.Tab) {}
        })
        val adapter =MyViewPagerAdapter(childFragmentManager,lifecycle)
        mydf =  DataFragment()
        mydf.setTabLayout(tabs)

        val myscan = ScanFragment()
        myscan.passportnumgetter = {
            mydf.passportnum()
        }
        myscan.dobgetter = {
            mydf.dob()
        }
        myscan.expdategetter = {
            mydf.expdate()
        }
        myscan.isDataValid={
            mydf.isDataValid()
        }
        myscan.mtab_layout = tabs
        adapter.addFragment(myscan,getString(R.string.Scan_fragment_text))
        adapter.addFragment(mydf, getString(R.string.Data_fragment_text))
        adapter.addFragment(TutorialFragment(), getString(R.string.Tutorial_fragment_text))
        adapter.notifyDataSetChanged()
        viewpagger.adapter =adapter

        val tabTitles = arrayOf("Scan", "Data", "Tutorial") //put titles based on your need
        val tabIcons = intArrayOf(R.drawable.ic_baseline_document_scanner_24, R.drawable.ic_baseline_edit_note_24, R.drawable.ic_baseline_help_24)

        TabLayoutMediator(tabs, viewpagger) { tab, position ->
            tab.text = tabTitles[position]
            tab.icon =  ResourcesCompat.getDrawable(resources, tabIcons[position], null);
            viewpagger.setCurrentItem(tab.position, true)
        }.attach()


        Log.d(TAG,"HomeFragment Created View")

        return binding.root
    }

    class MyViewPagerAdapter(manager: FragmentManager, lifecycle: Lifecycle) :  FragmentStateAdapter(manager,lifecycle ){
        private val fragmentList : MutableList<Fragment> =ArrayList()
        private val titleList : MutableList<String> =ArrayList()
        override fun getItemCount(): Int {
            return fragmentList.size
        }

        override fun createFragment(position: Int): Fragment {
            return  fragmentList[position]
        }

        fun addFragment(fragment: Fragment, title: String){
            fragmentList.add(fragment)
            titleList.add(title)
        }

        fun getPageTitle(position: Int): CharSequence? {
            return titleList[position]
        }
    }

    companion object {
        /**
         * Use this factory method to create a new instance of
         * this fragment using the provided parameters.
         *
         * @param param1 Parameter 1.
         * @param param2 Parameter 2.
         * @return A new instance of fragment HomeFragment.
         */
        // TODO: Rename and change types and number of parameters
        @JvmStatic
        fun newInstance(param1: String, param2: String) =
            HomeFragment().apply {
                arguments = Bundle().apply {
                    putString(ARG_PARAM1, param1)
                    putString(ARG_PARAM2, param2)
                }
            }
    }
}