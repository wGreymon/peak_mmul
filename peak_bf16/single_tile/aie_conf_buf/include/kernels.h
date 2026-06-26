#pragma once

#include <adf.h>
#include <cstdint>

void bf16_local_buffer_kernel(adf::input_buffer<bfloat16>& __restrict matA,
                              adf::input_buffer<bfloat16>& __restrict matB,
                              adf::output_buffer<bfloat16>& __restrict matC);
