/*
    Copyright 2016-2025 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#pragma once

#include "OpenGLSupport.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <optional>

// [KHMM] compositor takes a game Plugin to source a custom composite shader — from KHMelonMix src/GPU_OpenGL.h
#include "plugins/Plugin.h"

namespace melonDS
{
// [KHMM-DBG] accumulated GL 3D-render + composite time (ns) for the current frame. The GL render
// runs INSIDE nds->RunFrame(), so the frontend can't time it directly; both GL render entry points
// (GPU3D_OpenGL::RenderFrame and GLCompositor::RenderFrame) add here, and MelonInstance reads+resets
// it each report window to split runframe into GL-render vs emulation. Temporary; grep [KHMM-DBG].
extern std::atomic<uint64_t> g_khGlRenderNanos;

// [KHMM-DBG] fine split of the GL time above, to locate the cost inside the render. Indices:
// 0=UBO map/unmap  1=VRAM texture upload  2=palette convert+upload  3=polygon setup+buffer upload
// 4=draw submission (RenderSceneChunk)  5=composite pass. g_khGlScale mirrors the active internal
// resolution ScaleFactor so the perf log always states the res it measured. Temporary; grep [KHMM-DBG].
enum KhGlDetail { KH_GL_UBO = 0, KH_GL_VRAMTEX, KH_GL_PAL, KH_GL_POLY, KH_GL_DRAW, KH_GL_COMP, KH_GL_DETAIL_COUNT };
extern std::atomic<uint64_t> g_khGlDetailNanos[KH_GL_DETAIL_COUNT];
extern std::atomic<int> g_khGlScale;

class GPU;
struct RenderSettings;
class GLRenderer;
class Renderer3D;
class GLCompositor
{
public:
    static std::optional<GLCompositor> New(Plugins::Plugin* plugin) noexcept; // [KHMM] plugin param
    GLCompositor(const GLCompositor&) = delete;
    GLCompositor& operator=(const GLCompositor&) = delete;
    GLCompositor(GLCompositor&&) noexcept;
    GLCompositor& operator=(GLCompositor&&) noexcept;
    ~GLCompositor();

    void SetScaleFactor(int scale) noexcept;
    [[nodiscard]] int GetScaleFactor() const noexcept { return Scale; }

    void Stop(const GPU& gpu) noexcept;
    void RenderFrame(const GPU& gpu, Renderer3D& renderer) noexcept;
    void SetOutputTexture(int buf, GLuint texture);
    void BindOutputTexture(int buf);
    // [KHMM-DBG] runtime composite-shader toggle for perf isolation: false draws with the stock
    // nearest FS instead of the plugin composite FS (only meaningful when a plugin FS is active).
    void SetCompositeFSEnabled(bool enabled) noexcept { CompositeFSEnabled = enabled; }
private:
    GLCompositor(GLuint CompShader, Plugins::Plugin* plugin) noexcept; // [KHMM] plugin param
    int Scale = 0;
    int ScreenH = 0, ScreenW = 0;

    GLuint CompShader {};
    GLuint CompScaleLoc = 0;

    // [KHMM-DBG] second compiled program = the stock nearest composite FS, so the plugin
    // composite shader can be toggled at runtime (perf isolation) without a renderer reinit.
    // Compiled only when the plugin supplied a custom FS; 0 otherwise (toggle then a no-op).
    GLuint CompShaderNearest = 0;
    GLuint CompScaleLocNearest = 0;
    bool CompositeFSEnabled = true;

    GLuint CompVertexBufferID = 0;
    GLuint CompVertexArrayID = 0;

    struct CompVertex
    {
        std::array<float, 2> Position {};
        std::array<float, 2> Texcoord {};
    };
    std::array<CompVertex, 2*3*2> CompVertices {};

    // [KHMM] double-buffered (indexed by backbuf): re-uploaded every frame while the previous
    // composite draw may still be sampling it — alternating avoids the upload stall.
    std::array<GLuint, 2> CompScreenInputTex {};
    std::array<GLuint, 2> CompScreenOutputTex {};
    std::array<GLuint, 2> CompScreenOutputFB {};

    Plugins::Plugin* GamePlugin = nullptr; // [KHMM] active game plugin (custom composite shader)
    // [KHMM] true only when the plugin actually supplied a custom composite shader.
    // Gates the plugin's per-frame GL uniform/UBO updates so nothing plugin-specific
    // (e.g. glMapBuffer) runs on the default shader path. Port-specific safety.
    bool UsesPluginShader = false;
};

}
