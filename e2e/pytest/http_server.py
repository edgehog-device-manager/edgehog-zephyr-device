# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import http.server
import logging
import ssl
import threading

logger = logging.getLogger(__name__)

# Global references to manage the server lifecycle
_httpd = None
_server_thread = None

def start_server(port=8443, cert_file='192.0.2.2.pem', key_file='192.0.2.2.key', data_dir='.'):
    logger.info("Starting server with the following configuration:")
    logger.info(f"HTTP server port: {port}")
    logger.info(f"HTTP server certificate: {cert_file}")
    logger.info(f"HTTP server key: {key_file}")
    logger.info(f"HTTP server data directory: {data_dir}")

    global _httpd, _server_thread

    # Create a request handler that serves files from test data directory
    # Cast data_dir to a string in case a pathlib.Path object is passed
    class RequestHandler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(data_dir), **kwargs)

    # Initialize the HTTP server
    server_address = ('0.0.0.0', int(port))
    _httpd = http.server.HTTPServer(server_address, RequestHandler)

    # Establish the TLS context using the generated self-signed certificate
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)

    # Cast Path objects to strings and remove any accidental brackets at the end
    context.load_cert_chain(certfile=str(cert_file), keyfile=str(key_file))

    # Wrap the socket to serve HTTPS
    _httpd.socket = context.wrap_socket(_httpd.socket, server_side=True)

    # Start the server in a separate background thread
    _server_thread = threading.Thread(target=_httpd.serve_forever)
    _server_thread.daemon = True
    _server_thread.start()

    logger.info(f"Background HTTPS server started at https://localhost:{port}")

def stop_server():
    global _httpd, _server_thread
    if _httpd:
        logger.info("Shutting down HTTPS server...")
        # shutdown() blocks until the loop finishes, and MUST be called from
        # a different thread than the one running serve_forever()
        _httpd.shutdown()
        _httpd.server_close()

        # Wait for the background thread to safely exit
        if _server_thread:
            _server_thread.join()

        _httpd = None
        _server_thread = None
        logger.info("Server stopped.")
