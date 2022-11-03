package anon.fidoac

import android.util.Log
import anon.fidoac.certverifier.AndroidKeyAttestationVerifier
import java.io.ByteArrayInputStream
import java.io.InputStream
import java.security.Signature
import java.security.cert.CertificateFactory
import java.security.cert.X509Certificate

//Anonymous Credential Verifier - Should be implemented/executed at serverside, provided as reference only.

class ExampleVerifier {
    companion object{
        val TAG = "ACVerifier"

        //1- Verify_Mediator(H(clientHash||server)) + Verify Attestation => Proof of Interaction from Mediator.
        //2- Verify ZKP (Attribute) => Proof of Satistifability.
        //1+2 => Proof of Posession of a Valid eID that satisfy certain predicate.
        fun verify(clientRanomziedHash:ByteArray,serverChallenge:ByteArray, mediatorSignature:ByteArray, mediatorCertChains:ArrayList<ByteArray>,
                   zkproof:ByteArray, ageLimit:Int, verificationKey:ByteArray){
            val message=  clientRanomziedHash + serverChallenge

            if (verifyAttestation(message,mediatorSignature,mediatorCertChains)){
                Log.d(TAG,"Attestation Verified OK")
            }
            else{
                Log.e(TAG,"Attestation Verified Failed!!!!")
            }

            if (verifyZKProof(clientRanomziedHash,zkproof,ageLimit,verificationKey)){
                Log.d(TAG,"ZK OK")
            }
            else{
                Log.e(TAG,"ZK FAILED!!")
            }
        }

        fun verifyAttestation(signedMessage:ByteArray,mediatorSignature:ByteArray, mediatorCertChains:ArrayList<ByteArray>): Boolean{
            val certs:ArrayList<X509Certificate> = ArrayList<X509Certificate>()

            for (certByteArray in mediatorCertChains){
                val cf: CertificateFactory = CertificateFactory.getInstance("X.509");
                val input: InputStream = ByteArrayInputStream(certByteArray)
                val certificate = cf.generateCertificate(input) as X509Certificate
                certs.add(certificate)
                //Log.d(TAG,certificate.toString())
            }

            var signatureObj: Signature = Signature.getInstance(certs[0].sigAlgOID)
            signatureObj.initVerify(certs[0].publicKey)
            signatureObj.update(signedMessage)

            return AndroidKeyAttestationVerifier.verify(certs.toTypedArray()) && signatureObj.verify(mediatorSignature)
        }

        fun verifyZKProof(clientRanomziedHash:ByteArray, zkproof:ByteArray, ageLimit: Int,verificationKey: ByteArray):Boolean{
            return MainActivity.FIDO_AC_veirfy(zkproof,clientRanomziedHash, ageLimit, verificationKey)
        }
    }
}