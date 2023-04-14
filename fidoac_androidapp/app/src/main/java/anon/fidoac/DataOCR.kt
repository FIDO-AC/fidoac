package anon.fidoac

import android.annotation.SuppressLint
import android.content.Context
import android.util.Log
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import androidx.lifecycle.LifecycleOwner
import com.google.android.gms.tasks.TaskExecutors
import com.google.mlkit.vision.common.InputImage
import com.google.mlkit.vision.text.TextRecognition
import com.google.mlkit.vision.text.latin.TextRecognizerOptions
import fr.coppernic.lib.mrz.MrzParser
import fr.coppernic.lib.mrz.parser.MrzParserOptions
import java.util.*
import java.util.concurrent.Executor
import java.util.concurrent.atomic.AtomicBoolean

class DataOCR(
    private val context: Context,
    private val finderView: PreviewView,
    private val lifecycleOwner: LifecycleOwner,
    private val callBack: ((String, Date?, Date?) -> Unit)
){
    private val cameraExecutor = ScopedExecutor(TaskExecutors.MAIN_THREAD)
    private var isOpened = false
    private lateinit var camera: Camera
    private lateinit var imageAnalyzer: ImageAnalysis
    private lateinit var cameraProvider: ProcessCameraProvider
    private lateinit var preview: Preview
    private val TAG = "DataOCR"

    class ScopedExecutor(private val executor: Executor) : Executor {
        private val shutdown = AtomicBoolean()
        override fun execute(command: Runnable) {
            // Return early if this object has been shut down.
            if (shutdown.get()) {
                return
            }
            executor.execute {
                // Check again in case it has been shut down in the mean time.
                if (shutdown.get()) {
                    return@execute
                }
                command.run()
            }
        }
        /**
         * After this method is called, no runnables that have been submitted or are subsequently
         * submitted will start to execute, turning this executor into a no-op.
         * Runnables that have already started to execute will continue.
         */
        fun shutdown() {
            shutdown.set(true)
        }
    }

    fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(context)
        cameraProviderFuture.addListener(
            Runnable {
                cameraProvider = cameraProviderFuture.get()
                preview = Preview.Builder().build()

                // set Analyzer
                imageAnalyzer = ImageAnalysis.Builder()
                    .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                    .build()
                    .also {
                        it.setAnalyzer(cameraExecutor, dataOCRImageAnalyzer)
                    }

                val cameraSelector = CameraSelector.Builder()
                    .requireLensFacing(CameraSelector.LENS_FACING_BACK)
                    .build()

                // zoom
                //setUpPinchToZoom()
                setCameraConfig(cameraProvider, cameraSelector)
                isOpened = true

            }, ContextCompat.getMainExecutor(context)
        )
    }
    private fun setCameraConfig(
        cameraProvider: ProcessCameraProvider?,
        cameraSelector: CameraSelector
    ) {
        try {
            cameraProvider?.unbindAll()
            camera = cameraProvider?.bindToLifecycle(
                lifecycleOwner,
                cameraSelector,
                preview,
                imageAnalyzer
            )!!
            preview?.setSurfaceProvider(finderView.surfaceProvider)
        } catch (e: Exception) {
            Log.e(TAG, "Use case binding failed", e)
        }
    }
    fun stopCamera() {
        if (isOpened){
            isOpened = false
            cameraProvider.unbindAll()
        }
    }

    private val recognizer = TextRecognition.getClient(TextRecognizerOptions.DEFAULT_OPTIONS)
    private val parser = MrzParser()
    private val options = MrzParserOptions(
        lenient = false
    )
    @SuppressLint("UnsafeOptInUsageError")
    private val dataOCRImageAnalyzer = ImageAnalysis.Analyzer{
            val imageProxy = it
            val mediaImage = imageProxy.image

            if (mediaImage != null) {
                val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)
                val result = recognizer.process(image)
                    .addOnSuccessListener { visionText ->
                        var processed_str_2line = ""
                        var processed_str_3line = ""
                        var counter = 0
                        for( str in visionText.text.split("\n").reversed()){
//                            if (!processed_str.contains("<")){
//                                break
//                            }
                            if (counter<2)
                                processed_str_2line = str + "\n" +processed_str_2line

                            processed_str_3line = str + "\n" +processed_str_3line
                            counter+=1
                            if (counter == 3)
                                break
                        }

                        try{
                            val mrz = parser.parse(processed_str_3line, options)
                            Log.d(TAG,processed_str_3line)
                            Log.d(TAG,mrz.toString())
                            stopCamera()
                            callBack?.invoke(mrz.documentNumber,mrz.expiryDate,mrz.birthdate)
                        }
                        catch(e: Exception){
                            try{
                                val mrz = parser.parse(processed_str_2line, options)
                                Log.d(TAG,processed_str_2line)
                                Log.d(TAG,mrz.toString())
                                callBack?.invoke(mrz.documentNumber,mrz.expiryDate,mrz.birthdate)
                            }
                            catch(e:Exception){
                                Log.d(TAG,"Try again, cannot parse MRZ")
                                Log.d(TAG,processed_str_3line)
                                imageProxy.close()
                            }
                        }
                    }
                    .addOnFailureListener { e ->
                        Log.e(TAG,e.message.toString())
                        Log.e(TAG,e.stackTraceToString())
                        imageProxy.close()
                    }
            }
        }
}
