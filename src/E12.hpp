#pragma once

#include <coco/Unit.hpp>


using namespace coco;

extern const int e12Table[12];


template <typename T, int P>
struct E12 {
	T value;

	int get() const {
		int x = this->value;
		int e = e12Table[x % 12];
		int power = x / 12;
		for (int i = 0; i < power; ++i)
			e *= 10;
		return e;
	}
};

/**
 * Convert a value to the 12 series, e.g. 10 -> 0, 100 -> 12, 1000 -> 24
 * @param value
 * @return value in E12 series
 */
int toE12(int value);


// logarithmic duration: 10ms, 12ms, 15ms ...
using MillisecondsE12 = E12<uint8_t, -3>;

// logarithmic percentage: 1%, 1.2%, 1.5% ... 100%
using PercentageE12 = E12<uint8_t, -1>;


// stream operator
template <typename S, typename T, int P>
S &operator <<(S &s, E12<T, P> e12) {
	int x = e12.value;
	int e = e12Table[x % 12];
	int power = x / 12 + P;

	char ch1 = e / 10 + '0';
	char ch2 = e % 10 + '0';

	if (power < -1) {
		s << "0.";
		for (int i = 2; i < -power; ++i)
			s << '0';
	}
	s << ch1;
	if (power == -1)
		s << '.';
	s << ch2;

	for (int i = 0; i < power; ++i)
		s << '0';
	return s;
}


/*
template <int P, int U1>
struct E12 {
	int value;

	operator int() const {
		int x = this->value;
		int e = e12Table[x % 12];
		int power = x / 12;
		for (int i = 0; i < power; ++i)
			e *= 10;
		return e * pow10<-P>() / pow10<-P>();
	}

	operator Unit<int, P, U1>() const {
		int x = this->value;
		int e = e12Table[x % 12];
		int power = x / 12;
		for (int i = 0; i < power; ++i)
			e *= 10;
		return Unit<int, P, U1>(e);
	}

	template <int P2>
	static constexpr int pow10() {
		int p = 1;
		for (int i = 0; i < P2; ++i)
			p *= 10;
		return p;
	}
};


template <int P, int U1>
E12<P, U1> e12(int value) {
	return {value};
}

// logarithmic duration: 10ms, 12ms, 15ms ...
using E12Milliseconds = E12<-3, 1>;

// logarithmic brightness: 1%, 1.2% ... 100%
using E12Brightness = E12<-1, 0>;
*/