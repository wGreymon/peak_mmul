#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>

#include "peak_bf16_config.h"

namespace {

using MMUL = aie::mmul<peak_bf16::MmulM,
                       peak_bf16::MmulK,
                       peak_bf16::MmulN,
                       bfloat16,
                       bfloat16,
                       accauto>;

static_assert(MMUL::size_A == peak_bf16::MmulM * peak_bf16::MmulK);
static_assert(MMUL::size_B == peak_bf16::MmulK * peak_bf16::MmulN);
static_assert(MMUL::size_C == peak_bf16::MmulM * peak_bf16::MmulN);

inline void load_blocks(const bfloat16* __restrict pA0,
                        const bfloat16* __restrict pA1,
                        const bfloat16* __restrict pB0,
                        const bfloat16* __restrict pB1,
                        aie::vector<bfloat16, MMUL::size_A>& A0,
                        aie::vector<bfloat16, MMUL::size_A>& A1,
                        aie::vector<bfloat16, MMUL::size_B>& B0,
                        aie::vector<bfloat16, MMUL::size_B>& B1) {
    A0 = aie::load_v<MMUL::size_A>(pA0);
    A1 = aie::load_v<MMUL::size_A>(pA1);
    B0 = aie::load_v<MMUL::size_B>(pB0);
    B1 = aie::load_v<MMUL::size_B>(pB1);
}

}  // namespace

void bf16_pack_first(adf::input_circular_buffer<bfloat16>& __restrict matA,
                     adf::input_circular_buffer<bfloat16>& __restrict matB,
                     output_cascade<accfloat>* ps_out) {
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);

    const bfloat16* __restrict pA = matA.data();
    const bfloat16* __restrict pB = matB.data();

    for (int i = 0; i < peak_bf16::NumGroupM; i += 2)
        chess_prepare_for_pipelining {
            for (int j = 0; j < peak_bf16::NumGroupN; j += 2)
                chess_prepare_for_pipelining {
                    const bfloat16* __restrict pA0 =
                        pA + (i * peak_bf16::NumGroupK) * MMUL::size_A;
                    const bfloat16* __restrict pA1 =
                        pA + ((i + 1) * peak_bf16::NumGroupK) * MMUL::size_A;
                    const bfloat16* __restrict pB0 = pB + j * MMUL::size_B;
                    const bfloat16* __restrict pB1 = pB + (j + 1) * MMUL::size_B;

                    aie::vector<bfloat16, MMUL::size_A> A0;
                    aie::vector<bfloat16, MMUL::size_A> A1;
                    aie::vector<bfloat16, MMUL::size_B> B0;
                    aie::vector<bfloat16, MMUL::size_B> B1;

                    load_blocks(pA0, pA1, pB0, pB1, A0, A1, B0, B1);
                    pA0 += MMUL::size_A;
                    pA1 += MMUL::size_A;
                    pB0 += MMUL::size_B * peak_bf16::NumGroupN;
                    pB1 += MMUL::size_B * peak_bf16::NumGroupN;

                    MMUL C00;
                    MMUL C01;
                    MMUL C10;
                    MMUL C11;

                    C00.mul(A0, B0);
                    C01.mul(A0, B1);
                    C10.mul(A1, B0);
                    C11.mul(A1, B1);

                    for (int k = 0; k < peak_bf16::NumGroupK - 1; ++k)
                        chess_prepare_for_pipelining {
                            load_blocks(pA0, pA1, pB0, pB1, A0, A1, B0, B1);
                            pA0 += MMUL::size_A;
                            pA1 += MMUL::size_A;
                            pB0 += MMUL::size_B * peak_bf16::NumGroupN;
                            pB1 += MMUL::size_B * peak_bf16::NumGroupN;

                            C00.mac(A0, B0);
                            C01.mac(A0, B1);
                            C10.mac(A1, B0);
                            C11.mac(A1, B1);
                        }

                    *ps_out << C00.to_accum();
                    *ps_out << C01.to_accum();
                    *ps_out << C10.to_accum();
                    *ps_out << C11.to_accum();
                }
        }
}
