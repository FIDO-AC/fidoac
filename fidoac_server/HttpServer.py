# Python 3 server example
from http.server import BaseHTTPRequestHandler, HTTPServer
import time
import http
import subprocess
from urllib.parse import urlparse, parse_qs
import base64
import logging
import random
import os
import json
import cgi
import MediatorVerifier

hostName = "0.0.0.0"
serverPort = 8080

def mlog(mod_name):
    """
    To use this, do logger = get_module_logger(__name__)
    """
    logger = logging.getLogger(mod_name)
    handler = logging.StreamHandler()
    formatter = logging.Formatter(
        '%(asctime)s [%(name)-12s] %(levelname)-8s %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(logging.DEBUG)
    return logger

class MyServer(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        mlog("Snark_Verf Server").info("New Request")
        parsed_url = urlparse(self.path)
        mlog("Snark_Verf Server").info(parsed_url)
        if self.path == '/fidoac-server/trustedSetup.json':
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            file_path = '/app/build/setup/trustedSetup.json'
            with open(file_path, 'rb') as file:
                self.wfile.write(file.read())
        elif self.path.startswith("/fidoac-server/verify"):    
            query_dict= dict(parse_qs(parsed_url.query))
            mlog("Snark_Verf Server").info((query_dict["data"])[0])

            json_bytes = base64.urlsafe_b64decode((query_dict["data"])[0])
            data_string_json = (json_bytes).decode("utf-8")
            mlog("Snark_Verf Server").info(data_string_json)

            fileoffset = random.randint(0, 999999999)
            tempfilename = '/app/inputfile' + str(fileoffset) + ".json"
            with open(tempfilename, 'w') as file: 
                file.write(data_string_json)
            
            compltedProcess = subprocess.run(["/app/fidoac_zksnark_verf", "/app/verificationkey", tempfilename], capture_output=True, text=True)
            subprocess.run(["rm", tempfilename])

            mlog("Snark_Verf Server").info(compltedProcess.stdout)

            response = {}
            if (compltedProcess.stdout.find("Verified?: true") != -1):
                response["verified"] = True
            elif (compltedProcess.stdout.find("Verified?: false") != -1):
                response["verified"] = False
            else:
                response["error"] = True
            # send the message back
            self.send_response(200)
            self.send_header('Content-Type','application/json')
            self.end_headers()
            self.wfile.write(json.dumps(response).encode('utf-8'))
            
        else:
            print("Invalid URL")
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(bytes("<html><head><title>Failed.</title></head><body>failed</body></html>","utf-8")) 
            self.send_response(404)

    def do_POST(self):
        mlog("Snark_Verf Server").info("New POST Request")
        parsed_url = urlparse(self.path)
        mlog("Snark_Verf Server").info(parsed_url)
        if self.path.startswith("/fidoac-server/verify"):
            ctype, pdict = cgi.parse_header(self.headers.get('content-type'))
            
            content_len = int(self.headers.get('Content-Length'))
            post_body = self.rfile.read(content_len)

            mlog("Snark_Verf Server").info(post_body)

            data_string_json = (post_body).decode("utf-8")
            mlog("Snark_Verf Server").info(data_string_json)

            fileoffset = random.randint(0, 999999999)
            tempfilename = '/app/inputfile' + str(fileoffset) + ".json"
            with open(tempfilename, 'w') as file: 
                file.write(data_string_json)
            
            compltedProcess = subprocess.run(["/app/fidoac_zksnark_verf", "/app/verificationkey", tempfilename], capture_output=True, text=True)
            subprocess.run(["rm", tempfilename])

            mlog("Snark_Verf Server").info(compltedProcess.returncode)
            mlog("Snark_Verf Server").error(compltedProcess.stderr)
            mlog("Snark_Verf Server").info(compltedProcess.stdout)

            response = {}
            if (compltedProcess.stdout.find("Verified?: true") != -1):
                if (MediatorVerifier.verify_mediator(data_string_json) == True):
                    response["verified"] = True
                else:
                    response["verified"] = False
            elif (compltedProcess.stdout.find("Verified?: false") != -1):
                response["verified"] = False
            else:
                response["error"] = True
            # send the message back
            self.send_response(200)
            self.send_header('Content-Type','application/json')
            self.end_headers()
            self.wfile.write(json.dumps(response).encode('utf-8'))


    def verify_zkp(self, data_string_json):
        fileoffset = random.randint(0, 999999999)
        tempfilename = '/tmp/inputfile' + str(fileoffset) + ".json"
        with open(tempfilename, 'w') as file:
            file.write(data_string_json)
        compltedProcess = subprocess.run(["/app/fidoac_zksnark_verf", "/app/verificationkey", tempfilename],
                                         capture_output=True, text=True)
        subprocess.run(["rm", tempfilename])
        return compltedProcess


if __name__ == "__main__":
    webServer = HTTPServer((hostName, serverPort), MyServer)
    mlog("Snark_Verf Server").info("Server started http://%s:%s" % (hostName, serverPort))

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    mlog("Snark_Verf Server").info("Server stopped.")


# Test Sample
# Do not use GET

# POST raw data for testing (same in deployed prototype)
# {"PROOF_ID":"ERPrFDrlFsoD1BGi_AU1tlq4Ymt5eRAroMrRCBaIvnadQIOU_c0UQjATP9CHGf-pDXVnuOcyt-00EHDw5W0DDEmsmBL100-L_kjA6io5_3juaYyNGRNWtV1ZtWAsbmYTASnXAOvS5JORSqtP-8j8W8OXEp2mQ1OyO1QoibKZdl5qGd7u6amqpFgeQ2YXL6FEB1UxUeekiIAEU0NjWQV_0Y4hmYsAH678KvTpsRyqMhZktOD9Mtgde1tJlVy1Suv2EmtKfm3jOHTr2HpbMYrOkJ-wSNCoGMhSvPSgmgLqpNCGmNwAVknocA4KYMBTW_zKEQBEi1_AfoSF_NSICySMpWo8pceMUaJUoVVBp1fzk2Hv5mo_-DAZQp66X2wYrF-SABOKrvpHxwTQoWTWdSu7lZdOsa9hZzstqEFika53VukHDniykDLT2JWb1eB6nLIpAvKMQHMtFSHAYcNliyOK6lr9-jJgqVbOCl6ybKPFa_DZvt1yH4t8fdJ8vtxHBDhs","MEDIATOR_CERT_ID_0":"MIICnTCCAkKgAwIBAgIBATAKBggqhkjOPQQDAjA5MQwwCgYDVQQKEwNURUUxKTAnBgNVBAMTIDc1NWUwZDQ5YWIyZmE3MjM5M2U0MjIxZDBkMDM5MzJlMB4XDTcwMDEwMTAwMDAwMFoXDTQ4MDEwMTAwMDAwMFowHzEdMBsGA1UEAxMUQW5kcm9pZCBLZXlzdG9yZSBLZXkwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAQTco2k-UFAA-apbLbuISHvEKVbbPmBPNUL_J1Htr6ljjDujQmvjCjt2bFkSRhtSFC1yMpZf5j_GW6uQo5Gqnaao4IBUzCCAU8wDgYDVR0PAQH_BAQDAgeAMIIBOwYKKwYBBAHWeQIBEQSCASswggEnAgIAyAoBAQICAMgKAQEEIKOXGGQ21IkvjrSmzdRgnRSv9R8e3x06-o2QGxg0J0NkBAAwTL-FPQgCBgGHb_UD2L-FRTwEOjA4MRIwEAQLYW5vbi5maWRvYWMCAQExIgQgUTELy9jv64kT8-9OZcmzDNntbAEWoI0Te7-2s-LAqE0wgaShCDEGAgECAgEDogMCAQOjBAICAQClBTEDAgEEqgMCAQG_g3cCBQC_hT4DAgEAv4VATDBKBCBC7RvKNS-r1CjzTo_O5id29MssZuBvguWln_RJUme_wgEB_woBAAQgeizj-0Qb44b_mKCLVzKqK8Gs8s329hp0kPj6mXccGne_hUEFAgMB-9C_hUIFAgMDFeS_hU4GAgQBNI0Vv4VPBgIEATSNFTAKBggqhkjOPQQDAgNJADBGAiEA5z7_MJlh2WbANC8_3em98tLnOtUpIG9wkaKOC1TbsgoCIQDh7y8AZJ_PNS8-Nc3jf4725BivYtZ1dpabX5_xO5tPnA==","HASH_ID":"Skr-UX_9CNi6ot9TupaZs37t133bvQkdeE3Qt0312Dw=","MEDIATOR_CERT_ID_2":"MIIB1jCCAV2gAwIBAgIUAKQDvuF51iSj420QvO0qwmh_ebQwCgYIKoZIzj0EAwMwKTETMBEGA1UEChMKR29vZ2xlIExMQzESMBAGA1UEAxMJRHJvaWQgQ0EyMB4XDTIzMDMyODE1MDM0MloXDTIzMDUwMjE1MDM0MVowKTETMBEGA1UEChMKR29vZ2xlIExMQzESMBAGA1UEAxMJRHJvaWQgQ0EzMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEvGl9LTWzBAbcWnJISkBkqArCuRIlZMHKwoZr-3ckECOiZurMZzD8Ftb7mYTeFPdt9n0MarXKSX7nf_4qyTf88aNjMGEwDgYDVR0PAQH_BAQDAgIEMA8GA1UdEwEB_wQFMAMBAf8wHQYDVR0OBBYEFDcMcBkYg2MZx6zbDLXZhSJKIMemMB8GA1UdIwQYMBaAFDmYBwY6MxKe9RQGOoBBDHGAzhqtMAoGCCqGSM49BAMDA2cAMGQCMA4zQC2WeaXT1VtuLf_zKw4dr5tvwLQn-P6FkvGFr8R5vORcoUIu5SlAGoJ1Ng9XTwIwEEiDLGTG6gw7lzzbwn2eJyCr9OFtz8Cw56G9Ebdh4B7FY_rmqRBO8661ExdnZwI0","MEDIATOR_CERT_ID_1":"MIIBwjCCAWmgAwIBAgIQdV4NSasvpyOT5CIdDQOTLjAKBggqhkjOPQQDAjApMRMwEQYDVQQKEwpHb29nbGUgTExDMRIwEAYDVQQDEwlEcm9pZCBDQTMwHhcNMjMwMzI2MTc0NjEwWhcNMjMwNDI3MTc0NjEwWjA5MQwwCgYDVQQKEwNURUUxKTAnBgNVBAMTIDc1NWUwZDQ5YWIyZmE3MjM5M2U0MjIxZDBkMDM5MzJlMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE08By_9SEw0pa6G0lRYeeA_R9iqPMwH1UUkqdceObl8TYHROiWTVImZnCkgOrKuKbHnYdVXO7q7ojKEt1C5jiQaNjMGEwHQYDVR0OBBYEFMCuBTB0Yu6B12d7idWjxNyDH848MB8GA1UdIwQYMBaAFDcMcBkYg2MZx6zbDLXZhSJKIMemMA8GA1UdEwEB_wQFMAMBAf8wDgYDVR0PAQH_BAQDAgIEMAoGCCqGSM49BAMCA0cAMEQCIFDnaxeX3hD8Jb0HEBOqO24R-NUOFk3FWX8aWaQH-emAAiA42xV7mly_1GNiF-0htnrxciZVD1TYvktdG6kSlMltOg==","MEDIATOR_CERT_ID_4":"MIIFHDCCAwSgAwIBAgIJANUP8luj8tazMA0GCSqGSIb3DQEBCwUAMBsxGTAXBgNVBAUTEGY5MjAwOWU4NTNiNmIwNDUwHhcNMTkxMTIyMjAzNzU4WhcNMzQxMTE4MjAzNzU4WjAbMRkwFwYDVQQFExBmOTIwMDllODUzYjZiMDQ1MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAr7bHgiuxpwHsK7Qui8xUFmOr75gvMsd_dTEDDJdSSxtf6An7xyqpRR90PL2abxM1dEqlXnf2tqw1Ne4Xwl5jlRfdnJLmN0pTy_4lj4_7tv0Sk3iiKkypnEUtR6WfMgH0QZfKHM1-di-y9TFRtv6y__0rb-T-W8a9nsNL_ggjnar86461qO0rOs2cXjp3kOG1FEJ5MVmFmBGtnrKpa73XpXyTqRxB_M0n1n_W9nGqC4FSYa04T6N5RIZGBN2z2MT5IKGbFlbC8UrW0DxW7AYImQQcHtGl_m00QLVWutHQoVJYnFPlXTcHYvASLu-RhhsbDmxMgJJ0mcDpvsC4PjvB-TxywElgS70vE0XmLD-OJtvsBslHZvPBKCOdT0MS-tgSOIfga-z1Z1g7-DVagf7quvmag8jfPioyKvxnK_EgsTUVi2ghzq8wm27ud_mIM7AY2qEORR8Go3TVB4HzWQgpZrt3i5MIlCaY504LzSRiigHCzAPlHws-W0rB5N-er5_2pJKnfBSDiCiFAVtCLOZ7gLiMm0jhO2B6tUXHI_-MRPjy02i59lINMRRev56GKtcd9qO_0kUJWdZTdA2XoS82ixPvZtXQpUpuL12ab-9EaDK8Z4RHJYYfCT3Q5vNAXaiWQ-8PTWm2QgBR_bkwSWc-NpUFgNPN9PvQi8WEg5UmAGMCAwEAAaNjMGEwHQYDVR0OBBYEFDZh4QB8iAUJUYtEbEf_GkzJ6k8SMB8GA1UdIwQYMBaAFDZh4QB8iAUJUYtEbEf_GkzJ6k8SMA8GA1UdEwEB_wQFMAMBAf8wDgYDVR0PAQH_BAQDAgIEMA0GCSqGSIb3DQEBCwUAA4ICAQBOMaBc8oumXb2voc7XCWnuXKhBBK3e2KMGz39t7lA3XXRe2ZLLAkLM5y3J7tURkf5a1SutfdOyXAmeE6SRo83Uh6WszodmMkxK5GM4JGrnt4pBisu5igXEydaW7qq2CdC6DOGjG-mEkN8_TA6p3cnoL_sPyz6evdjLlSeJ8rFBH6xWyIZCbrcpYEJzXaUOEaxxXxgYz5_cTiVKN2M1G2okQBUIYSY6bjEL4aUN5cfo7ogP3UvliEo3Eo0YgwuzR2v0KR6C1cZqZJSTnghIC_vAD32KdNQ-c3N-vl2OTsUVMC1GiWkngNx1OO1-kXW-YTnnTUOtOIswUP_Vqd5SYgAImMAfY8U9_iIgkQj6T2W6FsScy94IN9fFhE1UtzmLoBIuUFsVXJMTz-Jucth-IqoWFua9v1R93_k98p41pjtFX-H8DslVgfP097vju4KDlqN64xV1grw3ZLl4CiOe_A91oeLm2UHOq6wn3esB4r2EIQKb6jTVGu5sYCcdWpXr0AUVqcABPdgL-H7qJguBw09ojm6xNIrw2OocrDKsudk_okr_AwqEyPKw9WnMlQgLIKw1rODG2NvU9oR3GVGdMkUBZutL8VuFkERQGt6vQ2OCw0sV47VMkuYbacK_xyZFiRcrPJPb41zgbQj9XAEyLKCHex0SdDrx-tWUDqG8At2JHA==","MEDIATOR_SIGNATURE_ID":"MEYCIQDQNqtqB-4Rr_B32UbxjyXUpkK0s_2CONHGm6A634XMiQIhANufYyzsEpi1rWwaf3DleuqGmwe2iNQYKBEnOPX3suOk","MEDIATOR_CERT_ID_3":"MIIDgDCCAWigAwIBAgIKA4gmZ2BliZaGDzANBgkqhkiG9w0BAQsFADAbMRkwFwYDVQQFExBmOTIwMDllODUzYjZiMDQ1MB4XDTIyMDEyNjIyNTAyMFoXDTM3MDEyMjIyNTAyMFowKTETMBEGA1UEChMKR29vZ2xlIExMQzESMBAGA1UEAxMJRHJvaWQgQ0EyMHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE_t-4AI454D8pM32ZUEpuaS0ewLjFP9EBOnCF4Kkz2jqcDECp0fjy34AaTCgJnpGdCLIU3u_WXBs3pEECgMuS9RVSKqj584wdbpcxiJahZWSzHqPK1Nn5LZYdQIpLJ9cUo2YwZDAdBgNVHQ4EFgQUOZgHBjozEp71FAY6gEEMcYDOGq0wHwYDVR0jBBgwFoAUNmHhAHyIBQlRi0RsR_8aTMnqTxIwEgYDVR0TAQH_BAgwBgEB_wIBAjAOBgNVHQ8BAf8EBAMCAQYwDQYJKoZIhvcNAQELBQADggIBAD0FO58gwWQb6ROp4c7hkOwQiWiCTG2Ud9Nww5cKlsMU8YlZOk8nXn5OwAfuFT01Kgcbau1CNDECX7qA1vJyQ9HBsoqa7fmi0cf1j_RRBvvAuGvg3zRy0-OckwI2832399l_81FMShS-GczTWfhLJY_ObkVBFkanRCpDhE_SxNHL_5nJzYaH8OdjAKufnD9mcFyYvzjixbcPEO5melGwk7KfCx9miSpVuB6mN1NdoCsSi96ZYQGBlZsE8oLdazckCygTvp2s77GtIswywOHf3HEa39OQm8B8g2cHcy4u5kKoFeSPI9zo6jx-WDb1Er8gKZT1u7lrwCW-JUQquYbGHLzSDIsRfGh0sTjoRH_s4pD371OYAkkPMHVguBZE8iv5uv0j4IBwN_eLyoQb1jmBv_dEUU9ceXd_s8b5-8k7PYhYcDMA0oyFQcvrhLoWbqy7BrY25iWEY5xH6EsHFre5vp1su17Rdmxby3nt7mXz1NxBQdA3rM-kcZlfcK9sHTNVTI290Wy9IS-8_xalrtalo4PA6EwofyXy18XI9AddNs754KPf8_yAMbVc_2aClm1RF7_7vB0fx3eQmLE4WS01SsqsWnCsHCSbyjdIaIyKBFQhABtIIxLNYLFw-0nnA7DBU_M1e9gWBLh8dz1xHFo-Tn5edYaY1bYyhlGBKUKG4M8l","CURYEAR_ID":"23","SERVER_CHALLENGE_ID":"o5cYZDbUiS-OtKbN1GCdFK_1Hx7fHTr6jZAbGDQnQ2Q=","AGEGT_ID":"20"}