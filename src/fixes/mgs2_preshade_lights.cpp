#include "stdafx.h"

#include "mgs2_preshade_lights.hpp"

#include "common.hpp"
#include "logging.hpp"
#include <cmath>

#include "gamevars.hpp"

//Fixes incorrect lighting on Deck 2 doors, and posters in w01a.
namespace
{
    constexpr float kWrapScale = 1.0f;      // half-lambert wrap (i = 0.5*dot + 0.5)
    constexpr float kBackSideScale = 0.6f;  // away-facing side, tuned to the PS2 door tone

    constexpr uint32_t kDoorTex[] = {
        GameVars::GV_StrCode("w03b_dr00"),
        GameVars::GV_StrCode("w03b_dr01"),
        GameVars::GV_StrCode("w03b_dr00a"),
        GameVars::GV_StrCode("w03b_dr00b"), //w01a0_dr00b.bmp

    };

    constexpr uint32_t kNoShadeTex[] = {
        GameVars::GV_StrCode("w01alc_mao3"),
        GameVars::GV_StrCode("w01alc_saya2"),

    };

    enum class PackMode : uint8_t { Vanilla, DoorTwoSided, NoShadeNeutral };
    thread_local PackMode gPackMode = PackMode::Vanilla;

    PackMode ClassifyPack(uint32_t tex)
    {
        for (uint32_t t : kDoorTex)
        {
            if (t == tex)
            {
                return PackMode::DoorTwoSided;
            }
        }
        for (uint32_t t : kNoShadeTex)
        {
            if (t == tex)
            {
                return PackMode::NoShadeNeutral;
            }
        }
        return PackMode::Vanilla;
    }

    // per-pack heads of the three shade loops in DG_MakePreshade (rbx = pack + 4)
    SafetyHookMid ShadeLoop_hooks[3] {};
    uintptr_t ShadeLoop_skipRip[3] {};

    // The door hooks fire per vertex (and per light), and temp lights re-shade rooms every
    // frame - hook overhead there was the cutscene lag. Door packs are a handful in one
    // deck, so keep the hooks UNPATCHED and splice their bytes in only while a door pack
    // is shading. Everything else runs original code at original speed.
    struct ToggleHook
    {
        SafetyHookMid hook {};
        uint8_t* site = nullptr;
        uint8_t original[24] {};
        uint8_t patched[24] {};
        size_t len = 0;
    };
    ToggleHook gDoorHooks[2];
    bool gDoorPatched = false;

    void SetDoorHooks(bool enable)
    {
        if (enable == gDoorPatched)
        {
            return;
        }
        gDoorPatched = enable;
        for (auto& t : gDoorHooks)
        {
            if (!t.site)
            {
                continue;
            }
            DWORD old;
            VirtualProtect(t.site, t.len, PAGE_EXECUTE_READWRITE, &old);
            memcpy(t.site, enable ? t.patched : t.original, t.len);
            VirtualProtect(t.site, t.len, old, &old);
            FlushInstructionCache(GetCurrentProcess(), t.site, t.len);
        }
    }

    bool InstallDoorHook(ToggleHook& t, const char* pattern, const char* label,
        void (*fn)(SafetyHookContext&))
    {
        uint8_t* p = Memory::PatternScan(baseModule, pattern, label);
        if (!p)
        {
            return false;
        }
        t.site = p;
        memcpy(t.original, p, sizeof(t.original));
        t.hook = safetyhook::create_mid(p, fn);
        memcpy(t.patched, p, sizeof(t.patched));
        t.len = sizeof(t.original);
        while (t.len > 0 && t.original[t.len - 1] == t.patched[t.len - 1])
        {
            t.len--;
        }
        if (t.len == 0)
        {
            return false;
        }
        // start unpatched; the pack classifier splices it in for door packs
        DWORD old;
        VirtualProtect(p, t.len, PAGE_EXECUTE_READWRITE, &old);
        memcpy(p, t.original, t.len);
        VirtualProtect(p, t.len, old, &old);
        FlushInstructionCache(GetCurrentProcess(), p, t.len);
        return true;
    }

    void ShadeLoopHook(SafetyHookContext& ctx, int loop, bool outInRsi)
    {
        // pack tex slot = texture record pointer at runtime; strcode at record+0x10
        // (plain range check - IsReadable is a syscall and this runs per pack per reshade)
        uint32_t tex = 0;
        const uint64_t rec = *reinterpret_cast<const uint64_t*>(ctx.rbx + 4);
        if (rec > 0x10000 && rec < 0x00007FFFFFFFFFFFull)
        {
            tex = *reinterpret_cast<const uint32_t*>(rec + 0x10);
        }
        gPackMode = ClassifyPack(tex);
        SetDoorHooks(gPackMode == PackMode::DoorTwoSided);

        if (gPackMode == PackMode::NoShadeNeutral)
        {
            // neutral vertex colors, skip the lighting call
            uint32_t* out = reinterpret_cast<uint32_t*>(outInRsi ? ctx.rsi : ctx.rdi);
            const uint32_t n = *reinterpret_cast<const uint32_t*>(ctx.rbx);
            if (out && n <= 0x10000)
            {
                for (uint32_t i = 0; i < n; i++)
                {
                    out[i] = 0x80808080;
                }
            }
            ctx.rip = ShadeLoop_skipRip[loop];
        }
    }

    // The port rotates preshade light-selection bounds with the matrix transposed, so
    // anything placed at an angle misses point lights it should own (the Deck-2 door
    // face baked near-black from this). Redo the transform the right way round.
    float* gPreshadeMatrix = nullptr;

    void BoundFix_Hook(SafetyHookContext& ctx)
    {
        const float* m = gPreshadeMatrix;
        const float inMax[3] = { ctx.xmm6.f32[0], ctx.xmm8.f32[0], ctx.xmm10.f32[0] };
        const float inMin[3] = { ctx.xmm7.f32[0], ctx.xmm9.f32[0], ctx.xmm11.f32[0] };
        float outMin[3];
        float outMax[3];
        for (int j = 0; j < 3; j++)
        {
            outMin[j] = outMax[j] = m[3 * 4 + j];
            for (int i = 0; i < 3; i++)
            {
                const float a = m[i * 4 + j] * inMin[i];
                const float b = m[i * 4 + j] * inMax[i];
                outMin[j] += a < b ? a : b;
                outMax[j] += a < b ? b : a;
            }
        }
        // min slot holds the smaller values
        float* s0 = reinterpret_cast<float*>(ctx.rsp);
        float* s1 = reinterpret_cast<float*>(ctx.rsp + 0x10);
        float* dstMin = s0[0] <= s1[0] ? s0 : s1;
        float* dstMax = s0[0] <= s1[0] ? s1 : s0;
        for (int j = 0; j < 3; j++)
        {
            dstMin[j] = outMin[j];
            dstMax[j] = outMax[j];
        }
    }

    void ShadeLoop0_Hook(SafetyHookContext& ctx) { ShadeLoopHook(ctx, 0, true); }
    void ShadeLoop1_Hook(SafetyHookContext& ctx) { ShadeLoopHook(ctx, 1, true); }
    void ShadeLoop2_Hook(SafetyHookContext& ctx) { ShadeLoopHook(ctx, 2, false); }

    // The port took pshade.c's PS3 branch of BP_LineLightCalc, which skips the per-vertex bound
    // test PS2 ran, so line lights spill past their boxes and wash surfaces that should stay
    // dark. Restore the test, and guard the 1/len that baked on-segment verts black.
    SafetyHookInline LineLight_hook {};

    void LineLight_Replacement(const float* v, const float* n, const uint8_t* litRaw, float* color)
    {
        const float* lit = reinterpret_cast<const float*>(litRaw);
        const float* bmax = lit;                             // +0x00
        const float* bmin = lit + 4;                         // +0x10
        if (v[0] > bmax[0] || v[0] < bmin[0] || v[1] > bmax[1] || v[1] < bmin[1] ||
            v[2] > bmax[2] || v[2] < bmin[2])
        {
            return;
        }
        const float* start = lit + 8;                        // +0x20
        const float* dir = lit + 12;                         // +0x30, segment length @ +0x3c
        const float segLen = lit[15];
        const float n1 = start[0]*dir[0] + start[1]*dir[1] + start[2]*dir[2];
        const float n2 = (start[0] + dir[0]*segLen)*dir[0] + (start[1] + dir[1]*segLen)*dir[1]
                       + (start[2] + dir[2]*segLen)*dir[2];
        float n3 = v[0]*dir[0] + v[1]*dir[1] + v[2]*dir[2];
        n3 = n3 < n1 ? n1 : (n3 > n2 ? n2 : n3);
        const float t = n3 - n1;
        const float dx = v[0] - (start[0] + dir[0]*t);
        const float dy = v[1] - (start[1] + dir[1]*t);
        const float dz = v[2] - (start[2] + dir[2]*t);
        const float len2 = dx*dx + dy*dy + dz*dz;
        const float len = len2 > 1e-6f ? std::sqrt(len2) : 0.0f;
        const float range = lit[17] * 2.0f;                  // r_range @ +0x44
        if (len >= range)
        {
            return;
        }
        const float inv = len > 1e-6f ? 1.0f / len : 0.0f;
        float i = (n[0]*dx + n[1]*dy + n[2]*dz) * inv;
        if (i < 0.0f)
        {
            return;
        }
        i *= 1.5f * (range - len) / range;
        color[0] += litRaw[0x40] * i;
        color[1] += litRaw[0x41] * i;
        color[2] += litRaw[0x42] * i;
    }
}

void MGS2PreshadeLights::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    // pack classification at each shade-loop head (shade / reshade / reshade2)
    struct { const char* pattern; const char* label; size_t skipOfs; void (*fn)(SafetyHookContext&); } loops[] = {
        { "?? ?? ?? 48 8B CE 4C 8B 43 ?? 48 8B 53 ?? E8 ?? ?? ?? ?? ?? ?? 48 8D 5B ?? FF C5",
          "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> ShadeRGB() pack loop", 19, ShadeLoop0_Hook },
        { "?? ?? ?? 48 8B CE 4C 8B 43 ?? 48 8B 53 ?? E8 ?? ?? ?? ?? ?? ?? 48 8D 5B ?? FF CF",
          "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> ReshadeRGB() pack loop", 19, ShadeLoop1_Hook },
        { "?? ?? ?? 48 8B CF 4C 8B 43",
          "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> ReshadeRGB2() pack loop", 24, ShadeLoop2_Hook },
    };
    for (int i = 0; i < 3; i++)
    {
        if (uint8_t* p = Memory::PatternScan(baseModule, loops[i].pattern, loops[i].label))
        {
            ShadeLoop_skipRip[i] = reinterpret_cast<uintptr_t>(p) + loops[i].skipOfs;
            ShadeLoop_hooks[i] = safetyhook::create_mid(p, loops[i].fn);
            LOG_HOOK(ShadeLoop_hooks[i], loops[i].label)
        }
        else
        {
            spdlog::error("MGS 2: Two-Sided Preshade Lighting : pack loop {} scan failed; staying vanilla.", i);
            return;
        }
    }

    // light-selection bound correction
    if (uint8_t* bf = Memory::PatternScan(baseModule, "F3 0F 10 74 24 ?? 48 8D 0D", "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> CreateLightBuffer() bound transform"))
    {
        gPreshadeMatrix = reinterpret_cast<float*>(Memory::GetRelativeOffset(Memory::PatternScan(baseModule, "F3 44 0F 10 05 ?? ?? ?? ?? F3 44 0F 10 15 ?? ?? ?? ?? F3 44 0F 10 1D ?? ?? ?? ?? 41 0F 28 CA", "MGS 2: Two-Sided Preshade Lighting : g_PreshadeMatrix") + 5));
        static SafetyHookMid boundFix {};
        boundFix = safetyhook::create_mid(bf, BoundFix_Hook);
        LOG_HOOK(boundFix, "MGS 2: Two-Sided Preshade Lighting : light-selection bound fix")
    }

    // Directional and point two-sided treatment, door packs only - spliced in per pack.
    const bool dirOk = InstallDoorHook(gDoorHooks[0],
        "F3 0F 5F CA F3 0F 59 F1",
        "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> BP_ParallelAmbientCalc()",
        [](SafetyHookContext& ctx) {
            if (gPackMode != PackMode::DoorTwoSided)
                return;
            const float d = ctx.xmm2.f32[0];                    // dot(normal, light dir)
            const float wrap = kWrapScale * (d * 0.5f + 0.5f);
            const float back = d < 0.0f ? -d * kBackSideScale : 0.0f;
            float i = wrap > back ? wrap : back;
            if (i > 1.0f) i = 1.0f;
            ctx.xmm1.f32[0] = i;
        });
    const bool pointOk = InstallDoorHook(gDoorHooks[1],
        "44 0F 2F CE 77 ?? 0F B6 43",
        "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> BP_PointLightCalc()",
        [](SafetyHookContext& ctx) {
            if (gPackMode != PackMode::DoorTwoSided)
                return;
            const float d = ctx.xmm6.f32[0];                    // dot(dir-to-vertex, normal)
            if (d < 0.0f)
                ctx.xmm6.f32[0] = -d * kBackSideScale;
        });
    if (!dirOk || !pointOk)
    {
        spdlog::error("MGS 2: Two-Sided Preshade Lighting : door hook install failed.");
    }

    // line-light bound restore (see LineLight_Replacement)
    if (uint8_t* ll = Memory::PatternScan(baseModule,
        "48 8B C4 48 89 58 ?? 48 89 70 ?? 57 48 81 EC ?? ?? ?? ?? F3 41 0F 10 68",
        "MGS 2: Preshade Lights : system\\libdg\\pshade.c -> BP_LineLightCalc()"))
    {
        LineLight_hook = safetyhook::create_inline(ll, reinterpret_cast<void*>(LineLight_Replacement));
        LOG_HOOK(LineLight_hook, "MGS 2: Preshade Lights : line-light bound restore")
    }
    else
    {
        spdlog::error("MGS 2: Preshade Lights : BP_LineLightCalc scan failed.");
    }
}
