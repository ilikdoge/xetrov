#pragma once
#include "xe/common.h"

namespace xetrov{

template<size_t N, typename T>
constexpr size_t xe_array_size(T (&)[N]){
	return N;
}

static constexpr uint xe_log2(ulong value){
	return value ? sizeof(ulong) * 8 - __builtin_clzl(value) - 1 : 0;
}

}