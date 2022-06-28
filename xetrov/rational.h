#pragma once
#include "types.h"
#include "xe/arch.h"

namespace xetrov{

template<typename T>
T xe_gcd(T a, T b){
	uint tz, tza, tzb;
	T tmp;

	if(!a)
		return b;
	if(!b)
		return a;
	tza = xe_arch_ctzl(a);
	a >>= tza;
	tzb = xe_arch_ctzl(b);
	b >>= tzb;
	tz = xe_min(tza, tzb);

	while(a != b){
		if(a > b){
			tmp = a;
			a = b;
			b = tmp;
		}

		b -= a;
		b >>= xe_arch_ctzl(b);
	}

	return a << tz;
}

struct xe_rational{
	uint num;
	uint den;

	xe_rational& operator=(uint d){
		num = 1;
		den = d;

		return *this;
	}

	xe_rational& reduce(){
		uint gcd = xe_gcd(num, den);

		if(gcd){
			num /= gcd;
			den /= gcd;
		}

		return *this;
	}

	ulong operator*(ulong value) const{
		return num * value / den;
	}

	double operator*(double value) const{
		return num * value / den;
	}
};

}