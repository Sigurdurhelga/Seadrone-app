#include <roboticscape.h>
#include <thread>
#include <iostream>
#include <string>
#include <cmath>
#include <csignal>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/system/system_error.hpp>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>

#define I2C_BUS 2
#define GPIO_INT_PIN_CHIP 3
#define GPIO_INT_PIN_PIN  21
#define UART_GPS_PORT 1
#define SERVO_PORT 1

using namespace std;

mutex gpsmutex;
string BOATGPS = "64,-21"; // first value is just something random.

static int running;

// this can be read for the heading data and more
rc_mpu_data_t data;



/* methods */

int reset_servo(const int ch);
float calculate_heading_from_lat_lng(string latlng_from, string latlng_to);
float calculate_distance_from_lat_lng(string latlng_from, string latlng_to);
string get_gprmc();
int rudder_control_to_marker(string markerlatlon);

void udp_gps_server(){
	boost::asio::io_service io_service;

	boost::asio::ip::udp::socket socket(io_service);
	boost::asio::ip::udp::endpoint remote_endpoint;
	boost::system::error_code err;

	socket.open(boost::asio::ip::udp::v4());

	while(running){
		remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string("192.168.1.154"), 5544);
		gpsmutex.lock();
		BOATGPS = get_gprmc();
		gpsmutex.unlock();
		socket.send_to(boost::asio::buffer(BOATGPS, BOATGPS.length()), remote_endpoint, 0, err);
		this_thread::sleep_for(chrono::milliseconds(5000));
	}

	socket.close();
}

void signal_handler(int num) {
	/* basic signal handler, ends the regular 
	 * loop the beaglebone is running
	 */
	running = 0;
	return;
}


int main(){
	//cout << calculate_distance_from_lat_lng("64,-33.0222", "33,22") << endl;
	//return 0;
	/*
	 * STARTING SETUP.
	 */
	rc_mpu_config_t conf = rc_mpu_default_config();
	conf.i2c_bus = I2C_BUS;
	conf.gpio_interrupt_pin_chip = GPIO_INT_PIN_CHIP;
	conf.gpio_interrupt_pin = GPIO_INT_PIN_PIN;
	conf.enable_magnetometer = 1;


	if(rc_mpu_initialize_dmp(&data, conf)){
		printf("rc_mpu_initialize_failed\n");
		return -1;
	}

	int GPS_BAUD = 9600;

	rc_uart_init(UART_GPS_PORT,GPS_BAUD,1,0,1,0);


	signal(SIGINT, signal_handler);
	running = 1;
		
	/*
	 * Check for battery. Can't run servos without it
	 */

	if(rc_adc_init()){
		printf("error running adc init\n");
		return -1;
	}
	if(rc_adc_batt() < 6.0){
		printf("battery disconnected or insufficient charge\n");
		return -1;
	}

	rc_adc_cleanup();

	rc_servo_init();

	rc_servo_power_rail_en(1);

	/*
	 * START THE SERVER THAT SENDS GPS POSITION
	 */
	thread udp_serv(udp_gps_server);
	
	/*
	 * END OF SETUP.
	 */
	rudder_control_to_marker("64,-21");
	printf("done\n");

	rc_usleep(50000);

	rc_servo_power_rail_en(0);
	rc_servo_cleanup();
	rc_dsm_cleanup();
	rc_mpu_power_off();
	fflush(stdout);	
	return 0;
}

int rudder_control_to_marker(string markerlatlon) {

	//cout << " in rudder " << endl;
	int servo_port = SERVO_PORT;
	int freq_hz = 50;
	float SERVO_LOWER = -1.5;
	float SERVO_UPPER = 1.5;
	
	int COMPASS_ERROR = 0;
	int TARGET_OFFSET = 15;
	float SERVO_STEP = 0.01;
	float TARGET;

	float servo_pos = 0;
	float curr_heading;

	float target_error;
	float target_error_left;
	float target_error_right;
	bool should_send_signal = false;
	
	auto last_time = chrono::system_clock::now();
	float output;
	float error, error_sum,delta_error;
	float last_error = 0;
	float kp = 1, ki = 0, kd = 0;

	if(reset_servo(servo_port) == -1) return -1;

	while(running){
		should_send_signal = false;
		curr_heading = data.compass_heading*RAD_TO_DEG + 180; // INPUT
		curr_heading += COMPASS_ERROR;
		gpsmutex.lock();
		string boatlatlon = BOATGPS;
		gpsmutex.unlock();
		auto now = chrono::system_clock::now();

		auto time_change = chrono::duration_cast<chrono::milliseconds>(now - last_time).count();

		TARGET = calculate_heading_from_lat_lng(boatlatlon, markerlatlon); // SETPOINT

		if(TARGET < curr_heading){
			target_error_right = (360 - curr_heading) + TARGET;
		} else {
			target_error_right = TARGET - curr_heading;
		}

		target_error_left = 360 - target_error_right;
		target_error = min(target_error_left, target_error_right);
		error_sum = (target_error * time_change);
		delta_error = (error - last_error) / time_change;
		
		output = kp * error + ki * error_sum + kd * delta_error;

		cout << "pid output " << output << endl;

		last_error = target_error;
		last_time = now;

		if(target_error > TARGET_OFFSET){
			/* now we know the boat is not heading within the bounds of TARGET */
			if(target_error == target_error_left && SERVO_LOWER+SERVO_STEP <= servo_pos ){
				cout << "downing servo" << endl;
				servo_pos -= SERVO_STEP;
				should_send_signal = true;
			} else if (target_error == target_error_right && SERVO_UPPER-SERVO_STEP >= servo_pos ) {
				cout << "upping servo" << endl;
				servo_pos += SERVO_STEP;
				should_send_signal = true;
			}
		}
		if(should_send_signal){
			if(rc_servo_send_pulse_normalized(servo_port, servo_pos) == -1){
				printf("failed\n");
				return -1;
			}
		}
		//cout << "distance to target " << calculate_distance_from_lat_lng(boatlatlon, markerlatlon) << endl;
		if(calculate_distance_from_lat_lng(boatlatlon, markerlatlon) < 10){
			/* closer than 10m from our target */
			cout << "we are close to our target woo " << endl;
		}
		rc_usleep(500000/freq_hz);
		
	}

}

float calculate_heading_from_lat_lng(string latlng_from, string latlng_to){	
	/*
		Calculates the forward azimuth from one latlng coord to another latlon
		the output is in degrees [0 - 360]

		example:
			inp("51.50,0", "-22.97,-43.18") : outp = 219.35
	*/
	vector<string> results_from;
	boost::split(results_from, latlng_from, [](char c){return c == ',';});
	vector<string> results_to;
	boost::split(results_to, latlng_to, [](char c){return c == ',';});
	float from_lat = stof(results_from[0])*DEG_TO_RAD;
	float to_lat = stof(results_to[0])*DEG_TO_RAD;
	float from_lng = stof(results_from[1]);
	float to_lng = stof(results_to[1]);
	float from_cos = cos(from_lat);
	float from_sin = sin(from_lat);
	float to_cos = cos(to_lat);
	float to_sin = sin(to_lat);
	float difference_lng = (to_lng - from_lng) * DEG_TO_RAD;
	float difference_lng_sin = sin(difference_lng);
	float difference_lng_cos = cos(difference_lng);
	float azimuth = atan2((difference_lng_sin*to_cos), (from_cos * to_sin - from_sin * to_cos * difference_lng_cos));
	return fmod((azimuth*RAD_TO_DEG)+360, 360);
}

float calculate_distance_from_lat_lng(string latlng_from, string latlng_to){	
	/*
		Calculates the distance from one latlng coord to another latlon
		via the haversine formula.

		example:
			inp("51.50,0", "-22.97,-43.18") : outp = 219.35
	*/

	int R = 6371000; // Radius of the Earth in meters

	vector<string> results_from;
	boost::split(results_from, latlng_from, [](char c){return c == ',';});
	vector<string> results_to;
	boost::split(results_to, latlng_to, [](char c){return c == ',';});

	float from_lat = stof(results_from[0]);
	float to_lat = stof(results_to[0]);
	float from_lng = stof(results_from[1]);
	float to_lng = stof(results_to[1]);

	float diff_lat = (to_lat - from_lat) * DEG_TO_RAD;
	float diff_lng = (to_lng - from_lng) * DEG_TO_RAD;

	float from_cos = cos(from_lat * DEG_TO_RAD);
	float to_cos = cos(to_lat * DEG_TO_RAD);
	
	float a = (sin(diff_lat/2) * sin(diff_lat/2)) + (from_cos * to_cos * sin(diff_lng/2)*sin(diff_lng/2));
	float c = 2 * atan2(sqrt(a),sqrt(1-a));

	return  R * c;
}

string get_gprmc(){
	/*
	 * Read 72 bytes from gps uart device, reads it into buffer.
	 *
	 * read on the internet that GPRMC should only 
	 * be of length 72 bytes, NOT CONFIRMED.
	 */

	int bus = UART_GPS_PORT;

	string startsWith = "$GPRMC";
	string gprmc;
	char buffer[1024];
	vector<string> results;
	string temp_string;
	
	int dot_position;
	float lat_minutes;
	float lng_minutes;
	float lat_degrees;
	float lng_degrees;

	float lat, lng;

	while(true){
		rc_uart_read_bytes(bus, buffer, 72);
		gprmc = buffer;
		boost::split(results, gprmc, [](char c){return c == ',';});

		if(results[0] == startsWith){
			if(results[2] != "A"){
				gprmc = "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62";
				results.clear();
				boost::split(results, gprmc, [](char c){return c == ',';});
				//continue;
			}

			dot_position = results[3].find('.');
			temp_string = results[3].substr(0,dot_position-2);
			lat_degrees = stof(temp_string);
			temp_string = results[3].substr(dot_position-2);
			lat_minutes = stof(temp_string);

			dot_position = results[5].find('.');
			temp_string = results[5].substr(0,dot_position-2);
			lng_degrees = stof(temp_string);
			temp_string = results[5].substr(dot_position-2);
			lng_minutes = stof(temp_string);

			lat = lat_degrees+(lat_minutes/60);
			lng = lng_degrees+(lng_minutes/60);

			if(results[4] == "S"){
				lat *= -1;
			}
			if(results[6] == "W"){
				lng *= -1;
			}
			temp_string = to_string(lat)+","+to_string(lng)+","+results[8];
			break;
		}
		rc_uart_flush(bus);
	}
	rc_uart_flush(bus);
	return temp_string;
}

int reset_servo(const int ch){
	/*
	 * move servo on channel ch to 0.
	 */

	for(int i = 0; i < 100; i++){
		if(rc_servo_send_pulse_normalized(ch, 0) == -1){
			printf("failed\n");
		       	return -1;
		}
		rc_usleep(500000/50);
	}
	return 0;
}
