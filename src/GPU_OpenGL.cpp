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

#include "GPU_OpenGL.h"

#include <assert.h>
#include <cstdio>
#include <cstring>

#include "NDS.h"
#include "GPU.h"
#include "GPU3D_OpenGL.h"
#include "OpenGLSupport.h"
#include "GPU_OpenGL_shaders.h"

namespace melonDS
{

using namespace OpenGL;

std::optional<GLCompositor> GLCompositor::New(Plugins::Plugin* plugin) noexcept // [KHMM] plugin param
{
    assert(glBindAttribLocation != nullptr);
    GLuint CompShader {};

    // [KHMM] let the game plugin supply a custom composite fragment shader (single-screen / HUD layout) — from KHMelonMix src/GPU_OpenGL.cpp
    const char* kCompositorFS_Custom = plugin == nullptr ? nullptr : plugin->gpuOpenGL_FS();
    const char* compositorFS = kCompositorFS_Custom == nullptr ? kCompositorFS_Nearest : kCompositorFS_Custom;
    if (!OpenGL::CompileVertexFragmentProgram(CompShader,
            kCompositorVS, compositorFS,
            "CompositorShader",
            {{"vPosition", 0}, {"vTexcoord", 1}},
            {{"oColor", 0}}))
        return std::nullopt;

    return { GLCompositor(CompShader, plugin) }; // [KHMM]
}

GLCompositor::GLCompositor(GLuint compShader, Plugins::Plugin* plugin) noexcept : CompShader(compShader) // [KHMM] plugin param
{
    CompScaleLoc = glGetUniformLocation(CompShader, "u3DScale");

    // [KHMM] hand the compositor shader to the plugin to bind its custom uniforms —
    // only when the plugin actually supplied a custom shader (see UsesPluginShader).
    GamePlugin = plugin;
    UsesPluginShader = (GamePlugin != nullptr && GamePlugin->gpuOpenGL_FS() != nullptr);
    if (UsesPluginShader) {
        GamePlugin->gpuOpenGL_FS_initVariables(CompShader);
    }

    glUseProgram(CompShader);
    GLuint screenTextureUniform = glGetUniformLocation(CompShader, "ScreenTex");
    glUniform1i(screenTextureUniform, 0);
    GLuint _3dTextureUniform = glGetUniformLocation(CompShader, "_3DTex");
    glUniform1i(_3dTextureUniform, 1);

    // [KHMM] when the plugin composite FS is active, also compile the stock nearest FS so
    // RenderFrame can switch programs at runtime (enhanced-graphics setting). Shares kCompositorVS
    // and the same texture interface, so it just needs its samplers + u3DScale location bound.
    if (UsesPluginShader) {
        if (OpenGL::CompileVertexFragmentProgram(CompShaderNearest,
                kCompositorVS, kCompositorFS_Nearest,
                "CompositorShaderNearest",
                {{"vPosition", 0}, {"vTexcoord", 1}},
                {{"oColor", 0}})) {
            glUseProgram(CompShaderNearest);
            glUniform1i(glGetUniformLocation(CompShaderNearest, "ScreenTex"), 0);
            glUniform1i(glGetUniformLocation(CompShaderNearest, "_3DTex"), 1);
            CompScaleLocNearest = glGetUniformLocation(CompShaderNearest, "u3DScale");
            glUseProgram(CompShader);
        }
    }

    // all this mess is to prevent bleeding
#define SETVERTEX(i, x, y, offset) \
    CompVertices[i].Position[0] = x; \
    CompVertices[i].Position[1] = y + offset; \
    CompVertices[i].Texcoord[0] = (x + 1.f) * (256.f / 2.f); \
    CompVertices[i].Texcoord[1] = (y + 1.f) * (384.f / 2.f)

    const float padOffset = 1.f/(192*2.f+2.f)*2.f;
    // top screen
    SETVERTEX(0, -1, 1, 0);
    SETVERTEX(1, 1, 0, padOffset);
    SETVERTEX(2, 1, 1, 0);
    SETVERTEX(3, -1, 1, 0);
    SETVERTEX(4, -1, 0, padOffset);
    SETVERTEX(5, 1, 0, padOffset);

    // bottom screen
    SETVERTEX(6, -1, 0, -padOffset);
    SETVERTEX(7, 1, -1, 0);
    SETVERTEX(8, 1, 0, -padOffset);
    SETVERTEX(9, -1, 0, -padOffset);
    SETVERTEX(10, -1, -1, 0);
    SETVERTEX(11, 1, -1, 0);

#undef SETVERTEX

    glGenBuffers(1, &CompVertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, CompVertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CompVertices), &CompVertices[0], GL_STATIC_DRAW);

    glGenVertexArrays(1, &CompVertexArrayID);
    glBindVertexArray(CompVertexArrayID);
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(CompVertex), (void*)(offsetof(CompVertex, Position)));
    glEnableVertexAttribArray(1); // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(CompVertex), (void*)(offsetof(CompVertex, Texcoord)));

    glGenFramebuffers(CompScreenOutputFB.size(), &CompScreenOutputFB[0]);

    // [KHMM] double-buffered input texture (indexed by backbuf)
    glGenTextures(CompScreenInputTex.size(), &CompScreenInputTex[0]);
    glActiveTexture(GL_TEXTURE0);
    for (GLuint tex : CompScreenInputTex)
    {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8UI, 256*3 + 1, 192*2, 0, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE, NULL);
    }

    glGenTextures(CompScreenOutputTex.size(), &CompScreenOutputTex[0]);
    for (GLuint i : CompScreenOutputTex)
    {
        glBindTexture(GL_TEXTURE_2D, i);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLCompositor::~GLCompositor()
{
    assert(glDeleteFramebuffers != nullptr);

    glDeleteFramebuffers(CompScreenOutputFB.size(), &CompScreenOutputFB[0]);
    glDeleteTextures(CompScreenInputTex.size(), &CompScreenInputTex[0]); // [KHMM] double-buffered
    glDeleteTextures(CompScreenOutputTex.size(), &CompScreenOutputTex[0]);

    glDeleteVertexArrays(1, &CompVertexArrayID);
    glDeleteBuffers(1, &CompVertexBufferID);

    glDeleteProgram(CompShader);
    if (CompShaderNearest) glDeleteProgram(CompShaderNearest); // [KHMM]
}


GLCompositor::GLCompositor(GLCompositor&& other) noexcept :
    Scale(other.Scale),
    ScreenH(other.ScreenH),
    ScreenW(other.ScreenW),
    CompScaleLoc(other.CompScaleLoc),
    CompShaderNearest(other.CompShaderNearest), // [KHMM]
    CompScaleLocNearest(other.CompScaleLocNearest), // [KHMM]
    CompositeFSEnabled(other.CompositeFSEnabled), // [KHMM]
    CompVertices(other.CompVertices),
    CompShader(other.CompShader),
    CompVertexBufferID(other.CompVertexBufferID),
    CompVertexArrayID(other.CompVertexArrayID),
    CompScreenInputTex(other.CompScreenInputTex),
    CompScreenOutputTex(other.CompScreenOutputTex),
    CompScreenOutputFB(other.CompScreenOutputFB),
    GamePlugin(other.GamePlugin), // [KHMM]
    UsesPluginShader(other.UsesPluginShader) // [KHMM]
{
    other.CompScreenOutputFB = {};
    other.CompScreenInputTex = {};
    other.CompScreenOutputTex = {};
    other.CompVertexArrayID = {};
    other.CompVertexBufferID = {};
    other.CompShader = {};
    other.CompShaderNearest = {}; // [KHMM]
}

GLCompositor& GLCompositor::operator=(GLCompositor&& other) noexcept
{
    if (this != &other)
    {
        Scale = other.Scale;
        ScreenH = other.ScreenH;
        ScreenW = other.ScreenW;
        CompScaleLoc = other.CompScaleLoc;
        CompVertices = other.CompVertices;
        CompScaleLocNearest = other.CompScaleLocNearest; // [KHMM]
        CompositeFSEnabled = other.CompositeFSEnabled;   // [KHMM]

        // Clean up these resources before overwriting them
        glDeleteProgram(CompShader);
        CompShader = other.CompShader;

        if (CompShaderNearest) glDeleteProgram(CompShaderNearest); // [KHMM]
        CompShaderNearest = other.CompShaderNearest;

        glDeleteBuffers(1, &CompVertexBufferID);
        CompVertexBufferID = other.CompVertexBufferID;

        glDeleteVertexArrays(1, &CompVertexArrayID);
        CompVertexArrayID = other.CompVertexArrayID;

        glDeleteTextures(CompScreenInputTex.size(), &CompScreenInputTex[0]); // [KHMM] double-buffered
        CompScreenInputTex = other.CompScreenInputTex;

        glDeleteTextures(CompScreenOutputTex.size(), &CompScreenOutputTex[0]);
        CompScreenOutputTex = other.CompScreenOutputTex;

        glDeleteFramebuffers(CompScreenOutputFB.size(), &CompScreenOutputFB[0]);
        CompScreenOutputFB = other.CompScreenOutputFB;

        other.CompScreenOutputFB = {};
        other.CompScreenInputTex = {};
        other.CompScreenOutputTex = {};
        other.CompVertexArrayID = {};
        other.CompVertexBufferID = {};
        other.CompShader = {};
        other.CompShaderNearest = {}; // [KHMM]
    }

    return *this;
}


void GLCompositor::SetScaleFactor(int scale) noexcept
{
    if (scale == Scale)
        return;

    Scale = scale;
    ScreenW = 256 * scale;
    ScreenH = (384+2) * scale;

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, CompScreenOutputTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ScreenW, ScreenH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        // fill the padding
        u8* zeroPixels = (u8*) calloc(1, ScreenW*2*scale*4);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192*scale, ScreenW, 2*scale, GL_RGBA, GL_UNSIGNED_BYTE, zeroPixels);

        GLenum fbassign[] = {GL_COLOR_ATTACHMENT0};
        glBindFramebuffer(GL_FRAMEBUFFER, CompScreenOutputFB[i]);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, CompScreenOutputTex[i], 0);
        glDrawBuffers(1, fbassign);
        free(zeroPixels);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLCompositor::Stop(const GPU& gpu) noexcept
{
    for (int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, CompScreenOutputFB[gpu.FrontBuffer]);

        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLCompositor::RenderFrame(const GPU& gpu, Renderer3D& renderer) noexcept
{
    int backbuf = gpu.FrontBuffer ^ 1;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, CompScreenOutputFB[backbuf]);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glViewport(0, 0, ScreenW, ScreenH);

    // TODO: select more shaders (filtering, etc)
    // [KHMM] pick the plugin composite FS or the stock nearest FS at runtime (enhanced-graphics
    // setting). With the gate off (or no nearest program compiled) we fall back to CompShader.
    bool usePlugin = UsesPluginShader && CompositeFSEnabled;
    GLuint prog = (usePlugin || CompShaderNearest == 0) ? CompShader : CompShaderNearest;
    GLuint scaleLoc = (prog == CompShader) ? CompScaleLoc : CompScaleLocNearest;
    glUseProgram(prog);
    glUniform1ui(scaleLoc, Scale);

    // [KHMM] refresh plugin-owned composite-shader uniforms each frame — only when the
    // plugin's custom shader is actually in use — from KHMelonMix src/GPU_OpenGL.cpp
    if (usePlugin) {
        GamePlugin->gpuOpenGL_FS_updateVariables(prog);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, CompScreenInputTex[backbuf]); // [KHMM] double-buffered

    if (gpu.Framebuffer[backbuf][0] && gpu.Framebuffer[backbuf][1])
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256*3 + 1, 192, GL_RGBA_INTEGER,
                        GL_UNSIGNED_BYTE, gpu.Framebuffer[backbuf][0].get());
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 192, 256*3 + 1, 192, GL_RGBA_INTEGER,
                        GL_UNSIGNED_BYTE, gpu.Framebuffer[backbuf][1].get());
    }

    glActiveTexture(GL_TEXTURE1);
    renderer.SetupAccelFrame();

    glBindBuffer(GL_ARRAY_BUFFER, CompVertexBufferID);
    glBindVertexArray(CompVertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, 4*3);
}

void GLCompositor::SetOutputTexture(int buf, GLuint texture)
{
    GLenum fbassign[] = {GL_COLOR_ATTACHMENT0};
    glBindFramebuffer(GL_FRAMEBUFFER, CompScreenOutputFB[buf]);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    glDrawBuffers(1, fbassign);
}

void GLCompositor::BindOutputTexture(int buf)
{
    glBindTexture(GL_TEXTURE_2D, CompScreenOutputTex[buf]);
}

}
