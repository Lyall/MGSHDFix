#include "stdafx.h"
#include "mgs2_rotor_procession.hpp"

#include <cmath>
#include <unordered_map>

#include "common.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"
#include "gamevars.hpp"

namespace
{
    bool isTanker = false;


    // Hook: chain2 packet emitter's matrix copy. rsi = unit, r12 = matrix block; blk[0] is
    // the model->clip composite the shader consumes.
    SafetyHookMid Emit_hook {};

    float gDriftDegPerFrame = -1.0f; // accumulating; negative = against the strobe

    // Only these rotor-blade textures get drifted.
    constexpr uint32_t kRotorTex[] = {
        GameVars::GV_StrCode("kck_blade_blur_alp"),
        GameVars::GV_StrCode("kck_roter01_alp"),
        GameVars::GV_StrCode("kck_mrot_propera"),

        //GameVars::GV_StrCode("gcyp_eff_blade_alp"),     
        //GameVars::GV_StrCode("gcyp_blade_alp"),

        //GameVars::GV_StrCode("cyp_roter01_alp"),
        //GameVars::GV_StrCode("cyp_propera_alp"),

        //GameVars::GV_StrCode("h6h_propera_blur_alp"),
        //GameVars::GV_StrCode("h6h_eff_mrot_alp"),

    };
    bool IsRotorTex(uint32_t tex)
    {
        for (uint32_t t : kRotorTex) { if (t == tex) { return true; } }
        return false;
    }

    struct Track
    {
        float wPrev[9];     // last frame's world orientation rows
        float axis[3];      // cardinal spin axis, once established
        float phase;        // accumulated drift (deg)
        uint64_t lastFrame;
        bool valid;
        bool locked;
    };
    std::unordered_map<uintptr_t, Track> gTracks;

    // Normalised rotation rows (0-2) of a 4-wide matrix.
    bool LoadRot(const float* m, float* r)
    {
        for (int row = 0; row < 3; row++)
        {
            const float* v = m + row * 4;
            const float len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
            if (len < 1e-4f || !std::isfinite(len))
            {
                return false;
            }
            r[row*3+0] = v[0]/len; r[row*3+1] = v[1]/len; r[row*3+2] = v[2]/len;
        }
        return true;
    }

    // Frame-to-frame spin between two orientations: L = cur * prev^T, axis + angle.
    bool RelRotation(const float* prev, const float* cur, float* axis, float* angDeg)
    {
        float L[9];
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                L[i*3+j] = cur[i*3+0]*prev[j*3+0] + cur[i*3+1]*prev[j*3+1] + cur[i*3+2]*prev[j*3+2];
            }
        }
        float c = (L[0] + L[4] + L[8] - 1.0f) * 0.5f;
        c = c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c);
        *angDeg = std::acos(c) * 57.29578f;
        axis[0] = L[7] - L[5]; axis[1] = L[2] - L[6]; axis[2] = L[3] - L[1];
        const float al = std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
        if (al < 1e-5f)
        {
            return false;
        }
        axis[0] /= al; axis[1] /= al; axis[2] /= al;
        return true;
    }

    // Spin the composite about the axis: M' = R * M, translation row untouched.
    void ApplyModelRotation(float* m, const float* axis, float deg)
    {
        const float th = deg * 0.017453293f;
        const float c = std::cos(th), s = std::sin(th), t = 1.0f - c;
        const float x = axis[0], y = axis[1], z = axis[2];
        const float R[9] = {
            t*x*x + c,   t*x*y + s*z, t*x*z - s*y,
            t*x*y - s*z, t*y*y + c,   t*y*z + s*x,
            t*x*z + s*y, t*y*z - s*x, t*z*z + c,
        };
        float out[12];
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                out[i*4+j] = R[i*3+0]*m[0*4+j] + R[i*3+1]*m[1*4+j] + R[i*3+2]*m[2*4+j];
            }
        }
        memcpy(m, out, sizeof(out));
    }

    // First pack's texture strcode: packs = *(unit+0x118), rec = *(pack0+0x20), code @ rec+0x10.
    uint32_t UnitTex(uintptr_t unit)
    {
        const uintptr_t packs = *reinterpret_cast<const uintptr_t*>(unit + 0x118);
        if (packs <= 0x10000 || packs >= 0x00007FFFFFFFFFFFull) { return 0; }
        const uintptr_t rec = *reinterpret_cast<const uintptr_t*>(packs + 0x20);
        if (rec <= 0x10000 || rec >= 0x00007FFFFFFFFFFFull) { return 0; }
        return *reinterpret_cast<const uint32_t*>(rec + 0x10);
    }

    void Emit_Hook(SafetyHookContext& ctx)
    {
        if (!isTanker) //rotors are randomly flipping axis atm. it's more visible during plant due to the brighter skies.
        {
            return;
        }
        float* blk = reinterpret_cast<float*>(ctx.r12);
        const uintptr_t unit = ctx.rsi;
        if (!blk || unit < 0x10000)
        {
            return;
        }
        // heli procession only happens in demos, and only rotor blades get past here
        if (!g_GameVars.InCutscene() || !IsRotorTex(UnitTex(unit)))
        {
            return;
        }

        float cur[9];
        if (!LoadRot(blk, cur))
        {
            return;
        }

        if (gTracks.size() > 2048)
        {
            gTracks.clear();
        }
        const uint64_t frame = g_D3D11Hooks.FrameCount;
        auto& t = gTracks[unit];

        if (t.valid && t.lastFrame != frame)
        {
            // establish / refresh the cardinal spin axis (identity already proved it's a rotor)
            float axis[3];
            float ang = 0.0f;
            if (RelRotation(t.wPrev, cur, axis, &ang) && ang > 15.0f)
            {
                int dom = 0;
                for (int i = 1; i < 3; i++) { if (std::fabs(axis[i]) > std::fabs(axis[dom])) { dom = i; } }
                if (std::fabs(axis[dom]) > 0.7f)
                {
                    const float sign = axis[dom] < 0.0f ? -1.0f : 1.0f;
                    t.axis[0] = t.axis[1] = t.axis[2] = 0.0f;
                    t.axis[dom] = sign;
                    t.locked = true;
                }
            }
            memcpy(t.wPrev, cur, sizeof(cur));

            if (t.locked)
            {
                t.phase += gDriftDegPerFrame;
                if (t.phase >= 360.0f) { t.phase -= 360.0f; }
                else if (t.phase <= -360.0f) { t.phase += 360.0f; }
            }
        }
        else if (!t.valid)
        {
            memcpy(t.wPrev, cur, sizeof(cur));
            t.valid = true;
        }
        t.lastFrame = frame;

        if (t.locked)
        {
            ApplyModelRotation(blk, t.axis, t.phase);
        }
    }

}


void MGS2RotorProcession::HandleLevelTransition()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    isTanker = (g_GameVars.MGS2_GetGameMode() == MGS2GameMode::Tanker);

}

void MGS2RotorProcession::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    if (uint8_t* site = Memory::PatternScan(baseModule,
        "?? ?? ?? ?? ?? 45 33 FF ?? ?? ?? 41 0F 10 4C 24",
        "MGS 2: Rotor Procession : libdg\\chain2.c -> unit packet emitter matrix copy"))
    {
        Emit_hook = safetyhook::create_mid(site, Emit_Hook);
        LOG_HOOK(Emit_hook, "MGS 2: Rotor Procession : packet emitter")
    }
    else
    {
        spdlog::error("MGS 2: Rotor Procession : emitter scan failed.");
    }
}
