import json
import base64
# import OpenSSL.crypto as crypto
import ssl
import datetime
from cryptography import x509
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.serialization import Encoding
from cryptography.x509.oid import ExtensionOID,ObjectIdentifier
from cryptography import exceptions
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import hashes
from asn1crypto.core import Sequence
import asn1
import pem

sample = '{"PROOF_ID":"ERPrFDrlFsoD1BGi_AU1tlq4Ymt5eRAroMrRCBaIvnadQIOU_c0UQjATP9CHGf-pDXVnuOcyt-00EHDw5W0DDEmsmBL100-L_kjA6io5_3juaYyNGRNWtV1ZtWAsbmYTASnXAOvS5JORSqtP-8j8W8OXEp2mQ1OyO1QoibKZdl5qGd7u6amqpFgeQ2YXL6FEB1UxUeekiIAEU0NjWQV_0Y4hmYsAH678KvTpsRyqMhZktOD9Mtgde1tJlVy1Suv2EmtKfm3jOHTr2HpbMYrOkJ-wSNCoGMhSvPSgmgLqpNCGmNwAVknocA4KYMBTW_zKEQBEi1_AfoSF_NSICySMpWo8pceMUaJUoVVBp1fzk2Hv5mo_-DAZQp66X2wYrF-SABOKrvpHxwTQoWTWdSu7lZdOsa9hZzstqEFika53VukHDniykDLT2JWb1eB6nLIpAvKMQHMtFSHAYcNliyOK6lr9-jJgqVbOCl6ybKPFa_DZvt1yH4t8fdJ8vtxHBDhs","MEDIATOR_CERT_ID_0":"MIICnTCCAkKgAwIBAgIBATAKBggqhkjOPQQDAjA5MQwwCgYDVQQKEwNURUUxKTAnBgNVBAMTIDc1NWUwZDQ5YWIyZmE3MjM5M2U0MjIxZDBkMDM5MzJlMB4XDTcwMDEwMTAwMDAwMFoXDTQ4MDEwMTAwMDAwMFowHzEdMBsGA1UEAxMUQW5kcm9pZCBLZXlzdG9yZSBLZXkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQTco2k-UFAA-apbLbuISHvEKVbbPmBPNUL_J1Htr6ljjDujQmvjCjt2bFkSRhtSFC1yMpZf5j_GW6uQo5Gqnaao4IBUzCCAU8wDgYDVR0PAQH_BAQDAgeAMIIBOwYKKwYBBAHWeQIBEQSCASswggEnAgIAyAoBAQICAMgKAQEEIKOXGGQ21IkvjrSmzdRgnRSv9R8e3x06-o2QGxg0J0NkBAAwTL-FPQgCBgGHb_UD2L-FRTwEOjA4MRIwEAQLYW5vbi5maWRvYWMCAQExIgQgUTELy9jv64kT8-9OZcmzDNntbAEWoI0Te7-2s-LAqE0wgaShCDEGAgECAgEDogMCAQOjBAICAQClBTEDAgEEqgMCAQG_g3cCBQC_hT4DAgEAv4VATDBKBCBC7RvKNS-r1CjzTo_O5id29MssZuBvguWln_RJUme_wgEB_woBAAQgeizj-0Qb44b_mKCLVzKqK8Gs8s329hp0kPj6mXccGne_hUEFAgMB-9C_hUIFAgMDFeS_hU4GAgQBNI0Vv4VPBgIEATSNFTAKBggqhkjOPQQDAgNJADBGAiEA5z7_MJlh2WbANC8_3em98tLnOtUpIG9wkaKOC1TbsgoCIQDh7y8AZJ_PNS8-Nc3jf4725BivYtZ1dpabX5_xO5tPnA==","HASH_ID":"Skr-UX_9CNi6ot9TupaZs37t133bvQkdeE3Qt0312Dw=","MEDIATOR_CERT_ID_2":"MIIB1jCCAV2gAwIBAgIUAKQDvuF51iSj420QvO0qwmh_ebQwCgYIKoZIzj0EAwMwKTETMBEGA1UEChMKR29vZ2xlIExMQzESMBAGA1UEAxMJRHJvaWQgQ0EyMB4XDTIzMDMyODE1MDM0MloXDTIzMDUwMjE1MDM0MVowKTETMBEGA1UEChMKR29vZ2xlIExMQzESMBAGA1UEAxMJRHJvaWQgQ0EzMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEvGl9LTWzBAbcWnJISkBkqArCuRIlZMHKwoZr-3ckECOiZurMZzD8Ftb7mYTeFPdt9n0MarXKSX7nf_4qyTf88aNjMGEwDgYDVR0PAQH_BAQDAgIEMA8GA1UdEwEB_wQFMAMBAf8wHQYDVR0OBBYEFDcMcBkYg2MZx6zbDLXZhSJKIMemMB8GA1UdIwQYMBaAFDmYBwY6MxKe9RQGOoBBDHGAzhqtMAoGCCqGSM49BAMDA2cAMGQCMA4zQC2WeaXT1VtuLf_zKw4dr5tvwLQn-P6FkvGFr8R5vORcoUIu5SlAGoJ1Ng9XTwIwEEiDLGTG6gw7lzzbwn2eJyCr9OFtz8Cw56G9Ebdh4B7FY_rmqRBO8661ExdnZwI0","MEDIATOR_CERT_ID_1":"MIIBwjCCAWmgAwIBAgIQdV4NSasvpyOT5CIdDQOTLjAKBggqhkjOPQQDAjApMRMwEQYDVQQKEwpHb29nbGUgTExDMRIwEAYDVQQDEwlEcm9pZCBDQTMwHhcNMjMwMzI2MTc0NjEwWhcNMjMwNDI3MTc0NjEwWjA5MQwwCgYDVQQKEwNURUUxKTAnBgNVBAMTIDc1NWUwZDQ5YWIyZmE3MjM5M2U0MjIxZDBkMDM5MzJlMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE08By_9SEw0pa6G0lRYeeA_R9iqPMwH1UUkqdceObl8TYHROiWTVImZnCkgOrKuKbHnYdVXO7q7ojKEt1C5jiQaNjMGEwHQYDVR0OBBYEFMCuBTB0Yu6B12d7idWjxNyDH848MB8GA1UdIwQYMBaAFDcMcBkYg2MZx6zbDLXZhSJKIMemMA8GA1UdEwEB_wQFMAMBAf8wDgYDVR0PAQH_BAQDAgIEMAoGCCqGSM49BAMCA0cAMEQCIFDnaxeX3hD8Jb0HEBOqO24R-NUOFk3FWX8aWaQH-emAAiA42xV7mly_1GNiF-0htnrxciZVD1TYvktdG6kSlMltOg==","MEDIATOR_CERT_ID_4":"MIIFHDCCAwSgAwIBAgIJANUP8luj8tazMA0GCSqGSIb3DQEBCwUAMBsxGTAXBgNVBAUTEGY5MjAwOWU4NTNiNmIwNDUwHhcNMTkxMTIyMjAzNzU4WhcNMzQxMTE4MjAzNzU4WjAbMRkwFwYDVQQFExBmOTIwMDllODUzYjZiMDQ1MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAr7bHgiuxpwHsK7Qui8xUFmOr75gvMsd_dTEDDJdSSxtf6An7xyqpRR90PL2abxM1dEqlXnf2tqw1Ne4Xwl5jlRfdnJLmN0pTy_4lj4_7tv0Sk3iiKkypnEUtR6WfMgH0QZfKHM1-di-y9TFRtv6y__0rb-T-W8a9nsNL_ggjnar86461qO0rOs2cXjp3kOG1FEJ5MVmFmBGtnrKpa73XpXyTqRxB_M0n1n_W9nGqC4FSYa04T6N5RIZGBN2z2MT5IKGbFlbC8UrW0DxW7AYImQQcHtGl_m00QLVWutHQoVJYnFPlXTcHYvASLu-RhhsbDmxMgJJ0mcDpvsC4PjvB-TxywElgS70vE0XmLD-OJtvsBslHZvPBKCOdT0MS-tgSOIfga-z1Z1g7-DVagf7quvmag8jfPioyKvxnK_EgsTUVi2ghzq8wm27ud_mIM7AY2qEORR8Go3TVB4HzWQgpZrt3i5MIlCaY504LzSRiigHCzAPlHws-W0rB5N-er5_2pJKnfBSDiCiFAVtCLOZ7gLiMm0jhO2B6tUXHI_-MRPjy02i59lINMRRev56GKtcd9qO_0kUJWdZTdA2XoS82ixPvZtXQpUpuL12ab-9EaDK8Z4RHJYYfCT3Q5vNAXaiWQ-8PTWm2QgBR_bkwSWc-NpUFgNPN9PvQi8WEg5UmAGMCAwEAAaNjMGEwHQYDVR0OBBYEFDZh4QB8iAUJUYtEbEf_GkzJ6k8SMB8GA1UdIwQYMBaAFDZh4QB8iAUJUYtEbEf_GkzJ6k8SMA8GA1UdEwEB_wQFMAMBAf8wDgYDVR0PAQH_BAQDAgIEMA0GCSqGSIb3DQEBCwUAA4ICAQBOMaBc8oumXb2voc7XCWnuXKhBBK3e2KMGz39t7lA3XXRe2ZLLAkLM5y3J7tURkf5a1SutfdOyXAmeE6SRo83Uh6WszodmMkxK5GM4JGrnt4pBisu5igXEydaW7qq2CdC6DOGjG-mEkN8_TA6p3cnoL_sPyz6evdjLlSeJ8rFBH6xWyIZCbrcpYEJzXaUOEaxxXxgYz5_cTiVKN2M1G2okQBUIYSY6bjEL4aUN5cfo7ogP3UvliEo3Eo0YgwuzR2v0KR6C1cZqZJSTnghIC_vAD32KdNQ-c3N-vl2OTsUVMC1GiWkngNx1OO1-kXW-YTnnTUOtOIswUP_Vqd5SYgAImMAfY8U9_iIgkQj6T2W6FsScy94IN9fFhE1UtzmLoBIuUFsVXJMTz-Jucth-IqoWFua9v1R93_k98p41pjtFX-H8DslVgfP097vju4KDlqN64xV1grw3ZLl4CiOe_A91oeLm2UHOq6wn3esB4r2EIQKb6jTVGu5sYCcdWpXr0AUVqcABPdgL-H7qJguBw09ojm6xNIrw2OocrDKsudk_okr_AwqEyPKw9WnMlQgLIKw1rODG2NvU9oR3GVGdMkUBZutL8VuFkERQGt6vQ2OCw0sV47VMkuYbacK_xyZFiRcrPJPb41zgbQj9XAEyLKCHex0SdDrx-tWUDqG8At2JHA==","MEDIATOR_SIGNATURE_ID":"MEYCIQDQNqtqB-4Rr_B32UbxjyXUpkK0s_2CONHGm6A634XMiQIhANufYyzsEpi1rWwaf3DleuqGmwe2iNQYKBEnOPX3suOk","MEDIATOR_CERT_ID_3":"MIIDgDCCAWigAwIBAgIKA4gmZ2BliZaGDzANBgkqhkiG9w0BAQsFADAbMRkwFwYDVQQFExBmOTIwMDllODUzYjZiMDQ1MB4XDTIyMDEyNjIyNTAyMFoXDTM3MDEyMjIyNTAyMFowKTETMBEGA1UEChMKR29vZ2xlIExMQzESMBAGA1UEAxMJRHJvaWQgQ0EyMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE_t-4AI454D8pM32ZUEpuaS0ewLjFP9EBOnCF4Kkz2jqcDECp0fjy34AaTCgJnpGdCLIU3u_WXBs3pEECgMuS9RVSKqj584wdbpcxiJahZWSzHqPK1Nn5LZYdQIpLJ9cUo2YwZDAdBgNVHQ4EFgQUOZgHBjozEp71FAY6gEEMcYDOGq0wHwYDVR0jBBgwFoAUNmHhAHyIBQlRi0RsR_8aTMnqTxIwEgYDVR0TAQH_BAgwBgEB_wIBAjAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggIBAD0FO58gwWQb6ROp4c7hkOwQiWiCTG2Ud9Nww5cKlsMU8YlZOk8nXn5OwAfuFT01Kgcbau1CNDECX7qA1vJyQ9HBsoqa7fmi0cf1j_RRBvvAuGvg3zRy0-OckwI2832399l_81FMShS-GczTWfhLJY_ObkVBFkanRCpDhE_SxNHL_5nJzYaH8OdjAKufnD9mcFyYvzjixbcPEO5melGwk7KfCx9miSpVuB6mN1NdoCsSi96ZYQGBlZsE8oLdazckCygTvp2s77GtIswywOHf3HEa39OQm8B8g2cHcy4u5kKoFeSPI9zo6jx-WDb1Er8gKZT1u7lrwCW-JUQquYbGHLzSDIsRfGh0sTjoRH_s4pD371OYAkkPMHVguBZE8iv5uv0j4IBwN_eLyoQb1jmBv_dEUU9ceXd_s8b5-8k7PYhYcDMA0oyFQcvrhLoWbqy7BrY25iWEY5xH6EsHFre5vp1su17Rdmxby3nt7mXz1NxBQdA3rM-kcZlfcK9sHTNVTI290Wy9IS-8_xalrtalo4PA6EwofyXy18XI9AddNs754KPf8_yAMbVc_2aClm1RF7_7vB0fx3eQmLE4WS01SsqsWnCsHCSbyjdIaIyKBFQhABtIIxLNYLFw-0nnA7DBU_M1e9gWBLh8dz1xHFo-Tn5edYaY1bYyhlGBKUKG4M8l","CURYEAR_ID":"23","SERVER_CHALLENGE_ID":"o5cYZDbUiS-OtKbN1GCdFK_1Hx7fHTr6jZAbGDQnQ2Q=","AGEGT_ID":"20"}'

expected_mediator_certificate="51310BCBD8EFEB8913F3EF4E65C9B30CD9ED6C0116A08D137BBFB6B3E2C0A84D"

PROOF_ID  = "PROOF_ID"
HASH_ID  = "HASH_ID"
SERVER_CHALLENGE_ID  = "SERVER_CHALLENGE_ID"
MEDIATOR_SIGNATURE_ID  = "MEDIATOR_SIGNATURE_ID"
MEDIATOR_CERT_ID  = "MEDIATOR_CERT_ID"
AGEGT_ID  = "AGEGT_ID"
CURYEAR_ID  = "CURYEAR_ID"
INIT_ID  = "INIT_ID"

# For DEMO purpose only, does not perform every required check (ie: IsDateValid)
def verify_mediator(jsonString):
    json_data = json.loads(jsonString)

    hash_bytes = base64.urlsafe_b64decode(json_data[HASH_ID])
    server_challenge_bytes =  base64.urlsafe_b64decode(json_data[SERVER_CHALLENGE_ID])

    message = hash_bytes + server_challenge_bytes
    signature_bytes = base64.urlsafe_b64decode(json_data[MEDIATOR_SIGNATURE_ID])

    cert=[]
    cert_raw=[]
    cert_number = 0
    for i in range (10):
        try:
            cert_id = MEDIATOR_CERT_ID+"_"+str(i)
            cert_data = base64.urlsafe_b64decode(json_data[cert_id])
            cert_raw.append(cert_data)
            temp_cert = x509.load_der_x509_certificate(cert_data)
            # print(temp_cert)
            cert.append(temp_cert)
            cert_number+=1
        except:
            break
    root_cert_raw_bytes= cert_raw[-1]
    # print(cert)
    is_valid_chain = True
    for i in reversed(range(cert_number)):
        # print("cert:"+str(i))
        if i==0:
            break
        signer_cert = cert[i]
        issued_cert = cert[i-1]
        try:
            # print(issued_cert.__dir__())
            # This method verifies that the certificate issuer name matches the issuer subject name and that the certificate is signed by the issuer’s private key. No other validation is performed. For any other additional validations required for specific use case (e.g. checking the validity period, whether the signer is allowed to issue certificates, that the issuing certificate has a strong public key, etc), please check accordingly.
            is_signed_by = issued_cert.verify_directly_issued_by(signer_cert)
        except:
            is_valid_chain= False
            break
    
    
    # Mediator is in list: In this specific case, verify it is from AndroidKeyStore and have the specific app_signer (Local Verifier)
    # Note that here the revocation list had not been checked yet, and the cert file is from https://developer.android.com/training/articles/security-key-attestation
    filename = "mediator_cert.pem"
    certificates = []
    cert_obj = pem.parse_file(filename)
    # print(root_cert_raw_bytes.hex())
    has_valid_root_cert = False
    for co in cert_obj:
        root_mediator_cert = x509.load_pem_x509_certificate(co.as_bytes())        
        if root_cert_raw_bytes == root_mediator_cert.public_bytes(Encoding.DER):
            has_valid_root_cert = True
            break
    has_valid_app_signer = False
    # # Get all subject attributes from the certificate
    # for ext in cert[0].extensions:
    #     print(ext)
    # Extension OID https://cs.android.com/android/platform/superproject/+/master:prebuilts/vndk/v30/arm/include/generated-headers/hardware/interfaces/keymaster/4.1/android.hardware.keymaster@4.1_genc++_headers/gen/android/hardware/keymaster/4.1/IKeymasterDevice.h?q=1.3.6.1.4.1.11129.2.1.17&ss=android%2Fplatform%2Fsuperproject
    oid_string = '1.3.6.1.4.1.11129.2.1.17'
    extension_obj = cert[0].extensions.get_extension_for_oid(ObjectIdentifier(oid_string))
    extension_data = extension_obj.value.value
    seq = Sequence.load(extension_data)
    # 0.native is the version, 6 is the SE-AL, 7 is the TEE-AL
    asn1_sequence_obj = seq[6]
    decoder = asn1.Decoder()
    decoder.start(asn1_sequence_obj._contents)
    tag, value = decoder.read()
    while tag != None:
        # Finding the signature of the signed package
        if tag.nr == 709:
            decoder.start(value)
            tag, value = decoder.read()
            # Into Seqeunce
            decoder.start(value)
            tag, seq_value = decoder.read()
            decoder.start(seq_value)
            decoder.read() #dropping AttestationPackageInfo
            tag, seq_digest = decoder.read()
            decoder.start(seq_digest)
            tag, signature = decoder.read()
            while signature!=None:
                if signature.hex().upper() == expected_mediator_certificate.upper():
                    has_valid_app_signer = True
                read_result = decoder.read()
                if read_result!=None:
                    tag, signature = read_result
                else:
                    signature=None
                    tag=None
            break
        read_result = decoder.read()
        if read_result!=None:
            tag, value = read_result
        else: 
            tag=None
            value = None

    is_mediator_verified = is_valid_chain and has_valid_root_cert and has_valid_app_signer

    # Verify Signature is correct
    signature_algorithm = cert[0].signature_algorithm_oid
    is_valid_signature = True
    if signature_algorithm._name == "ecdsa-with-SHA256":
        try:
            # message += message
            cert[0].public_key().verify(signature_bytes,message,ec.ECDSA(hashes.SHA256()))
            # Throw signature exception if not valid.
        except exceptions.InvalidSignature:
            is_valid_signature = False

    return is_mediator_verified and is_valid_signature


if __name__ == "__main__":
    print("Is_Valid Signature: {}".format(verify_mediator(sample)))
