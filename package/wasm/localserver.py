import http.server
from http.server import SimpleHTTPRequestHandler
# import socketserver
# import sys, os, random, string, signal, subprocess
# import time, random, string

class CustomRequestHandler (SimpleHTTPRequestHandler):
    def end_headers (self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        SimpleHTTPRequestHandler.end_headers(self)

def main():
    handler = CustomRequestHandler
    httpd = http.server.HTTPServer(("", 8000), handler)
    httpd.serve_forever()

if __name__ == "__main__":
    main()