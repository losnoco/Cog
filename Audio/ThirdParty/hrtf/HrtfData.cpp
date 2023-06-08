
#include "HrtfData.h"
#include "Endianness.h"
#include <algorithm>
#include <cassert>
#include <cmath>

typedef struct {
	uint8_t bytes[3];
} sample_int24_t;

const double pi = 3.1415926535897932385;

template <typename T>
void read_stream(std::istream& stream, T& value) {
	stream.read(reinterpret_cast<std::istream::char_type*>(&value), sizeof(value));
	from_little_endian_inplace(value);
}

HrtfData::HrtfData(std::istream& stream) {
	const char required_magic00[] = { 'M', 'i', 'n', 'P', 'H', 'R', '0', '0' };
	const char required_magic01[] = { 'M', 'i', 'n', 'P', 'H', 'R', '0', '1' };
	const char required_magic02[] = { 'M', 'i', 'n', 'P', 'H', 'R', '0', '2' };
	const char required_magic03[] = { 'M', 'i', 'n', 'P', 'H', 'R', '0', '3' };
	char actual_magic[sizeof(required_magic03) / sizeof(required_magic03[0])];

	stream.read(actual_magic, sizeof(actual_magic));
	if(std::equal(std::begin(required_magic03), std::end(required_magic03), std::begin(actual_magic), std::end(actual_magic))) {
		LoadHrtf03(stream);
	} else if(std::equal(std::begin(required_magic02), std::end(required_magic02), std::begin(actual_magic), std::end(actual_magic))) {
		LoadHrtf02(stream);
	} else if(std::equal(std::begin(required_magic01), std::end(required_magic01), std::begin(actual_magic), std::end(actual_magic))) {
		LoadHrtf01(stream);
	} else if(std::equal(std::begin(required_magic00), std::end(required_magic00), std::begin(actual_magic), std::end(actual_magic))) {
		LoadHrtf00(stream);
	} else {
		throw std::logic_error("Bad file format.");
	}
}

void HrtfData::LoadHrtf03(std::istream& stream) {
	// const uint8_t ChanType_LeftOnly{0};
	const uint8_t ChanType_LeftRight{ 1 };

	uint32_t sample_rate;
	uint8_t channel_type;
	uint8_t impulse_response_length;
	uint8_t distances_count;

	read_stream(stream, sample_rate);
	read_stream(stream, channel_type);
	read_stream(stream, impulse_response_length);
	read_stream(stream, distances_count);

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	if(channel_type > ChanType_LeftRight) {
		throw std::logic_error("Invalid channel format.");
	}

	int channel_count = channel_type == ChanType_LeftRight ? 2 : 1;

	std::vector<DistanceData> distances(distances_count);

	for(uint8_t i = 0; i < distances_count; i++) {
		uint16_t distance;
		read_stream(stream, distance);
		distances[i].distance = float(distance) / 1000.0f;

		uint8_t elevations_count;
		read_stream(stream, elevations_count);
		distances[i].elevations.resize(elevations_count);

		if(!stream || stream.eof()) {
			throw std::logic_error("Failed reading file.");
		}

		for(uint8_t j = 0; j < elevations_count; j++) {
			uint8_t azimuth_count;
			read_stream(stream, azimuth_count);
			distances[i].elevations[j].azimuths.resize(azimuth_count);
		}

		if(!stream || stream.eof()) {
			throw std::logic_error("Failed reading file.");
		}
	}

	const float normalization_factor = 1.0f / 8388608.0f;

	for(auto& distance : distances) {
		for(auto& elevation : distance.elevations) {
			for(auto& azimuth : elevation.azimuths) {
				azimuth.impulse_response.resize(impulse_response_length * channel_count);
				for(auto& sample : azimuth.impulse_response) {
					union {
						sample_int24_t sample;
						int32_t sample_int;
					} sample_union;
					sample_union.sample_int = 0;
					read_stream(stream, sample_union.sample);
					sample_union.sample_int <<= 8;
					sample_union.sample_int >>= 8;
					sample = sample_union.sample_int * normalization_factor;
				}
			}
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	uint8_t longest_delay = 0;
	for(auto& distance : distances) {
		for(auto& elevation : distance.elevations) {
			for(auto& azimuth : elevation.azimuths) {
				uint8_t delay;
				read_stream(stream, delay);
				azimuth.delay = delay;
				longest_delay = std::max(longest_delay, delay);
				if(channel_type == ChanType_LeftRight) {
					read_stream(stream, delay);
					azimuth.delay_right = delay;
					longest_delay = std::max(longest_delay, delay);
				}
			}
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	std::sort(distances.begin(), distances.end(),
	          [](const DistanceData& lhs, const DistanceData& rhs) noexcept { return lhs.distance > rhs.distance; });

	m_distances = std::move(distances);
	m_channel_count = channel_count;
	m_response_length = impulse_response_length;
	m_sample_rate = sample_rate;
	m_longest_delay = longest_delay;
}

void HrtfData::LoadHrtf02(std::istream& stream) {
	// const uint8_t SampleType_S16{0};
	const uint8_t SampleType_S24{ 1 };
	// const uint8_t ChanType_LeftOnly{0};
	const uint8_t ChanType_LeftRight{ 1 };

	uint32_t sample_rate;
	uint8_t sample_type;
	uint8_t channel_type;
	uint8_t impulse_response_length;
	uint8_t distances_count;

	read_stream(stream, sample_rate);
	read_stream(stream, sample_type);
	read_stream(stream, channel_type);
	read_stream(stream, impulse_response_length);
	read_stream(stream, distances_count);

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	if(sample_type > SampleType_S24) {
		throw std::logic_error("Invalid sample type.");
	}

	if(channel_type > ChanType_LeftRight) {
		throw std::logic_error("Invalid channel format.");
	}

	int channel_count = channel_type == ChanType_LeftRight ? 2 : 1;

	std::vector<DistanceData> distances(distances_count);

	for(uint8_t i = 0; i < distances_count; i++) {
		uint16_t distance;
		read_stream(stream, distance);
		distances[i].distance = float(distance) / 1000.0f;

		uint8_t elevations_count;
		read_stream(stream, elevations_count);
		distances[i].elevations.resize(elevations_count);

		if(!stream || stream.eof()) {
			throw std::logic_error("Failed reading file.");
		}

		for(uint8_t j = 0; j < elevations_count; j++) {
			uint8_t azimuth_count;
			read_stream(stream, azimuth_count);
			distances[i].elevations[j].azimuths.resize(azimuth_count);
		}

		if(!stream || stream.eof()) {
			throw std::logic_error("Failed reading file.");
		}
	}

	const float normalization_factor = (sample_type == SampleType_S24) ? 1.0f / 8388608.0f : 1.0f / 32768.0f;

	for(auto& distance : distances) {
		for(auto& elevation : distance.elevations) {
			for(auto& azimuth : elevation.azimuths) {
				azimuth.impulse_response.resize(impulse_response_length * channel_count);
				if(sample_type == SampleType_S24) {
					for(auto& sample : azimuth.impulse_response) {
						union {
							sample_int24_t sample;
							int32_t sample_int;
						} sample_union;
						sample_union.sample_int = 0;
						read_stream(stream, sample_union.sample);
						sample_union.sample_int <<= 8;
						sample_union.sample_int >>= 8;
						sample = sample_union.sample_int * normalization_factor;
					}
				} else {
					for(auto& sample : azimuth.impulse_response) {
						int16_t sample_from_file;
						read_stream(stream, sample_from_file);
						sample = sample_from_file * normalization_factor;
					}
				}
			}
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	uint8_t longest_delay = 0;
	for(auto& distance : distances) {
		for(auto& elevation : distance.elevations) {
			for(auto& azimuth : elevation.azimuths) {
				uint8_t delay;
				read_stream(stream, delay);
				azimuth.delay = delay;
				longest_delay = std::max(longest_delay, delay);
				if(channel_type == ChanType_LeftRight) {
					read_stream(stream, delay);
					azimuth.delay_right = delay;
					longest_delay = std::max(longest_delay, delay);
				}
			}
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	std::sort(distances.begin(), distances.end(),
	          [](const DistanceData& lhs, const DistanceData& rhs) noexcept { return lhs.distance > rhs.distance; });

	m_distances = std::move(distances);
	m_channel_count = channel_count;
	m_response_length = impulse_response_length;
	m_sample_rate = sample_rate;
	m_longest_delay = longest_delay;
}

void HrtfData::LoadHrtf01(std::istream& stream) {
	uint32_t sample_rate;
	uint8_t impulse_response_length;

	read_stream(stream, sample_rate);
	read_stream(stream, impulse_response_length);

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	std::vector<DistanceData> distances(1);

	distances[0].distance = 1.0;

	uint8_t elevations_count;
	read_stream(stream, elevations_count);
	distances[0].elevations.resize(elevations_count);

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	for(uint8_t i = 0; i < elevations_count; i++) {
		uint8_t azimuth_count;
		read_stream(stream, azimuth_count);
		distances[0].elevations[i].azimuths.resize(azimuth_count);
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	const float normalization_factor = 1.0f / 32768.0f;

	for(auto& elevation : distances[0].elevations) {
		for(auto& azimuth : elevation.azimuths) {
			azimuth.impulse_response.resize(impulse_response_length);
			for(auto& sample : azimuth.impulse_response) {
				int16_t sample_from_file;
				read_stream(stream, sample_from_file);
				sample = sample_from_file * normalization_factor;
			}
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	uint8_t longest_delay = 0;
	for(auto& elevation : distances[0].elevations) {
		for(auto& azimuth : elevation.azimuths) {
			uint8_t delay;
			read_stream(stream, delay);
			delay <<= 2;
			azimuth.delay = delay;
			longest_delay = std::max(longest_delay, delay);
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	m_distances = std::move(distances);
	m_channel_count = 1;
	m_response_length = impulse_response_length;
	m_sample_rate = sample_rate;
	m_longest_delay = longest_delay;
}

void HrtfData::LoadHrtf00(std::istream& stream) {
	uint32_t sample_rate;
	uint16_t impulse_response_count;
	uint16_t impulse_response_length;

	read_stream(stream, sample_rate);
	read_stream(stream, impulse_response_count);
	read_stream(stream, impulse_response_length);

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	std::vector<DistanceData> distances(1);

	distances[0].distance = 1.0;

	uint8_t elevations_count;
	read_stream(stream, elevations_count);
	distances[0].elevations.resize(elevations_count);

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	std::vector<uint16_t> irOffsets(elevations_count);

	for(uint8_t i = 0; i < elevations_count; i++) {
		read_stream(stream, irOffsets[i]);
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	for(size_t i = 1; i < elevations_count; i++) {
		if(irOffsets[i] <= irOffsets[i - 1]) {
			throw std::logic_error("Invalid elevation offset.");
		}
	}
	if(impulse_response_count <= irOffsets[elevations_count - 1]) {
		throw std::logic_error("Invalid elevation offset.");
	}

	for(size_t i = 1; i < elevations_count; i++) {
		distances[0].elevations[i - 1].azimuths.resize(irOffsets[i] - irOffsets[i - 1]);
	}
	distances[0].elevations[elevations_count - 1].azimuths.resize(impulse_response_count - irOffsets[elevations_count - 1]);

	const float normalization_factor = 1.0f / 32768.0f;

	for(auto& elevation : distances[0].elevations) {
		for(auto& azimuth : elevation.azimuths) {
			azimuth.impulse_response.resize(impulse_response_length);
			for(auto& sample : azimuth.impulse_response) {
				int16_t sample_from_file;
				read_stream(stream, sample_from_file);
				sample = sample_from_file * normalization_factor;
			}
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	uint8_t longest_delay = 0;
	for(auto& elevation : distances[0].elevations) {
		for(auto& azimuth : elevation.azimuths) {
			uint8_t delay;
			read_stream(stream, delay);
			delay <<= 2;
			azimuth.delay = delay;
			longest_delay = std::max(longest_delay, delay);
		}
	}

	if(!stream || stream.eof()) {
		throw std::logic_error("Failed reading file.");
	}

	m_distances = std::move(distances);
	m_channel_count = 1;
	m_response_length = impulse_response_length;
	m_sample_rate = sample_rate;
	m_longest_delay = longest_delay;
}

void HrtfData::get_direction_data(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t channel, DirectionData& ref_data) const {
	assert(elevation >= -angle_t(pi * 0.5));
	assert(elevation <= angle_t(pi * 0.5));
	assert(azimuth >= -angle_t(2.0 * pi));
	assert(azimuth <= angle_t(2.0 * pi));

	const float azimuth_mod = std::fmod(azimuth + angle_t(pi * 2.0), angle_t(pi * 2.0));

	size_t distance_index0 = 0;
	while(distance_index0 < m_distances.size() - 1 &&
	      m_distances[distance_index0].distance > distance) {
		distance_index0++;
	}
	const size_t distance_index1 = std::min(distance_index0 + 1, m_distances.size() - 1);
	const distance_t distance0 = m_distances[distance_index0].distance;
	const distance_t distance1 = m_distances[distance_index1].distance;
	const distance_t distance_delta = distance0 - distance1;
	const float distance_fractional_part = distance_delta ? (distance - distance1) / distance_delta : 0;

	const auto& elevations0 = m_distances[distance_index0].elevations;
	const auto& elevations1 = m_distances[distance_index1].elevations;

	const angle_t elevation_scaled0 = (elevation + angle_t(pi * 0.5)) * (elevations0.size() - 1) / angle_t(pi);
	const angle_t elevation_scaled1 = (elevation + angle_t(pi * 0.5)) * (elevations1.size() - 1) / angle_t(pi);
	const size_t elevation_index00 = static_cast<size_t>(elevation_scaled0);
	const size_t elevation_index10 = static_cast<size_t>(elevation_scaled1);
	const size_t elevation_index01 = std::min(elevation_index00 + 1, elevations0.size() - 1);
	const size_t elevation_index11 = std::min(elevation_index10 + 1, elevations1.size() - 1);

	const float elevation_fractional_part0 = std::fmod(elevation_scaled0, 1.0);
	const float elevation_fractional_part1 = std::fmod(elevation_scaled1, 1.0);

	const angle_t azimuth_scaled00 = azimuth_mod * elevations0[elevation_index00].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index000 = static_cast<size_t>(azimuth_scaled00) % elevations0[elevation_index00].azimuths.size();
	const size_t azimuth_index001 = static_cast<size_t>(azimuth_scaled00 + 1) % elevations0[elevation_index00].azimuths.size();
	const float azimuth_fractional_part00 = std::fmod(azimuth_scaled00, 1.0);

	const angle_t azimuth_scaled10 = azimuth_mod * elevations1[elevation_index10].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index100 = static_cast<size_t>(azimuth_scaled10) % elevations1[elevation_index10].azimuths.size();
	const size_t azimuth_index101 = static_cast<size_t>(azimuth_scaled10 + 1) % elevations1[elevation_index10].azimuths.size();
	const float azimuth_fractional_part10 = std::fmod(azimuth_scaled10, 1.0);

	const angle_t azimuth_scaled01 = azimuth_mod * elevations0[elevation_index01].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index010 = static_cast<size_t>(azimuth_scaled01) % elevations0[elevation_index01].azimuths.size();
	const size_t azimuth_index011 = static_cast<size_t>(azimuth_scaled01 + 1) % elevations0[elevation_index01].azimuths.size();
	const float azimuth_fractional_part01 = std::fmod(azimuth_scaled01, 1.0);

	const angle_t azimuth_scaled11 = azimuth_mod * elevations1[elevation_index11].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index110 = static_cast<size_t>(azimuth_scaled11) % elevations1[elevation_index11].azimuths.size();
	const size_t azimuth_index111 = static_cast<size_t>(azimuth_scaled11 + 1) % elevations1[elevation_index11].azimuths.size();
	const float azimuth_fractional_part11 = std::fmod(azimuth_scaled11, 1.0);

	const float blend_factor_000 = (1.0f - elevation_fractional_part0) * (1.0f - azimuth_fractional_part00) * distance_fractional_part;
	const float blend_factor_001 = (1.0f - elevation_fractional_part0) * azimuth_fractional_part00 * distance_fractional_part;
	const float blend_factor_010 = elevation_fractional_part0 * (1.0f - azimuth_fractional_part01) * distance_fractional_part;
	const float blend_factor_011 = elevation_fractional_part0 * azimuth_fractional_part01 * distance_fractional_part;

	const float blend_factor_100 = (1.0f - elevation_fractional_part1) * (1.0f - azimuth_fractional_part10) * (1.0f - distance_fractional_part);
	const float blend_factor_101 = (1.0f - elevation_fractional_part1) * azimuth_fractional_part10 * (1.0f - distance_fractional_part);
	const float blend_factor_110 = elevation_fractional_part1 * (1.0f - azimuth_fractional_part11) * (1.0f - distance_fractional_part);
	const float blend_factor_111 = elevation_fractional_part1 * azimuth_fractional_part11 * (1.0f - distance_fractional_part);

	delay_t delay0;
	delay_t delay1;

	if(channel == 0) {
		delay0 =
		elevations0[elevation_index00].azimuths[azimuth_index000].delay * blend_factor_000 + elevations0[elevation_index00].azimuths[azimuth_index001].delay * blend_factor_001 + elevations0[elevation_index01].azimuths[azimuth_index010].delay * blend_factor_010 + elevations0[elevation_index01].azimuths[azimuth_index011].delay * blend_factor_011;

		delay1 =
		elevations1[elevation_index10].azimuths[azimuth_index100].delay * blend_factor_100 + elevations1[elevation_index10].azimuths[azimuth_index101].delay * blend_factor_101 + elevations1[elevation_index11].azimuths[azimuth_index110].delay * blend_factor_110 + elevations1[elevation_index11].azimuths[azimuth_index111].delay * blend_factor_111;
	} else {
		delay0 =
		elevations0[elevation_index00].azimuths[azimuth_index000].delay_right * blend_factor_000 + elevations0[elevation_index00].azimuths[azimuth_index001].delay_right * blend_factor_001 + elevations0[elevation_index01].azimuths[azimuth_index010].delay_right * blend_factor_010 + elevations0[elevation_index01].azimuths[azimuth_index011].delay_right * blend_factor_011;

		delay1 =
		elevations1[elevation_index10].azimuths[azimuth_index100].delay_right * blend_factor_100 + elevations1[elevation_index10].azimuths[azimuth_index101].delay_right * blend_factor_101 + elevations1[elevation_index11].azimuths[azimuth_index110].delay_right * blend_factor_110 + elevations1[elevation_index11].azimuths[azimuth_index111].delay_right * blend_factor_111;
	}

	ref_data.delay = delay0 + delay1;

	if(ref_data.impulse_response.size() < m_response_length)
		ref_data.impulse_response.resize(m_response_length);

	for(size_t i = 0, j = channel; i < m_response_length; i++, j += m_channel_count) {
		float sample0 =
		elevations0[elevation_index00].azimuths[azimuth_index000].impulse_response[j] * blend_factor_000 + elevations0[elevation_index00].azimuths[azimuth_index001].impulse_response[j] * blend_factor_001 + elevations0[elevation_index01].azimuths[azimuth_index010].impulse_response[j] * blend_factor_010 + elevations0[elevation_index01].azimuths[azimuth_index011].impulse_response[j] * blend_factor_011;
		float sample1 =
		elevations1[elevation_index10].azimuths[azimuth_index100].impulse_response[j] * blend_factor_100 + elevations1[elevation_index10].azimuths[azimuth_index101].impulse_response[j] * blend_factor_101 + elevations1[elevation_index11].azimuths[azimuth_index110].impulse_response[j] * blend_factor_110 + elevations1[elevation_index11].azimuths[azimuth_index111].impulse_response[j] * blend_factor_111;

		ref_data.impulse_response[i] = sample0 + sample1;
	}
}

void HrtfData::get_direction_data(angle_t elevation, angle_t azimuth, distance_t distance, DirectionData& ref_data_left, DirectionData& ref_data_right) const {
	assert(elevation >= -angle_t(pi * 0.5));
	assert(elevation <= angle_t(pi * 0.5));
	assert(azimuth >= -angle_t(2.0 * pi));
	assert(azimuth <= angle_t(2.0 * pi));

	get_direction_data(elevation, azimuth, distance, 0, ref_data_left);
	if(m_channel_count == 1) {
		get_direction_data(elevation, -azimuth, distance, 0, ref_data_right);
	} else {
		get_direction_data(elevation, azimuth, distance, 1, ref_data_right);
	}
}

void HrtfData::sample_direction(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t sample, uint32_t channel, float& value, float& delay) const {
	assert(elevation >= -angle_t(pi * 0.5));
	assert(elevation <= angle_t(pi * 0.5));
	assert(azimuth >= -angle_t(2.0 * pi));
	assert(azimuth <= angle_t(2.0 * pi));

	size_t distance_index0 = 0;
	while(distance_index0 < m_distances.size() - 1 &&
	      m_distances[distance_index0].distance > distance) {
		distance_index0++;
	}
	const size_t distance_index1 = std::min(distance_index0 + 1, m_distances.size() - 1);
	const distance_t distance0 = m_distances[distance_index0].distance;
	const distance_t distance1 = m_distances[distance_index1].distance;
	const distance_t distance_delta = distance0 - distance1;
	const float distance_fractional_part = distance_delta ? (distance - distance1) / distance_delta : 0;

	const auto& elevations0 = m_distances[distance_index0].elevations;
	const auto& elevations1 = m_distances[distance_index1].elevations;

	const float azimuth_mod = std::fmod(azimuth + angle_t(pi * 2.0), angle_t(pi * 2.0));

	const angle_t elevation_scaled0 = (elevation + angle_t(pi * 0.5)) * (elevations0.size() - 1) / angle_t(pi);
	const angle_t elevation_scaled1 = (elevation + angle_t(pi * 0.5)) * (elevations1.size() - 1) / angle_t(pi);
	const size_t elevation_index00 = static_cast<size_t>(elevation_scaled0);
	const size_t elevation_index10 = static_cast<size_t>(elevation_scaled1);
	const size_t elevation_index01 = std::min(elevation_index00 + 1, elevations0.size() - 1);
	const size_t elevation_index11 = std::min(elevation_index10 + 1, elevations1.size() - 1);

	const float elevation_fractional_part0 = std::fmod(elevation_scaled0, 1.0);
	const float elevation_fractional_part1 = std::fmod(elevation_scaled1, 1.0);

	const angle_t azimuth_scaled00 = azimuth_mod * elevations0[elevation_index00].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index000 = static_cast<size_t>(azimuth_scaled00) % elevations0[elevation_index00].azimuths.size();
	const size_t azimuth_index001 = static_cast<size_t>(azimuth_scaled00 + 1) % elevations0[elevation_index00].azimuths.size();
	const float azimuth_fractional_part00 = std::fmod(azimuth_scaled00, 1.0);

	const angle_t azimuth_scaled10 = azimuth_mod * elevations1[elevation_index10].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index100 = static_cast<size_t>(azimuth_scaled10) % elevations1[elevation_index10].azimuths.size();
	const size_t azimuth_index101 = static_cast<size_t>(azimuth_scaled10 + 1) % elevations1[elevation_index10].azimuths.size();
	const float azimuth_fractional_part10 = std::fmod(azimuth_scaled10, 1.0);

	const angle_t azimuth_scaled01 = azimuth_mod * elevations0[elevation_index01].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index010 = static_cast<size_t>(azimuth_scaled01) % elevations0[elevation_index01].azimuths.size();
	const size_t azimuth_index011 = static_cast<size_t>(azimuth_scaled01 + 1) % elevations0[elevation_index01].azimuths.size();
	const float azimuth_fractional_part01 = std::fmod(azimuth_scaled01, 1.0);

	const angle_t azimuth_scaled11 = azimuth_mod * elevations1[elevation_index11].azimuths.size() / angle_t(2 * pi);
	const size_t azimuth_index110 = static_cast<size_t>(azimuth_scaled11) % elevations1[elevation_index11].azimuths.size();
	const size_t azimuth_index111 = static_cast<size_t>(azimuth_scaled11 + 1) % elevations1[elevation_index11].azimuths.size();
	const float azimuth_fractional_part11 = std::fmod(azimuth_scaled11, 1.0);

	const float blend_factor_000 = (1.0f - elevation_fractional_part0) * (1.0f - azimuth_fractional_part00) * distance_fractional_part;
	const float blend_factor_001 = (1.0f - elevation_fractional_part0) * azimuth_fractional_part00 * distance_fractional_part;
	const float blend_factor_010 = elevation_fractional_part0 * (1.0f - azimuth_fractional_part01) * distance_fractional_part;
	const float blend_factor_011 = elevation_fractional_part0 * azimuth_fractional_part01 * distance_fractional_part;

	const float blend_factor_100 = (1.0f - elevation_fractional_part1) * (1.0f - azimuth_fractional_part10) * (1.0f - distance_fractional_part);
	const float blend_factor_101 = (1.0f - elevation_fractional_part1) * azimuth_fractional_part10 * (1.0f - distance_fractional_part);
	const float blend_factor_110 = elevation_fractional_part1 * (1.0f - azimuth_fractional_part11) * (1.0f - distance_fractional_part);
	const float blend_factor_111 = elevation_fractional_part1 * azimuth_fractional_part11 * (1.0f - distance_fractional_part);

	float delay0;
	float delay1;

	if(channel == 0) {
		delay0 =
		elevations0[elevation_index00].azimuths[azimuth_index000].delay * blend_factor_000 + elevations0[elevation_index00].azimuths[azimuth_index001].delay * blend_factor_001 + elevations0[elevation_index01].azimuths[azimuth_index010].delay * blend_factor_010 + elevations0[elevation_index01].azimuths[azimuth_index011].delay * blend_factor_011;

		delay1 =
		elevations1[elevation_index10].azimuths[azimuth_index100].delay * blend_factor_100 + elevations1[elevation_index10].azimuths[azimuth_index101].delay * blend_factor_101 + elevations1[elevation_index11].azimuths[azimuth_index110].delay * blend_factor_110 + elevations1[elevation_index11].azimuths[azimuth_index111].delay * blend_factor_111;
	} else {
		delay0 =
		elevations0[elevation_index00].azimuths[azimuth_index000].delay_right * blend_factor_000 + elevations0[elevation_index00].azimuths[azimuth_index001].delay_right * blend_factor_001 + elevations0[elevation_index01].azimuths[azimuth_index010].delay_right * blend_factor_010 + elevations0[elevation_index01].azimuths[azimuth_index011].delay_right * blend_factor_011;

		delay1 =
		elevations1[elevation_index10].azimuths[azimuth_index100].delay_right * blend_factor_100 + elevations1[elevation_index10].azimuths[azimuth_index101].delay_right * blend_factor_101 + elevations1[elevation_index11].azimuths[azimuth_index110].delay_right * blend_factor_110 + elevations1[elevation_index11].azimuths[azimuth_index111].delay_right * blend_factor_111;
	}

	delay = delay0 + delay1;

	sample = sample * m_channel_count + channel;

	float value0 =
	elevations0[elevation_index00].azimuths[azimuth_index000].impulse_response[sample] * blend_factor_000 + elevations0[elevation_index00].azimuths[azimuth_index001].impulse_response[sample] * blend_factor_001 + elevations0[elevation_index01].azimuths[azimuth_index010].impulse_response[sample] * blend_factor_010 + elevations0[elevation_index01].azimuths[azimuth_index011].impulse_response[sample] * blend_factor_011;

	float value1 =
	elevations1[elevation_index10].azimuths[azimuth_index100].impulse_response[sample] * blend_factor_100 + elevations1[elevation_index10].azimuths[azimuth_index101].impulse_response[sample] * blend_factor_101 + elevations1[elevation_index11].azimuths[azimuth_index110].impulse_response[sample] * blend_factor_110 + elevations1[elevation_index11].azimuths[azimuth_index111].impulse_response[sample] * blend_factor_111;

	value = value0 + value1;
}

void HrtfData::sample_direction(angle_t elevation, angle_t azimuth, distance_t distance, uint32_t sample, float& value_left, float& delay_left, float& value_right, float& delay_right) const {
	assert(elevation >= -angle_t(pi * 0.5));
	assert(elevation <= angle_t(pi * 0.5));
	assert(azimuth >= -angle_t(2.0 * pi));
	assert(azimuth <= angle_t(2.0 * pi));

	sample_direction(elevation, azimuth, distance, sample, 0, value_left, delay_left);
	if(m_channel_count == 1) {
		sample_direction(elevation, -azimuth, distance, sample, 0, value_right, delay_right);
	} else {
		sample_direction(elevation, azimuth, distance, sample, 1, value_right, delay_right);
	}
}
