from socket import *
import os
import sys
import threading

# Server configuration
proxy_port = 8888
cache_dir = "./cache"

# Create cache directory if not exists
if not os.path.exists(cache_dir):
    os.makedirs(cache_dir)

# Handle client connections
def handle_client(client_socket):
    try:
        message = client_socket.recv(1024).decode()
        print(f"Received request:\n{message}")

        # Parse the request line
        request_line = message.split('\n')[0]
        filename = request_line.split()[1].partition("/")[2]

        # Construct file path for cache
        filepath = os.path.join(cache_dir, filename.replace('/', '_'))

        # Check cache
        if os.path.exists(filepath):
            print(f"Cache hit: {filename}")
            with open(filepath, "rb") as f:
                response = f.read()
            client_socket.sendall(b"HTTP/1.0 200 OK\r\n\r\n" + response)
        else:
            print(f"Cache miss: {filename}")
            # Connect to origin server
            host = filename.split('/')[0]
            path = filename.partition('/')[2]
            origin_socket = socket(AF_INET, SOCK_STREAM)
            origin_socket.connect((host, 80))
            origin_socket.sendall(f"GET /{path} HTTP/1.0\r\nHost: {host}\r\n\r\n".encode())

            # Receive response
            response = b""
            while True:
                data = origin_socket.recv(4096)
                if not data:
                    break
                response += data
            origin_socket.close()

            # Save response to cache
            with open(filepath, "wb") as f:
                f.write(response)

            # Send response to client
            client_socket.sendall(response)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        client_socket.close()

# Start proxy server
def start_server():
    server_socket = socket(AF_INET, SOCK_STREAM)
    server_socket.bind(('', proxy_port))
    server_socket.listen(5)
    print(f"Proxy server running on port {proxy_port}...")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"Connection from {addr}")
        thread = threading.Thread(target=handle_client, args=(client_socket,))
        thread.start()

if __name__ == "__main__":
    start_server()