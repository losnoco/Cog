#pragma once

#include "HrtfTypes.h"
#include "IHrtfData.h"
#include <cstdint>
#include <iostream>
#include <vector>

struct ElevationData {
	std::vector<DirectionData> azimuths;
};

struct DistanceData {
	distance_t distance;
	std::vector<ElevationData> elevations;
};

class HrtfData : public IHrtfData {
	void LoadHrtf00(std::istream& stream);
	void LoadHrtf01(std::istream& stream);
	void LoadHrtf02(std::istream& stream);
	void LoadHrtf03(std::istream& stream);

	public:
	HrtfData(std::istream& stream);

	void get_direction_data(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t channel, DirectionData& ref_data) const override;
	void get_direction_data(angle_t elevation, angle_t azimuth, distance_t distance, DirectionData& ref_data_left, DirectionData& ref_data_right) const override;
	void sample_direction(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t sample, uint32_t channel, float& value, float& delay) const override;
	void sample_direction(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t sample, float& value_left, float& delay_left, float& value_right, float& delay_right) const override;

	uint32_t get_sample_rate() const override {
		return m_sample_rate;
	}
	uint32_t get_response_length() const override {
		return m_response_length;
	}
	uint32_t get_longest_delay() const override {
		return m_longest_delay;
	}

	private:
	uint32_t m_sample_rate;
	uint32_t m_response_length;
	uint32_t m_longest_delay;
	uint32_t m_channel_count;
	std::vector<DistanceData> m_distances;
};
