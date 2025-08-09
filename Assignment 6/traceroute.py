from socket import *
import os
import sys
import struct
import time
import select

ICMP_ECHO_REQUEST = 8
MAX_HOPS = 30
TIMEOUT = 2.0
TRIES = 2

def checksum(string):
    csum = 0
    countTo = (len(string) // 2) * 2
    count = 0
    while count < countTo:
        thisVal = string[count + 1] * 256 + string[count]
        csum += thisVal
        csum &= 0xffffffff
        count += 2
    if countTo < len(string):
        csum += string[len(string) - 1]
        csum &= 0xffffffff
    csum = (csum >> 16) + (csum & 0xffff)
    csum += (csum >> 16)
    answer = ~csum & 0xffff
    answer = answer >> 8 | (answer << 8 & 0xff00)
    return answer

def build_packet():
    myID = os.getpid() & 0xFFFF
    header = struct.pack("bbHHh", ICMP_ECHO_REQUEST, 0, 0, myID, 1)
    data = struct.pack("d", time.time())
    myChecksum = checksum(header + data)
    header = struct.pack("bbHHh", ICMP_ECHO_REQUEST, 0, myChecksum, myID, 1)
    packet = header + data
    return packet

def get_route(hostname):
    for ttl in range(1, MAX_HOPS):
        for tries in range(TRIES):
            destAddr = gethostbyname(hostname)

            # Fill in start
            mySocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)
            # Fill in end

            mySocket.setsockopt(IPPROTO_IP, IP_TTL, struct.pack('I', ttl))
            mySocket.settimeout(TIMEOUT)

            try:
                d = build_packet()
                mySocket.sendto(d, (hostname, 0))
                t = time.time()
                startedSelect = time.time()
                whatReady = select.select([mySocket], [], [], TIMEOUT)
                howLongInSelect = (time.time() - startedSelect)

                if whatReady[0] == []:  # Timeout
                    print(f"{ttl}  * * * Request timed out.")
                    continue

                recvPacket, addr = mySocket.recvfrom(1024)
                timeReceived = time.time()

                # Fill in start
                icmpHeader = recvPacket[20:28]
                types, code, checksum_recv, packetID, sequence = struct.unpack("bbHHh", icmpHeader)
                # Fill in end

                if types == 11:  # Time Exceeded
                    bytes_in_double = struct.calcsize("d")
                    timeSent = struct.unpack("d", recvPacket[28:28 + bytes_in_double])[0]
                    print(f"{ttl}  rtt={(timeReceived - t) * 1000:.0f} ms  {addr[0]}")
                elif types == 3:  # Destination Unreachable
                    bytes_in_double = struct.calcsize("d")
                    timeSent = struct.unpack("d", recvPacket[28:28 + bytes_in_double])[0]
                    print(f"{ttl}  rtt={(timeReceived - t) * 1000:.0f} ms  {addr[0]}")
                elif types == 0:  # Echo Reply
                    bytes_in_double = struct.calcsize("d")
                    timeSent = struct.unpack("d", recvPacket[28:28 + bytes_in_double])[0]
                    print(f"{ttl}  rtt={(timeReceived - timeSent) * 1000:.0f} ms  {addr[0]}")
                    return
                else:
                    print("Error: Unknown ICMP type.")
                    break
            except timeout:
                continue
            finally:
                mySocket.close()

if __name__ == "__main__":
    host = input("Enter host to traceroute: ")
    get_route(host)