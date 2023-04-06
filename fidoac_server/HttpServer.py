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
            # file_path = '/home/kepuss/IdeaProjects/libsnark-fido-ac/build/setup/trustedSetup.json'
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

            # mlog("Snark_Verf Server").info(compltedProcess.returncode)
            mlog("Snark_Verf Server").info(compltedProcess.stdout)
            # mlog("Snark_Verf Server").error(compltedProcess.stderr)

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
# http://localhost:8080/fidoac-server/verify?data=eyJQUk9PRl9JRCI6IkdWcU1oR0RJSzB1ckM2cGxwUS11cWhUcC1taGl6QVpCaEtqdUNIOFU0emRDZTEzeFdER1Y1R1ZON2JkVjMzMlZGbkhKS25FdmRLajZFWDktTjJvUFZKTjljR0M2c3hyNVAybkxRNGcwbTd4cVB2UHQ1SlpQQTl6UURESUxHVGw0RVFESzNSYmpDY3RudWpMek5NVk1Ra3pZalU2SXgwclU0eld1M3dsMlJySVM2WlFDdXBBcG1UU2JUd0J6REtxYkZ1MlZnNnJXZVVaUVltcUJ5aWhERWdobDg1MUVPNVg1R2ZLdGloREV5QmdiR0JCN1NZOUFHdW1XU2x2NGxHRG5BR0NsTUpCWndhVTVFZ3otYzJYbzl5S1dGbU9CazRXeFJjNHM0TU9TU1hVVUw5bi0yTjh3YVhtUWw2RUthTzk3QWhUOHhmZmp0amNuREdWdmlFb2d2YWJqWHFzZHRaS3pueVZraUFlSUhIblB4M1dTeWtMU1FuNkxPeDhmdEhrSER6VUo3NXphX3hTWFF0bDJOWXRwUU1vdmJqbW1QNk40TEMzaE10Smg4Y1dpejkxV2dLbGUtdHJrMzdDOUhhdFdGWGt5WEFzUzh4OUw4RmdZTVFyVHJOdk9rcUxtX2lKYWljMjZWbUpoQVlUSGk0ZUd5ZDNpREhNcjVVRE8zLWNrIiwiSEFTSF9JRCI6Ing3Q2FYazhfR0pLT3lkQjdEV2dvdVhyM2N6UDFja3R4bHpiZnlpNkdGaVE9IiwiQ1VSWUVBUl9JRCI6MjMsIkFHRUdUX0lEIjoyMH0=

# POST
# curl -X POST http://localhost:8080/fidoac-server/verify -H "Content-Type: application/json" -d '{"data": "eyJQUk9PRl9JRCI6IkdWcU1oR0RJSzB1ckM2cGxwUS11cWhUcC1taGl6QVpCaEtqdUNIOFU0emRDZTEzeFdER1Y1R1ZON2JkVjMzMlZGbkhKS25FdmRLajZFWDktTjJvUFZKTjljR0M2c3hyNVAybkxRNGcwbTd4cVB2UHQ1SlpQQTl6UURESUxHVGw0RVFESzNSYmpDY3RudWpMek5NVk1Ra3pZalU2SXgwclU0eld1M3dsMlJySVM2WlFDdXBBcG1UU2JUd0J6REtxYkZ1MlZnNnJXZVVaUVltcUJ5aWhERWdobDg1MUVPNVg1R2ZLdGloREV5QmdiR0JCN1NZOUFHdW1XU2x2NGxHRG5BR0NsTUpCWndhVTVFZ3otYzJYbzl5S1dGbU9CazRXeFJjNHM0TU9TU1hVVUw5bi0yTjh3YVhtUWw2RUthTzk3QWhUOHhmZmp0amNuREdWdmlFb2d2YWJqWHFzZHRaS3pueVZraUFlSUhIblB4M1dTeWtMU1FuNkxPeDhmdEhrSER6VUo3NXphX3hTWFF0bDJOWXRwUU1vdmJqbW1QNk40TEMzaE10Smg4Y1dpejkxV2dLbGUtdHJrMzdDOUhhdFdGWGt5WEFzUzh4OUw4RmdZTVFyVHJOdk9rcUxtX2lKYWljMjZWbUpoQVlUSGk0ZUd5ZDNpREhNcjVVRE8zLWNrIiwiSEFTSF9JRCI6Ing3Q2FYazhfR0pLT3lkQjdEV2dvdVhyM2N6UDFja3R4bHpiZnlpNkdGaVE9IiwiQ1VSWUVBUl9JRCI6MjMsIkFHRUdUX0lEIjoyMH0="}'
