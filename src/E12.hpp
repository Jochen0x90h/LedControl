#pragma once

#include <coco/Unit.hpp>


using namespace coco;

extern const int e12Table[12];

template <int P, int U1>
struct E12 {
	int value;

	/*operator int() const {
		int x = this->value;
		int e = e12Table[x % 12];
		int power = x / 12;
		for (int i = 0; i < power; ++i)
			e *= 10;
		return e;
	}*/

	operator Unit<int, P, U1>() const {
		int x = this->value;
		int e = e12Table[x % 12];
		int power = x / 12;
		for (int i = 0; i < power; ++i)
			e *= 10;
		return Unit<int, P, U1>(e);
	}
};

template <typename S, int P, int U1>
S &operator <<(S &s, E12<P, U1> e12) {
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

template <int P, int U1>
E12<P, U1> e12(int value) {
	return {value};
}


using E12Milliseconds = E12<-3, 1>;
