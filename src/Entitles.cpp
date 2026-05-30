#include "../include/Entities.hpp"

namespace ticket {

UserRecord::UserRecord() : username(), password(), name(), mail(), privilege(0) {}

TrainRecord::TrainRecord()
	: train_id(), station_num(0), seat_num(0), start_time(), sale_begin(), sale_end(), type(' '), released(false) {
	// initialize arrays
	for (int i = 0; i < MAX_STATIONS; ++i) stations[i].clear();
	for (int i = 0; i < MAX_PRICE_SEGMENTS; ++i) prices[i] = 0;
	for (int i = 0; i < MAX_TRAVEL_SEGMENTS; ++i) travel_times[i] = stopover_times[i] = 0;
}

OrderRecord::OrderRecord()
    : order_id(0), username(), train_id(), date(), from_idx(0), to_idx(0), num(0), price(0), status(OrderStatus::Success), is_waiting(false), timestamp(0) {}

WaitlistRecord::WaitlistRecord()
	: username(), train_id(), date(), from_idx(0), to_idx(0), num(0), timestamp(0) {}

} // namespace ticket

