# Python 3 server example
from http.server import BaseHTTPRequestHandler, HTTPServer
import time
import http

hostName = "0.0.0.0"
serverPort = 8080

class MyServer(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        print(self.path)
        if self.path == '/trustedSetup.json':
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            file_path = '/app/build/setup/trustedSetup.json'
            # file_path = '/home/kepuss/IdeaProjects/libsnark-fido-ac/build/setup/trustedSetup.json'
            with open(file_path, 'rb') as file:
                self.wfile.write(file.read())

if __name__ == "__main__":
    webServer = HTTPServer((hostName, serverPort), MyServer)
    print("Server started http://%s:%s" % (hostName, serverPort))

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")