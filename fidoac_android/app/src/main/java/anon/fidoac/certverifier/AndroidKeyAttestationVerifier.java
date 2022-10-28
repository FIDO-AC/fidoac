package anon.fidoac.certverifier;

import static java.nio.charset.StandardCharsets.UTF_8;

import static anon.fidoac.certverifier.Constants.GOOGLE_ROOT_CERTIFICATE;
import static anon.fidoac.certverifier.ParsedAttestationRecord.createParsedAttestationRecord;

import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import org.bouncycastle.util.encoders.Base64;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.SignatureException;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;
import java.util.stream.Stream;

public class AndroidKeyAttestationVerifier {

    private AndroidKeyAttestationVerifier() {}

    public static boolean verify(X509Certificate[] certs){
        try {
            return verify(certs, false);
        } catch (CertificateException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (NoSuchProviderException e) {
            e.printStackTrace();
        } catch (NoSuchAlgorithmException e) {
            e.printStackTrace();
        } catch (InvalidKeyException e) {
            e.printStackTrace();
        } catch (SignatureException e) {
            e.printStackTrace();
        }
        return false;
    }

    public static boolean verify(X509Certificate[] certs, boolean isChallengeOnly)
            throws CertificateException, IOException, NoSuchProviderException, NoSuchAlgorithmException,
            InvalidKeyException, SignatureException {
        //        X509Certificate[] certs;
        //        if (args.length == 1) {
        //            String certFilesDir = args[0];
        //            certs = loadCertificates(certFilesDir);
        //        } else {
        //            throw new IOException("Expected path to a directory containing certificates as an argument.");
        //        }

        //        verifyCertificateChain(certs);

        ParsedAttestationRecord parsedAttestationRecord = createParsedAttestationRecord(certs[0]);
        if (isChallengeOnly){
            Log.i("WZAct",
                    "Attestation Challenge: "
                            + new String(parsedAttestationRecord.attestationChallenge, UTF_8));
            return false;
        }

        Log.i("WZAct", "Attestation version: " + parsedAttestationRecord.attestationVersion);
        Log.i("WZAct",
                "Attestation Security Level: " + parsedAttestationRecord.attestationSecurityLevel.name());
        Log.i("WZAct","Keymaster Version: " + parsedAttestationRecord.keymasterVersion);
        Log.i("WZAct",
                "Keymaster Security Level: " + parsedAttestationRecord.keymasterSecurityLevel.name());

        Log.i("WZAct",
                "Attestation Challenge: "
                        + new String(parsedAttestationRecord.attestationChallenge, UTF_8));
        Log.i("WZAct","Unique ID: " + Arrays.toString(parsedAttestationRecord.uniqueId));

        Log.i("WZAct","Software Enforced Authorization List:");
        AuthorizationList softwareEnforced = parsedAttestationRecord.softwareEnforced;
        printAuthorizationList(softwareEnforced, "\t");

        Log.i("WZAct","TEE Enforced Authorization List:");
        AuthorizationList teeEnforced = parsedAttestationRecord.teeEnforced;
        printAuthorizationList(teeEnforced, "\t");

        return verifyCertificateChain(certs);
    }

    private static void printAuthorizationList(AuthorizationList authorizationList, String indent) {
        // Detailed explanation of the keys and their values can be found here:
        // https://source.android.com/security/keystore/tags
        printOptional(authorizationList.purpose, indent + "Purpose(s)");
        printOptional(authorizationList.algorithm, indent + "Algorithm");
        printOptional(authorizationList.keySize, indent + "Key Size");
        printOptional(authorizationList.digest, indent + "Digest");
        printOptional(authorizationList.padding, indent + "Padding");
        printOptional(authorizationList.ecCurve, indent + "EC Curve");
        printOptional(authorizationList.rsaPublicExponent, indent + "RSA Public Exponent");
        Log.i("WZAct",indent + "Rollback Resistance: " + authorizationList.rollbackResistance);
        printOptional(authorizationList.activeDateTime, indent + "Active DateTime");
        printOptional(
                authorizationList.originationExpireDateTime, indent + "Origination Expire DateTime");
        printOptional(authorizationList.usageExpireDateTime, indent + "Usage Expire DateTime");
        Log.i("WZAct",indent + "No Auth Required: " + authorizationList.noAuthRequired);
        printOptional(authorizationList.userAuthType, indent + "User Auth Type");
        printOptional(authorizationList.authTimeout, indent + "Auth Timeout");
        Log.i("WZAct",indent + "Allow While On Body: " + authorizationList.allowWhileOnBody);
        Log.i("WZAct",
                indent
                        + "Trusted User Presence Required: "
                        + authorizationList.trustedUserPresenceRequired);
        Log.i("WZAct",
                indent + "Trusted Confirmation Required: " + authorizationList.trustedConfirmationRequired);
        Log.i("WZAct",
                indent + "Unlocked Device Required: " + authorizationList.unlockedDeviceRequired);
        Log.i("WZAct",indent + "All Applications: " + authorizationList.allApplications);
        printOptional(authorizationList.applicationId, indent + "Application ID");
        printOptional(authorizationList.creationDateTime, indent + "Creation DateTime");
        printOptional(authorizationList.origin, indent + "Origin");
        Log.i("WZAct",indent + "Rollback Resistant: " + authorizationList.rollbackResistant);
        if (authorizationList.rootOfTrust.isPresent()) {
            Log.i("WZAct",indent + "Root Of Trust:");
            printRootOfTrust(authorizationList.rootOfTrust, indent + "\t");
        }
        printOptional(authorizationList.osVersion, indent + "OS Version");
        printOptional(authorizationList.osPatchLevel, indent + "OS Patch Level");
        if (authorizationList.attestationApplicationId.isPresent()) {
            Log.i("WZAct",indent + "Attestation Application ID:");
            printAttestationApplicationId(authorizationList.attestationApplicationId, indent + "\t");
        }
        printOptional(
                authorizationList.attestationApplicationIdBytes,
                indent + "Attestation Application ID Bytes");
        printOptional(authorizationList.attestationIdBrand, indent + "Attestation ID Brand");
        printOptional(authorizationList.attestationIdDevice, indent + "Attestation ID Device");
        printOptional(authorizationList.attestationIdProduct, indent + "Attestation ID Product");
        printOptional(authorizationList.attestationIdSerial, indent + "Attestation ID Serial");
        printOptional(authorizationList.attestationIdImei, indent + "Attestation ID IMEI");
        printOptional(authorizationList.attestationIdMeid, indent + "Attestation ID MEID");
        printOptional(
                authorizationList.attestationIdManufacturer, indent + "Attestation ID Manufacturer");
        printOptional(authorizationList.attestationIdModel, indent + "Attestation ID Model");
        printOptional(authorizationList.vendorPatchLevel, indent + "Vendor Patch Level");
        printOptional(authorizationList.bootPatchLevel, indent + "Boot Patch Level");

    }

    private static void printRootOfTrust(Optional<RootOfTrust> rootOfTrust, String indent) {
        if (rootOfTrust.isPresent()) {
            Log.i("WZAct",
                    indent
                            + "Verified Boot Key: "
                            + Base64.toBase64String(rootOfTrust.get().verifiedBootKey));
            Log.i("WZAct",indent + "Device Locked: " + rootOfTrust.get().deviceLocked);
            Log.i("WZAct",
                    indent + "Verified Boot State: " + rootOfTrust.get().verifiedBootState.name());
            Log.i("WZAct",
                    indent
                            + "Verified Boot Hash: "
                            + Base64.toBase64String(rootOfTrust.get().verifiedBootHash));
        }
    }

    private static void printAttestationApplicationId(
            Optional<AttestationApplicationId> attestationApplicationId, String indent) {
        if (attestationApplicationId.isPresent()) {
            Log.i("WZAct",indent + "Package Infos (<package name>, <version>): ");
            for (AttestationApplicationId.AttestationPackageInfo info : attestationApplicationId.get().packageInfos) {
                Log.i("WZAct",indent + "\t" + info.packageName + ", " + info.version);
            }
            Log.i("WZAct",indent + "Signature Digests:");
            for (byte[] digest : attestationApplicationId.get().signatureDigests) {
                Log.i("WZAct",indent + "\t" + Base64.toBase64String(digest));
            }
        }
    }

    private static <T> void printOptional(Optional<T> optional, String caption) {
        if (optional.isPresent()) {
            if (optional.get() instanceof byte[]) {
                Log.i("WZAct",caption + ": " + Base64.toBase64String((byte[]) optional.get()));
            } else {
                Log.i("WZAct",caption + ": " + optional.get());
            }
        }
    }

    private static boolean verifyCertificateChain(X509Certificate[] certs)
            throws CertificateException, NoSuchAlgorithmException, InvalidKeyException,
            NoSuchProviderException, SignatureException, IOException {
        X509Certificate parent = certs[certs.length - 1];
        for (int i = certs.length - 1; i >= 0; i--) {
            X509Certificate cert = certs[i];
            // Verify that the certificate has not expired.
            cert.checkValidity();
            cert.verify(parent.getPublicKey());
            parent = cert;
            try {
                CertificateRevocationStatus certStatus = CertificateRevocationStatus
                        .fetchStatus(cert.getSerialNumber());
                if (certStatus != null) {
                    throw new CertificateException(
                            "Certificate revocation status is " + certStatus.status.name());
                }
            } catch (IOException e) {
                throw new IOException("Unable to fetch certificate status. Check connectivity.");
            }
        }

        // If the attestation is trustworthy and the device ships with hardware-
        // level key attestation, Android 7.0 (API level 24) or higher, and
        // Google Play services, the root certificate should be signed with the
        // Google attestation root key.
        X509Certificate secureRoot =
                (X509Certificate)
                        CertificateFactory.getInstance("X.509")
                                .generateCertificate(
                                        new ByteArrayInputStream(GOOGLE_ROOT_CERTIFICATE.getBytes(UTF_8)));
        if (Arrays.equals(
                secureRoot.getPublicKey().getEncoded(),
                certs[certs.length - 1].getPublicKey().getEncoded())) {
            Log.i("WZAct",
                    "The root certificate is correct, so this attestation is trustworthy, as long as none of"
                            + " the certificates in the chain have been revoked. A production-level system"
                            + " should check the certificate revocation lists using the distribution points that"
                            + " are listed in the intermediate and root certificates.");
            return true;
        } else {
            Log.i("WZAct",
                    "The root certificate is NOT correct. The attestation was probably generated by"
                            + " software, not in secure hardware. This means that, although the attestation"
                            + " contents are probably valid and correct, there is no proof that they are in fact"
                            + " correct. If you're using a production-level system, you should now treat the"
                            + " properties of this attestation certificate as advisory only, and you shouldn't"
                            + " rely on this attestation certificate to provide security guarantees.");
            return false;
        }
    }

    private static X509Certificate[] loadCertificates(String certFilesDir)
            throws CertificateException, IOException {
        // Load the attestation certificates from the directory in alphabetic order.
        List<Path> records;
        try (Stream<Path> pathStream = Files.walk(Paths.get(certFilesDir))) {
            records = pathStream.filter(Files::isRegularFile).sorted().collect(Collectors.toList());
        }
        X509Certificate[] certs = new X509Certificate[records.size()];
        CertificateFactory factory = CertificateFactory.getInstance("X.509");
        for (int i = 0; i < records.size(); ++i) {
            byte[] encodedCert = Files.readAllBytes(records.get(i));
            ByteArrayInputStream inputStream = new ByteArrayInputStream(encodedCert);
            certs[i] = (X509Certificate) factory.generateCertificate(inputStream);
        }
        return certs;
    }
}
