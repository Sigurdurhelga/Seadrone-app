#!/usr/bin/python3
import rcpy.clock as clock
import rcpy.mpu9250 as mpu9250
import rcpy.servo as servo
import rcpy.clock as clock
import time
import math

def mySet(serv, duty, lastDuty):
    timeToWait = 0.2 * abs(duty - lastDuty)
    serv.set(duty)
    serv.run()
    #time.sleep(timeToWait)

COMPASS_FIX = -90
COMPASS_ERR = 5


imu = mpu9250.IMU(enable_dmp = True, dmp_sample_rate = 4,
        enable_magnetometer = True)

serv = servo.Servo(1)
servo.enable()
clck = clock.Clock(serv, 0.01)
clck.start()
duty = 0
lastDuty = duty
mySet(serv, duty, duty)
dataArr = []

prev_heading = 0
change = 0

servo_move = 0.2
servo_min = -1.5
servo_max = 1.5

while True:
    data = imu.read()
    dataArr.append(math.degrees(float(data['head'])))

    if len(dataArr) > 10:
        avr_head = sum(dataArr)/len(dataArr)
        change = prev_heading - avr_head
        print('\r change {}'.format(change),end='')
        prev_heading = avr_head
        #print("my average heading is {}".format(avrHead),end=' ')
        if avr_head > (0 + COMPASS_FIX) + COMPASS_ERR and duty <= servo_max-servo_move:
            #print("increasing duty", end='')
            duty += servo_move
        elif avr_head < (0 + COMPASS_FIX) - COMPASS_ERR and duty >= servo_min+servo_move:
            #print("decreasing duty", end='')
            duty -= servo_move
        else:
            #print("EDGE OF SERVO  ", end='')
            pass

        #print('\r',end='')
        mySet(serv,duty,lastDuty)
        lastDuty = duty
        dataArr = []



