#pragma once

#include "HrtfTypes.h"

class IHrtfData {
	public:
	virtual ~IHrtfData() = default;

	virtual void get_direction_data(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t channel, DirectionData& ref_data) const = 0;
	virtual void get_direction_data(angle_t elevation, angle_t azimuth, distance_t distance, DirectionData& ref_data_left, DirectionData& ref_data_right) const = 0;
	// Get only once IR sample at given direction. The delay returned is the delay of IR's beginning, not the sample's!
	virtual void sample_direction(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t sample, uint32_t channel, float& value, float& delay) const = 0;
	// Get only once IR sample at given direction for both channels. The delay returned is the delay of IR's beginning, not the sample's!
	virtual void sample_direction(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t sample, float& value_left, float& delay_left, float& value_right, float& delay_right) const = 0;

	virtual uint32_t get_sample_rate() const = 0;
	virtual uint32_t get_response_length() const = 0;
	virtual uint32_t get_longest_delay() const = 0;
};
