#pragma once

#include <cstdint>
#include <vector>

typedef float distance_t;
typedef float angle_t;
typedef int delay_t;

struct DirectionData {
	std::vector<float> impulse_response;
	delay_t delay;
	delay_t delay_right;
};
