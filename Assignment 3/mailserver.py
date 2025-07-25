import socket
import ssl
import base64

# Mailtrap config
mailserver = ("live.smtp.mailtrap.io", 587)
username = "smtp@mailtrap.io"
password = "<<token>>"

# Message
msg = "From: hello@demomailtrap.co\r\nTo: khanhnguyenthe.flagship@gmail.com\r\nSubject: Test\r\n\r\nI love computer networks!"
endmsg = "\r\n.\r\n"

# Step 1: Connect with plain socket
context = ssl.create_default_context()
context.check_hostname = False
context.verify_mode = ssl.CERT_NONE
clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
clientSocket.connect(mailserver)

# Step 2: Read 220
print(clientSocket.recv(1024).decode())

# Step 3: EHLO
clientSocket.send(b"EHLO localhost\r\n")
print(clientSocket.recv(1024).decode())

# Step 4: STARTTLS
clientSocket.send(b"STARTTLS\r\n")
recv = clientSocket.recv(1024).decode()
print("STARTTLS:", recv)
if not recv.startswith('220'):
    print("STARTTLS failed")
    clientSocket.close()
    exit()

# Step 5: Wrap socket in TLS
clientSocket = context.wrap_socket(clientSocket, server_hostname=mailserver[0])

# Step 6: Send EHLO again after TLS
clientSocket.send(b"EHLO localhost\r\n")
print(clientSocket.recv(1024).decode())

# Step 7: AUTH LOGIN
clientSocket.send(b"AUTH LOGIN\r\n")
print(clientSocket.recv(1024).decode())

clientSocket.send(base64.b64encode(username.encode()) + b"\r\n")
print(clientSocket.recv(1024).decode())

clientSocket.send(base64.b64encode(password.encode()) + b"\r\n")
print(clientSocket.recv(1024).decode())

# Step 8: MAIL FROM
clientSocket.send(b"MAIL FROM:<hello@demomailtrap.co>\r\n")
print(clientSocket.recv(1024).decode())

# Step 9: RCPT TO
clientSocket.send(b"RCPT TO:<khanhnguyenthe.flagship@gmail.com>\r\n")
print(clientSocket.recv(1024).decode())

# Step 10: DATA
clientSocket.send(b"DATA\r\n")
print(clientSocket.recv(1024).decode())

# Step 11: Send message
clientSocket.send(msg.encode())
clientSocket.send(endmsg.encode())
print(clientSocket.recv(1024).decode())

# Step 12: QUIT
clientSocket.send(b"QUIT\r\n")
print(clientSocket.recv(1024).decode())

clientSocket.close()