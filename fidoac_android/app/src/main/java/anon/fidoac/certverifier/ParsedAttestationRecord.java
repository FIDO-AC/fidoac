package anon.fidoac.certverifier;

import static anon.fidoac.certverifier.Constants.ATTESTATION_CHALLENGE_INDEX;
import static anon.fidoac.certverifier.Constants.ATTESTATION_SECURITY_LEVEL_INDEX;
import static anon.fidoac.certverifier.Constants.ATTESTATION_VERSION_INDEX;
import static anon.fidoac.certverifier.Constants.KEYMASTER_SECURITY_LEVEL_INDEX;
import static anon.fidoac.certverifier.Constants.KEYMASTER_VERSION_INDEX;
import static anon.fidoac.certverifier.Constants.KEY_DESCRIPTION_OID;
import static anon.fidoac.certverifier.Constants.KM_SECURITY_LEVEL_SOFTWARE;
import static anon.fidoac.certverifier.Constants.KM_SECURITY_LEVEL_STRONG_BOX;
import static anon.fidoac.certverifier.Constants.KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT;
import static anon.fidoac.certverifier.Constants.SW_ENFORCED_INDEX;
import static anon.fidoac.certverifier.Constants.TEE_ENFORCED_INDEX;
import static anon.fidoac.certverifier.Constants.UNIQUE_ID_INDEX;

import android.os.Build;

import org.bouncycastle.asn1.ASN1InputStream;
import org.bouncycastle.asn1.ASN1OctetString;
import org.bouncycastle.asn1.ASN1Sequence;

import java.io.IOException;
import java.security.cert.X509Certificate;

public class ParsedAttestationRecord {

    public final int attestationVersion;
    public final SecurityLevel attestationSecurityLevel;
    public final int keymasterVersion;
    public final SecurityLevel keymasterSecurityLevel;
    public final byte[] attestationChallenge;
    public final byte[] uniqueId;
    public final AuthorizationList softwareEnforced;
    public final AuthorizationList teeEnforced;

    private ParsedAttestationRecord(ASN1Sequence extensionData) {
        this.attestationVersion =
                ASN1Parsing.getIntegerFromAsn1(extensionData.getObjectAt(ATTESTATION_VERSION_INDEX));
        this.attestationSecurityLevel =
                securityLevelToEnum(
                        ASN1Parsing.getIntegerFromAsn1(
                                extensionData.getObjectAt(ATTESTATION_SECURITY_LEVEL_INDEX)));
        this.keymasterVersion =
                ASN1Parsing.getIntegerFromAsn1(extensionData.getObjectAt(KEYMASTER_VERSION_INDEX));
        this.keymasterSecurityLevel =
                securityLevelToEnum(
                        ASN1Parsing.getIntegerFromAsn1(
                                extensionData.getObjectAt(KEYMASTER_SECURITY_LEVEL_INDEX)));
        this.attestationChallenge =
                ((ASN1OctetString) extensionData.getObjectAt(ATTESTATION_CHALLENGE_INDEX)).getOctets();
        this.uniqueId = ((ASN1OctetString) extensionData.getObjectAt(UNIQUE_ID_INDEX)).getOctets();
        this.softwareEnforced =
                AuthorizationList.createAuthorizationList(
                        ((ASN1Sequence) extensionData.getObjectAt(SW_ENFORCED_INDEX)).toArray(),
                        attestationVersion);
        this.teeEnforced =
                AuthorizationList.createAuthorizationList(
                        ((ASN1Sequence) extensionData.getObjectAt(TEE_ENFORCED_INDEX)).toArray(),
                        attestationVersion);
    }

    public static ParsedAttestationRecord createParsedAttestationRecord(X509Certificate cert)
            throws IOException {
        ASN1Sequence extensionData = extractAttestationSequence(cert);
        return new ParsedAttestationRecord(extensionData);
    }

    private static SecurityLevel securityLevelToEnum(int securityLevel) {
        switch (securityLevel) {
            case KM_SECURITY_LEVEL_SOFTWARE:
                return SecurityLevel.SOFTWARE;
            case KM_SECURITY_LEVEL_TRUSTED_ENVIRONMENT:
                return SecurityLevel.TRUSTED_ENVIRONMENT;
            case KM_SECURITY_LEVEL_STRONG_BOX:
                return SecurityLevel.STRONG_BOX;
            default:
                throw new IllegalArgumentException("Invalid security level.");
        }
    }

    private static ASN1Sequence extractAttestationSequence(X509Certificate attestationCert)
            throws IOException {
        byte[] attestationExtensionBytes = attestationCert.getExtensionValue(KEY_DESCRIPTION_OID);
        if (attestationExtensionBytes == null || attestationExtensionBytes.length == 0) {
            throw new IllegalArgumentException("Couldn't find the keystore attestation extension data.");
        }

        ASN1Sequence decodedSequence;
        try (ASN1InputStream asn1InputStream = new ASN1InputStream(attestationExtensionBytes)) {
            // The extension contains one object, a sequence, in the
            // Distinguished Encoding Rules (DER)-encoded form. Get the DER
            // bytes.
            byte[] derSequenceBytes = ((ASN1OctetString) asn1InputStream.readObject()).getOctets();
            // Decode the bytes as an ASN1 sequence object.
            try (ASN1InputStream seqInputStream = new ASN1InputStream(derSequenceBytes)) {
                decodedSequence = (ASN1Sequence) seqInputStream.readObject();
            }
        }
        return decodedSequence;
    }

    /**
     * This indicates the extent to which a software feature, such as a key pair, is protected based
     * on its location within the device.
     */
    public enum SecurityLevel {
        SOFTWARE,
        TRUSTED_ENVIRONMENT,
        STRONG_BOX
    }
}
