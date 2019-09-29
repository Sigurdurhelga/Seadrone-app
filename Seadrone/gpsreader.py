import socket
from gpsrmc import get_gps

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

SERVER_IP = "192.168.43.173"
SERVER_PORT = 5544
SERVER = (SERVER_IP,SERVER_PORT)

while True:
    gps_message = get_gps()
    if gps_message == None:
        gps_message = "64,-21,"
    gps_message = str.encode(gps_message)
        
    sock.sendto(gps_message,SERVER)
