#pragma once

#include <adf.h>

void bf16_local_buffer_kernel(adf::input_buffer<bfloat16>& matA,
                              adf::input_buffer<bfloat16>& matB,
                              adf::output_buffer<bfloat16>& matC);
