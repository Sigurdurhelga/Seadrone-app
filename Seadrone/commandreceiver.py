"""

    Handle communication between SeadroneCommandCenter
    and Seadrone.

    Author: Sigurdur Helgason

"""


import os, io
import socket
import json
import math

from gpsrmc import get_gps

# Create the TCP server
serversocket = socket.socket(socket.AF_INET,
                             socket.SOCK_STREAM)
serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
# Listen to anything that comes on port 5545
serversocket.bind(("", 5545))
serversocket.listen(5)

command_dict = {}

while True:
    # Get a client
    client_conn, address = serversocket.accept()
    print("GOT CONNECTION", address)
    while True:
        data = client_conn.recv(2048)
        print("done receiving")
        if not data:
            print("nodata")
            break
        command_dict = json.loads(data.decode())

        """
            Start handling commands
        """

        if command_dict['COMMAND_TYPE'] == "WAYPOINT_FOLLOW":
            boat_pos = get_gps()
            if not boat_pos:
                boat_pos = command_dict['boat_position']
            else:
                lat,lon,tc = boat_pos.split(',')
                boat_pos = {
                            'lat':lat,
                            'lng':lon
                        }
            markers = command_dict['markers']
            if markers:
                # temp testing heading function########
                boat_pos = command_dict['markers']['0']
                #######################################
                print(boat_pos)
                boat_lat_rads = math.radians(boat_pos['lat'])
                #boat_lon_rads = math.radians(boat_pos['longitude'])
                boat_cos = math.cos(boat_lat_rads)
                boat_sin = math.sin(boat_lat_rads)

                for key,val in sorted(markers.items(), key=lambda x: x[0]):
                    print("BEARING FROM BOAT POS TO MARKER")
                    print("KEY:",key,"VAL:",val)
                    marker_lat_rads = math.radians(val['lat'])
                    marker_sin = math.sin(marker_lat_rads)
                    marker_cos = math.cos(marker_lat_rads)

                    difference_lon = math.radians(val['lng'] - boat_pos['lng'])

                    difference_cos = math.cos(difference_lon)

                    x = math.sin(difference_lon) * marker_cos

                    y = boat_cos * marker_sin - ( boat_sin * marker_cos * difference_cos )

                    initial_bearing = math.atan2(x,y)
                    print("ATAN VERSION:", initial_bearing)
                    print("COMPASS VERSION:", (initial_bearing+360)%360)

        # Tell the client the command was received and close the connection
        # We close the connection because we want them to reestablish connection 
        # everytime since connectivity is an issue out at sea.
        client_conn.send(b'ack')
        client_conn.close()
        break
    print("DONE COMMUNICATING")
