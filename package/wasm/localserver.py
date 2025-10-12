import http.server
from http.server import SimpleHTTPRequestHandler
import socketserver
import ssl
import os
import sys

binaryRoot = os.path.dirname(os.path.abspath(__file__))
resourcesRoot = os.path.normpath(binaryRoot + "/../resources")
toolsRoot = os.path.normpath(binaryRoot + "/../tools")

print("Binary Root: ", binaryRoot)
print("Resources Root: ", resourcesRoot)
print("Tools Root: ", toolsRoot)

sys.path.insert(1, toolsRoot)

import gen_meshes
import gen_emitters
import gen_grounds

class CustomRequestHandler (SimpleHTTPRequestHandler):
    def end_headers (self):
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        self.send_header('Connection', 'close')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cross-Origin-Embedder-Policy', 'credentialless')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        SimpleHTTPRequestHandler.end_headers(self)

    def do_POST(self):
        print("post >>> ", self.path)
        SimpleHTTPRequestHandler.do_POST(self)

    def do_GET(self):
        print("get  >>> ", self.path)
        if self.path == "/host_cmd_update":
            # when it becomes critical make update for specific resources
            print("Updating engine resources...")
            gen_meshes.main(resourcesRoot + "/meshes", binaryRoot + "/data/meshes", 1)
            gen_emitters.main(resourcesRoot + "/emitters", binaryRoot + "/data/emitters")
            gen_grounds.main(resourcesRoot + "/grounds", binaryRoot + "/data/grounds", resourcesRoot + "/palette.png")
            print("Finished")

            self.send_response(200)
            self.send_header("Content-type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(b"done")
        else: 
            SimpleHTTPRequestHandler.do_GET(self)

def main():
    context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    context.load_cert_chain(certfile='./cert.pem', keyfile='./key.pem')

    handler = CustomRequestHandler
    handler.protocol_version='HTTP/1.1'
    handler.extensions_map['.js'] = 'text/javascript'
    handler.extensions_map['.wasm'] = 'application/wasm'

    httpd = http.server.ThreadingHTTPServer(("", 9001), handler)
    httpd.allow_reuse_address = True
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
    httpd.serve_forever()

if __name__ == "__main__":
    main()
