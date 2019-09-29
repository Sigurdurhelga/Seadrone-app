#!/usr/bin/python

"""
    Grab GPS information from the UART1 connection, 
    get the GPSRMC information convert it to lat/lon

    Author: Sigurdur Helgason
"""


"""
GPSRMC SYNTAX:

    $GPRMC 0 , TIMESTAMP 1 , VALIDITY STATUS 2 , 
    LATITUDE DMS 3 , NORTH SOUTH 4 , LONGITUDE DMS 5 , 
    EAST WEST 6 , SPEED 7 , TRUE COURSE 8 , DATE 9 , 
    MAGNETIC VAR 10 , CHECKSUM 11
"""

import io, os

tty = io.TextIOWrapper(io.FileIO(
            os.open("/dev/ttyO1",
                    os.O_NOCTTY | 
                    os.O_RDWR),
            "r+"))
def get_gps():
    try:
        for line in iter(tty.readline, None):
            line = line.strip()
            if "$GPRMC" in line:
                gprmc_str_split = line.split(',')

                if gprmc_str_split[2] != 'A':
                    return None

                lat_dotpos = gprmc_str_split[3].find('.')
                lat_minutes = float(gprmc_str_split[3][lat_dotpos-2:])
                lat_degrees = float(gprmc_str_split[3][:lat_dotpos-2])


                lon_dotpos = gprmc_str_split[5].find('.')
                lon_minutes = float(gprmc_str_split[5][lon_dotpos-2:])
                lon_degrees = float(gprmc_str_split[5][:lon_dotpos-2])

                lat = lat_degrees+(lat_minutes/60)
                lon = lon_degrees+(lon_minutes/60)

                if gprmc_str_split[4] == 'S':
                    lat *= -1
                if gprmc_str_split[6] == 'W':
                    lon *= -1

                true_course = gprmc[8]

                gps_message = str(lat)+','+str(lon)+','+str(true_course)
                return gps_message
    except UnicodeDecodeError:
        return get_gps()
if __name__ == "__main__":
    print(get_gps())
