#pragma once

#include <adf.h>

void bf16_pack_first(adf::input_circular_buffer<bfloat16>& matA,
                     adf::input_circular_buffer<bfloat16>& matB,
                     output_cascade<accfloat>* ps_out);

void bf16_pack_middle(adf::input_circular_buffer<bfloat16>& matA,
                      adf::input_circular_buffer<bfloat16>& matB,
                      input_cascade<accfloat>* ps_in,
                      output_cascade<accfloat>* ps_out);

void bf16_pack_last(adf::input_circular_buffer<bfloat16>& matA,
                    adf::input_circular_buffer<bfloat16>& matB,
                    input_cascade<accfloat>* ps_in,
                    adf::output_buffer<bfloat16>& matC);
