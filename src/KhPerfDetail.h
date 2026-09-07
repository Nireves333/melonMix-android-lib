// [KHMM-DBG] fine split of the "emu" slice of the frame (everything inside nds->RunFrame()
// that is NOT the GL 3D render/composite, which g_khGlRenderNanos already covers). Buckets:
//   KH_EMU_2D    - software 2D scanline + sprite rendering (GPU::StartHBlank draw calls;
//                  always software even when 3D is GL)
//   KH_EMU_3DGEO - 3D geometry command execution (GPU3D::Run command loop) + the VBlank
//                  polygon partition/Y-sort
//   KH_EMU_SPU   - audio channel mixing (SPU::Mix)
// The remainder (emu minus these) is ~ARM9/ARM7 CPU (JIT) + DMA + scheduler + IO.
// Written on the emu thread, read+reset by MelonInstance::khReportPerf. Same pattern as
// g_khGlDetailNanos in GPU_OpenGL.h. Temporary scaffolding; grep [KHMM-DBG] to remove.

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

namespace melonDS
{

enum KhEmuDetail { KH_EMU_2D = 0, KH_EMU_3DGEO, KH_EMU_SPU, KH_EMU_DETAIL_COUNT };
extern std::atomic<uint64_t> g_khEmuDetailNanos[KH_EMU_DETAIL_COUNT];

// Scoped accumulator: adds the enclosing scope's wall time into one bucket on destruction.
struct KhEmuScopedNs
{
    std::atomic<uint64_t>& target;
    std::chrono::steady_clock::time_point t0;

    explicit KhEmuScopedNs(KhEmuDetail bucket)
        : target(g_khEmuDetailNanos[bucket]), t0(std::chrono::steady_clock::now()) {}

    ~KhEmuScopedNs()
    {
        target.fetch_add((uint64_t) std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - t0).count(), std::memory_order_relaxed);
    }
};

}
