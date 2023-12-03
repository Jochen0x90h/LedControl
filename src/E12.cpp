
extern const int e12Table[12] = {10, 12, 15, 18, 22, 27, 33, 39, 47, 56, 68, 82};

int toE12(int value) {
	int e = 0;
    int v1 = -1000000;
	for (int power = 1; power <= 1000000; power *= 10) {
		for (int i = 0; i < 12; ++i) {
			int v2 = e12Table[i] * power;
			if (v2 >= value) {
				int d1 = value - v1;
				int d2 = v2 - value;
				return d1 <= d2 ? e - 1 : e;
			}
			v1 = v2;
            ++e;
		}
	}
    return 60;
}
