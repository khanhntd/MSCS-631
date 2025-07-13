# UDPPingerClient.py
import time
from socket import *

# Create a UDP socket
clientSocket = socket(AF_INET, SOCK_DGRAM)
clientSocket.settimeout(1)  # Set timeout to 1 second

# Server address and port
serverName = '127.0.0.1'  # Use localhost for testing
serverPort = 12000

# Send 10 pings
for sequence_number in range(1, 11):
    # Format message
    send_time = time.time()
    message = f"Ping {sequence_number} {send_time}"

    try:
        # Send the ping
        clientSocket.sendto(message.encode(), (serverName, serverPort))

        # Receive the response
        response, serverAddress = clientSocket.recvfrom(1024)

        # Measure RTT
        rtt = time.time() - send_time

        print(f"Reply from {serverAddress}: {response.decode()} RTT = {rtt:.6f} sec")

    except timeout:
        print("Request timed out")

# Close the socket
clientSocket.close()