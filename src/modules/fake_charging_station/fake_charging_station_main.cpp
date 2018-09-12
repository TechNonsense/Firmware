/***
 * This modules publishes messages from a fake charging station.
 * topics: charging_station_state, external_vehicle_position
 * Add `fake_charging_station start` to the launch script (e. g. init/ekf2/iris) for using.
 *
 * @author Oleg Kalachev <okalachev@gmail.com>
 */

#include <px4_config.h>
#include <px4_posix.h>
#include <px4_tasks.h>
#include <systemlib/mavlink_log.h>
#include <stdio.h>
#include <errno.h>
#include <drivers/drv_hrt.h>
#include <matrix/math.hpp>

// uORB Publications
#include <uORB/Publication.hpp>
#include <uORB/topics/charging_station_state.h>
#include <uORB/topics/external_vehicle_position.h>

#define CHARGING_STATION_ID 17
#define CHARGING_STATION_LAT 47.3976174
#define CHARGING_STATION_LON 8.5455089
#define CHARGING_STATION_ALT 487.0
#define CHARGING_STATION_HDG 90 // heading the East

charging_station_state_s state;

extern "C" __EXPORT int fake_charging_station_main(int argc, char *argv[]);

int fake_charging_station_thread_main(int argc, char *argv[]);

int fake_charging_station_thread_main(int argc, char *argv[]) {
	PX4_INFO("Starting fake charging station publisher");

	state = {};

	state.id = CHARGING_STATION_ID;
	state.base_mode = charging_station_state_s::BASE_MODE_FLAG_CUSTOM_MODE_ENABLED;
	state.system_status = 0;
	state.custom_mode = charging_station_state_s::CUSTOM_MODE_CLOSED;

	external_vehicle_position_s pos = {};

	pos.id = CHARGING_STATION_ID;
	pos.alt = CHARGING_STATION_ALT;
	pos.lat = CHARGING_STATION_LAT;
	pos.lon = CHARGING_STATION_LON;
	pos.yaw = matrix::wrap_pi(CHARGING_STATION_HDG * M_DEG_TO_RAD_F);
	pos.yaw_valid = true;

	orb_advert_t state_pub = orb_advertise(ORB_ID(charging_station_state), &state);
	orb_advert_t pos_pub = orb_advertise(ORB_ID(external_vehicle_position), &pos);

	while(true) {
		usleep(1000000);
		state.timestamp = pos.timestamp = hrt_absolute_time();
		orb_publish(ORB_ID(charging_station_state), state_pub, &state);
		orb_publish(ORB_ID(external_vehicle_position), pos_pub, &pos);
	}
}

int fake_charging_station_main(int argc, char *argv[]) {
	if (argc < 2) {
		PX4_INFO("usage: fake_charging_station {start|open|close|critical}");
		return 1;

	} else if (strcmp(argv[1], "start") == 0) {
		__attribute__((unused)) px4_task_t deamon_task = px4_task_spawn_cmd("fake_charging_station",
						SCHED_DEFAULT,
						1,
						200,
						fake_charging_station_thread_main,
						nullptr);

	} else if (strcmp(argv[1], "open") == 0) {
		state.custom_mode = charging_station_state_s::CUSTOM_MODE_OPEN;

	} else if (strcmp(argv[1], "close") == 0) {
		state.custom_mode = charging_station_state_s::CUSTOM_MODE_CLOSED;

	} else if (strcmp(argv[1], "critical") == 0) {
		state.system_status = charging_station_state_s::SYSTEM_STATUS_CRITICAL;
	}

	return OK;
}