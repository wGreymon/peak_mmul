#pragma once

#include <adf.h>
#include <cstdint>

void int8_stay_reg_kernel(adf::input_buffer<int8_t>& matA,
                          adf::input_buffer<int8_t>& matB,
                          adf::output_buffer<uint8_t>& matC);
