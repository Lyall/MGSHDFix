#include "stdafx.h"

#include "mgs2_preshade_lights.hpp"

#include "common.hpp"
#include "logging.hpp"

// Preshade fixes: two-sided lighting for the inside-out watertight door, fullbright
// collectables, and a light-selection bound correction for rotated models.

namespace
{
    constexpr float kWrapScale = 1.0f;      // half-lambert wrap (i = 0.5*dot + 0.5)
    constexpr float kBackSideScale = 0.6f;  // away-facing side, tuned to the PS2 door tone

    // w03bsdr door textures only; never the shared gray trim (0x284f03)
    constexpr uint32_t kDoorTex[] = { 0x8b77b4, 0x8b77b5, 0x9c5bba, 0x9c5bbb };

    // Fullbright collectables (posters, memos): bake packs neutral and fullbright works.
    // Add any posters we missed here: code = the texture's 8-hex id from
    // BP_FlatlistTextureMapping.txt (strcode24 of the texture name).
    constexpr uint32_t kNoShadeTex[] = {
        // memo notes, bullet holes
        0x45e48e, 0x3b57e3, 0x3c57e3, 0xbd4c0b, 0x144548, 0x884312,
        // tanker posters (quarters, lounge, engine room)
        0x5c37c1, 0xe7261d, 0x2cd7be, 0x5fc83f, 0x5fc840, 0x5fc841,
        0x4af484, 0x4afb45, 0x4afb46,
        // plant struts (pump room, transformer room, dining hall)
        0x0a9041, 0xcae92c, 0x21f6e6, 0xc9e498, 0x926f7c,
        0x82d8f5, 0x82d8f6, 0x915d28, 0x915d48,
        // Shell 1 core (lockers, game posters, monitors, lounge)
        0x9b65c9, 0x21f7e6, 0x21f7e7, 0xf3c96d, 0xdbab00, 0xb6ea53,
        0xb3f9e2, 0xc3f9e2, 0xaa3210, 0x3d7b2a, 0x3d7b2b, 0x3b1381,
        // Shell 2 core (filtration chambers, FHM)
        0xeaea41, 0xeaea42, 0xeaea43, 0xeeea3e,
        // Substance idol posters + alt monitors + promo
        0xd5613b, 0x15861e, 0x3b62fa, 0xc5ee63, 0xc6b2c7,
        0x15861d, 0x3b62fb, 0xc5ee62, 0xb3f932, 0xc3f932, 0x8ef342,
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
    uint8_t* gPreshadeMatrix = nullptr;

    void BoundFix_Hook(SafetyHookContext& ctx)
    {
        const float* m = reinterpret_cast<const float*>(gPreshadeMatrix);
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
}

void MGS2PreshadeLights::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    // pack classification at each shade-loop head (shade / reshade / reshade2)
    struct { const char* pattern; const char* label; size_t skipOfs; void (*fn)(SafetyHookContext&); } loops[] = {
        { "44 8B 0B 48 8B CE 4C 8B 43 24 48 8B 53 1C E8 ?? ?? ?? ?? 8B 03 48 8D 5B 60 FF C5",
          "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> ShadeRGB() pack loop", 19, ShadeLoop0_Hook },
        { "44 8B 0B 48 8B CE 4C 8B 43 24 48 8B 53 1C E8 ?? ?? ?? ?? 8B 03 48 8D 5B 60 FF CF",
          "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> ReshadeRGB() pack loop", 19, ShadeLoop1_Hook },
        { "44 8B 0B 48 8B CF 4C 8B 43 24 48 8B 53 1C 48 89 6C 24 20 E8",
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
    gPreshadeMatrix = reinterpret_cast<uint8_t*>(baseModule) + 0x15557E0;
    if (uint8_t* bf = Memory::PatternScan(baseModule,
        "F3 0F 10 74 24 08 48 8D 0D ?? ?? ?? ?? F3 0F 10 7C 24 04",
        "MGS 2: Two-Sided Preshade Lighting : system\\libdg\\pshade.c -> CreateLightBuffer() bound transform"))
    {
        static SafetyHookMid boundFix {};
        boundFix = safetyhook::create_mid(bf, BoundFix_Hook);
        LOG_HOOK(boundFix, "MGS 2: Two-Sided Preshade Lighting : light-selection bound fix")
    }

    // Directional and point two-sided treatment, door packs only - spliced in per pack.
    const bool dirOk = InstallDoorHook(gDoorHooks[0],
        "F3 0F 5F CA F3 0F 59 F1 F3 0F 59 F9",
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
        "44 0F 2F CE 77 60 0F B6 43 10 0F 28 C2",
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
}
