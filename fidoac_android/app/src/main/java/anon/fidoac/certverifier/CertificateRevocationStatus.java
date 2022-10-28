package anon.fidoac.certverifier;

import com.google.gson.Gson;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

import java.io.FileReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.Reader;
import java.math.BigInteger;
import java.net.MalformedURLException;
import java.net.URL;

/**
 * Utils for fetching and decoding attestation certificate status.
 */
public class CertificateRevocationStatus {

    private static final String STATUS_URL = "https://android.googleapis.com/attestation/status";
    public final Status status;
    public final Reason reason;
    public final String comment;
    public final String expires;

    public static CertificateRevocationStatus loadStatusFromFile(BigInteger serialNumber,
                                                                 String filePath)
            throws IOException {
        return loadStatusFromFile(serialNumber.toString(16), filePath);
    }

    public static CertificateRevocationStatus loadStatusFromFile(String serialNumber, String filePath)
            throws IOException {
        FileReader reader = new FileReader(filePath);
        return decodeStatus(serialNumber, reader);
    }


    public static CertificateRevocationStatus fetchStatus(BigInteger serialNumber)
            throws IOException {
        return fetchStatus(serialNumber.toString(16));
    }

    public static CertificateRevocationStatus fetchStatus(String serialNumber) throws IOException {
        URL url;
        try {
            url = new URL(STATUS_URL);
        } catch (MalformedURLException e) {
            throw new IllegalStateException(e);
        }

        InputStreamReader statusListReader = new InputStreamReader(url.openStream());

        return decodeStatus(serialNumber, statusListReader);

    }

    private static CertificateRevocationStatus decodeStatus(String serialNumber,
                                                            Reader statusListReader) {
        if (serialNumber == null) {
            throw new IllegalArgumentException("serialNumber cannot be null");
        }
        serialNumber = serialNumber.toLowerCase();

        JsonObject entries = new JsonParser().parse(statusListReader)
                .getAsJsonObject()
                .getAsJsonObject("entries");

        if (!entries.has(serialNumber)) {
            return null;
        }

        return new Gson().fromJson(entries.get(serialNumber), CertificateRevocationStatus.class);
    }

    public enum Status {
        REVOKED, SUSPENDED
    }

    public enum Reason {
        UNSPECIFIED, KEY_COMPROMISE, CA_COMPROMISE, SUPERSEDED, SOFTWARE_FLAW
    }

    public CertificateRevocationStatus() {
        status = Status.REVOKED;
        reason = Reason.UNSPECIFIED;
        comment = null;
        expires = null;
    }
}
