import http.server
from http.server import SimpleHTTPRequestHandler
import socketserver
import ssl

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
