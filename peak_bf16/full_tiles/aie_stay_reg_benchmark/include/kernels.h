#pragma once

#include <adf.h>
#include <cstdint>

void bf16_full_tile_first(input_stream_uint32* control,
                          output_stream_uint32* out);
void bf16_full_tile_middle(input_stream_uint32* in,
                           output_stream_uint32* out);
void bf16_full_tile_last(input_stream_uint32* in,
                         output_stream_uint32* out);
