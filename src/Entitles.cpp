#include "Entities.hpp"

namespace ticket {

UserRecord::UserRecord() : username(), password(), name(), mail(), privilege(0) {}

TrainRecord::TrainRecord()
    : train_id(), station_num(0), seat_num(0), start_time(), sale_begin(), sale_end(), type(' '), released(false) {
}

OrderRecord::OrderRecord()
    : order_id(0), username(), train_id(), date(), from_idx(0), to_idx(0), num(0),
    price(0), status(OrderStatus::Success), is_waiting(false), timestamp(0) {}

WaitlistRecord::WaitlistRecord()
	: username(), train_id(), date(), from_idx(0), to_idx(0), num(0), timestamp(0) {}

} // namespace ticket

