package anon.fidoac

import android.util.Log
import java.security.KeyStore
import java.security.cert.X509Certificate

class KeyManagement {
    private val KEYSTORE_ALIAS:String = "FIDO-AC"
    private val TAG = "KeyManagement"

    fun signResult(){

    }

    fun getCert(){
        val cert = KeyStore.getInstance("AndroidKeyStore").getCertificateChain(KEYSTORE_ALIAS)
        val cert_x509 = arrayOfNulls<X509Certificate>(cert.size)
        for (i in cert.indices) {
            val ct = cert[i]
            Log.i(TAG, "Cert: " + ct.type)
            Log.i(TAG, "Public: " + (ct as X509Certificate).toString())
            cert_x509[i] = ct
        }
        Log.i(TAG, "Public: $cert_x509")
    }

    fun genKey(){
        //                        var newSpec:KeyGenParameterSpec? = null
        //                        if (agreementAlg == "EC"){
        //                            newSpec = params?.let {
        //                                android.security.keystore.KeyGenParameterSpec.Builder(
        //                                    KEYSTORE_ALIAS,
        //                                    android.security.keystore.KeyProperties.PURPOSE_AGREE_KEY)
        //                                    .setAlgorithmParameterSpec(it as ECGenParameterSpec)
        //                                    .setDigests(android.security.keystore.KeyProperties.DIGEST_SHA256)
        //                                    .setAttestationChallenge(this.stateBasket.relyingparty_challenge)
        //                                    .build()
        //                            }
        //                        }else{
        //                            newSpec = params?.let {
        //                                android.security.keystore.KeyGenParameterSpec.Builder(
        //                                    KEYSTORE_ALIAS,
        //                                    android.security.keystore.KeyProperties.PURPOSE_AGREE_KEY)
        //                                    .setAlgorithmParameterSpec(it as DHGenParameterSpec)
        //                                    .setDigests(android.security.keystore.KeyProperties.DIGEST_SHA256)
        //                                    .setAttestationChallenge(this.stateBasket.relyingparty_challenge)
        //                                    .build()
        //                            }
        //                        }
        //                        keyPairGenerator.initialize(newSpec)
    }
}