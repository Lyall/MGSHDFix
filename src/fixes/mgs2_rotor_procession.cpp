#include "stdafx.h"
#include "mgs2_rotor_procession.hpp"

#include <cmath>
#include <unordered_map>

#include "common.hpp"
#include "logging.hpp"

namespace
{
    // Hook: chain2 packet emitter's matrix copy. rsi = unit, r12 = matrix block.
    // blk[0] is the model->clip composite the shader consumes; blk[1..3] are lighting - leave them alone.
    SafetyHookMid Emit_hook {};

    float gDriftDegPerFrame = -1.0f; // accumulating; negative = against the strobe

    struct Track
    {
        float m16[16];     // last clean composite
        float rPost[9];    // pose as left after injection (same-frame repeat detection)
        float axis[3];     // model-space spin axis, cardinal-snapped at lock
        float step;        // smoothed per-frame step (deg)
        float phase;       // accumulated drift (deg)
        int streak;
        int quiet;
        bool valid;
        bool rotor;
    };
    std::unordered_map<uintptr_t, Track> gTracks;

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

    float DeltaAngle(const float* a, const float* b)
    {
        float tr = 0.0f;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                if (i == j)
                    tr += a[0*3+i]*b[0*3+j] + a[1*3+i]*b[1*3+j] + a[2*3+i]*b[2*3+j];
        float c = (tr - 1.0f) * 0.5f;
        c = c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c);
        return std::acos(c) * 57.29578f;
    }

    bool Mat4Invert(const float* m, float* out)
    {
        float inv[16];
        inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
        inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
        inv[8]  =  m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
        inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
        inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
        inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
        inv[9]  = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
        inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
        inv[2]  =  m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
        inv[6]  = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
        inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
        inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
        inv[3]  = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
        inv[7]  =  m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
        inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
        inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];
        float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
        if (std::fabs(det) < 1e-12f || !std::isfinite(det))
        {
            return false;
        }
        det = 1.0f / det;
        for (int i = 0; i < 16; i++)
        {
            out[i] = inv[i] * det;
        }
        return true;
    }

    // model-space spin between consecutive composites: L = cur * inv(prev)
    bool ModelAxisFromPair(const float* prev, const float* cur, float* axis, float* angDeg)
    {
        float ip[16];
        if (!Mat4Invert(prev, ip))
        {
            return false;
        }
        float L[16];
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                L[i*4+j] = cur[i*4+0]*ip[0*4+j] + cur[i*4+1]*ip[1*4+j] +
                           cur[i*4+2]*ip[2*4+j] + cur[i*4+3]*ip[3*4+j];
            }
        }
        // Only lock genuine rotations. A real rotor spins in place so L is a clean rotation;
        // a moving camera (e.g. the glass-shatter picture-in-picture) makes static geometry
        // look like it spins, but L comes out non-orthonormal - reject it or the room drifts.
        const float l0 = L[0]*L[0] + L[1]*L[1] + L[2]*L[2];
        const float l1 = L[4]*L[4] + L[5]*L[5] + L[6]*L[6];
        const float l2 = L[8]*L[8] + L[9]*L[9] + L[10]*L[10];
        const float d01 = L[0]*L[4] + L[1]*L[5] + L[2]*L[6];
        const float d02 = L[0]*L[8] + L[1]*L[9] + L[2]*L[10];
        const float d12 = L[4]*L[8] + L[5]*L[9] + L[6]*L[10];
        if (std::fabs(l0 - 1.0f) > 0.05f || std::fabs(l1 - 1.0f) > 0.05f ||
            std::fabs(l2 - 1.0f) > 0.05f ||
            std::fabs(d01) > 0.05f || std::fabs(d02) > 0.05f || std::fabs(d12) > 0.05f)
        {
            return false;
        }

        float c = (L[0] + L[5] + L[10] - 1.0f) * 0.5f;
        c = c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c);
        *angDeg = std::acos(c) * 57.29578f;
        axis[0] = L[9] - L[6]; axis[1] = L[2] - L[8]; axis[2] = L[4] - L[1];
        const float al = std::sqrt(axis[0]*axis[0] + axis[1]*axis[1] + axis[2]*axis[2]);
        if (al < 1e-5f)
        {
            return false;
        }
        axis[0] /= al; axis[1] /= al; axis[2] /= al;
        return true;
    }

    // model-space rotation on the composite: M' = R * M, translation row untouched
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

    void Emit_Hook(SafetyHookContext& ctx)
    {
        float* blk = reinterpret_cast<float*>(ctx.r12);
        if (!blk)
        {
            return;
        }
        float r[9];
        if (!LoadRot(blk, r))
        {
            return;
        }
        if (gTracks.size() > 2048)
        {
            gTracks.clear(); // scene churn; spinners re-lock within 8 frames
        }
        auto& t = gTracks[static_cast<uintptr_t>(ctx.rsi)];
        if (t.valid)
        {
            // repeat packet within the same frame: block already handled
            if (DeltaAngle(t.rPost, r) < 0.5f)
            {
                return;
            }
            float axis[3];
            float ang = 0.0f;
            const bool solved = ModelAxisFromPair(t.m16, blk, axis, &ang);
            // machine-constant spin only: steady step about a steady axis
            const bool qualifies = solved && ang > 20.0f && ang < 120.0f &&
                (t.streak == 0 ||
                 (std::fabs(ang - t.step) < 1.5f &&
                  axis[0]*t.axis[0] + axis[1]*t.axis[1] + axis[2]*t.axis[2] > 0.995f));
            if (qualifies)
            {
                t.step = t.streak == 0 ? ang : t.step * 0.9f + ang * 0.1f;
                memcpy(t.axis, axis, sizeof(axis));
                t.quiet = 0;
                if (++t.streak == 8 && !t.rotor)
                {
                    // rotors are authored on exact cardinal axes
                    int dom = 0;
                    for (int i = 1; i < 3; i++)
                    {
                        if (std::fabs(t.axis[i]) > std::fabs(t.axis[dom])) { dom = i; }
                    }
                    if (std::fabs(t.axis[dom]) > 0.9f)
                    {
                        const float sign = t.axis[dom] < 0.0f ? -1.0f : 1.0f;
                        t.axis[0] = t.axis[1] = t.axis[2] = 0.0f;
                        t.axis[dom] = sign;
                    }
                    t.rotor = true;
                }
            }
            else if (++t.quiet > 30 && t.rotor)
            {
                t.rotor = false;
                t.streak = 0;
                t.phase = 0.0f;
            }
            else if (t.quiet > 2)
            {
                t.streak = 0;
            }
        }
        memcpy(t.m16, blk, sizeof(t.m16));
        t.valid = true;

        if (t.rotor && gDriftDegPerFrame != 0.0f)
        {
            t.phase += gDriftDegPerFrame;
            if (t.phase >= 360.0f)
            {
                t.phase -= 360.0f;
            }
            else if (t.phase <= -360.0f)
            {
                t.phase += 360.0f;
            }
            ApplyModelRotation(blk, t.axis, t.phase);
        }
        LoadRot(blk, t.rPost);
    }
}

void MGS2RotorProcession::Initialize()
{
    if (!(eGameType & MGS2) || !bEnabled)
    {
        return;
    }

    if (uint8_t* site = Memory::PatternScan(baseModule,
        "41 0F 10 04 24 45 33 FF 0F 11 00",
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
