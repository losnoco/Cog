#pragma once

// The functions provide little endianness to native endianness conversion and back again
#if(defined(_MSC_VER) && defined(_WIN32)) || defined(__APPLE__)
template <typename T>
inline void from_little_endian_inplace(T& x) {
}

template <typename T>
inline T from_little_endian(T x) {
	return x;
}

template <typename T>
inline void to_little_endian_inplace(T& x) {
}

template <typename T>
inline T to_little_endian(T x) {
	return x;
}

#else
#error "Specify endianness conversion for your platform"
#endif
