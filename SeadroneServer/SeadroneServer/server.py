from flask import Flask, render_template, url_for, send_from_directory, request

import json
import socket
from threading import Thread, Lock
from queue import Queue
import time

static_folder = './static/public'
react_folder = './static/dist'

app = Flask(__name__, static_folder=static_folder, template_folder=react_folder)

mutex = Lock()

BOAT_LATLNG = {}

boat_command_queue = Queue()

def udp_boat_position_server():

    global BOAT_LATLNG

    UDP_ADDRESS = ""
    UDP_PORT = 5544
    UDP_SERVER = (UDP_ADDRESS, UDP_PORT)

    serversocket = socket.socket(socket.AF_INET,
                                socket.SOCK_DGRAM)

    serversocket.setsockopt(socket.SOL_SOCKET,
                            socket.SO_REUSEADDR,
                            1)

    serversocket.bind(UDP_SERVER)

    while True:
        data, addr = serversocket.recvfrom(2048)
        lat,lng = data.decode().split(',')
        mutex.acquire()
        try:
            BOAT_LATLNG = {
                'lat': lat,
                'lng': lng
            }
        finally:
            mutex.release()

def tcp_boat_commands():
    #BOAT_TCP = "192.168.1.169"
    #BOAT_TCP = "192.168.43.165"
    BOAT_TCP = "192.168.1.176"
    #BOAT_TCP = "localhost"
    BOAT_PORT = 5545
    BOAT_SERVER = (BOAT_TCP, BOAT_PORT)

    global boat_command_queue

    while True:
        command = boat_command_queue.get()
        successful_connection = True
        try:
            boat_tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            boat_tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            boat_tcp_socket.connect(BOAT_SERVER)
        except:
            successful_connection = False
        if successful_connection:
            byte_command = bytes(command, 'utf-8')
            boat_tcp_socket.sendall(byte_command)
            ack = boat_tcp_socket.recv(1024)
            if ack.decode() == 'ack':
                print("command acknowledged!")
            else:
                boat_command_queue.put(command)
            boat_tcp_socket.close()
        else:
            boat_command_queue.put(command)
            time.sleep(10)



@app.route('/')
def react_app():
    return render_template('index.html')

@app.route('/api/command', methods=['POST'])
def receive_command():
    command_dict = request.get_json()
    mutex.acquire()
    try:
        command_dict['boat_position'] = BOAT_LATLNG
    finally:
        mutex.release()
    command_json = json.dumps(command_dict)
    boat_command_queue.put(command_json)
    return ('', 204)

@app.route('/api/boatpos')
def boat_position():
    mutex.acquire()
    try:
        if BOAT_LATLNG != {}:
            status = 200
            response = app.response_class(
                response=json.dumps(BOAT_LATLNG),
                status=status,
                mimetype='application/json'
            )
        else:
            response = app.response_class(
                status = 204
            )
    finally:
        mutex.release()
    return response

"""
    Handle serving static files.
"""

@app.route('/index_bundle.js')
def react_app_js():
    return send_from_directory(react_folder,'index_bundle.js')

@app.route('/OSMPublicTransport/<path:path>')
def send_map_chunk(path):
    return send_from_directory(static_folder+'/OSMPublicTransport/', path)

@app.route('/images/<path:path>')
def send_image(path):
    return send_from_directory(static_folder+'/images/', path)

"""
    Start the servers (flask and communication servers).
"""

if __name__ == '__main__':
    udp_pos_thread = Thread(target=udp_boat_position_server)
    udp_pos_thread.start()
    tcp_cmd_thread = Thread(target=tcp_boat_commands)
    tcp_cmd_thread.start()
    app.run(debug=False)
