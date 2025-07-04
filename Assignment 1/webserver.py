from socket import *
import os

# Define server port
server_port = 6789

# Create TCP socket
server_socket = socket(AF_INET, SOCK_STREAM)

# Bind to local IP and port
server_socket.bind(('', server_port))

# Listen for incoming connections
server_socket.listen(1)
print(f'Server is ready to receive on port {server_port}...')

while True:
    # Accept a new connection from the client
    connection_socket, addr = server_socket.accept()
    try:
        # Receive HTTP request from client
        message = connection_socket.recv(1024).decode()
        print("Received request:")
        print(message)

        # Parse HTTP GET request
        filename = message.split()[1]  # e.g., /HelloWorld.html
        filepath = filename[1:]  # remove leading "/"

        # Open and read requested file
        with open(filepath, 'r') as f:
            output_data = f.read()

        # Send HTTP response header
        response_header = 'HTTP/1.1 200 OK\r\n'
        response_header += 'Content-Type: text/html\r\n'
        response_header += '\r\n'

        # Send response header and file content
        connection_socket.send(response_header.encode())
        connection_socket.send(output_data.encode())

    except FileNotFoundError:
        # If file not found, return 404
        error_response = 'HTTP/1.1 404 Not Found\r\n\r\n'
        error_response += '<html><body><h1>404 Not Found</h1></body></html>'
        connection_socket.send(error_response.encode())

    except Exception as e:
        print("Error processing request:", e)

    finally:
        # Close client socket
        connection_socket.close()