#include "stdafx.h"
#include "mgs3_map_relight.hpp"

#include "common.hpp"
#include "logging.hpp"
#include "d3d11_api.hpp"

// Every light ships with a box saying how far it is allowed to reach. BP throws those
// away at load and works out its own, which come out far bigger - light spills onto ceilings
// and walls the artists kept it off. So let BP build its boxes, then put Konami's back.

namespace
{
    constexpr int kLitTypeChange = 0x8000;

    bool g_changeGroup = false;
    bool g_anyChangeThisBake = false;
    bool g_inBake = false;
    uintptr_t gSkipModel = 0;

    uintptr_t gSkipSpot = 0;
    uintptr_t gSkipLine = 0;
    uintptr_t gSkipBlack = 0;
    SafetyHookMid gSpot{};
    SafetyHookMid gLine{};
    SafetyHookMid gBlack{};
    SafetyHookMid gSelEntry{};
    SafetyHookMid gGroupPass{};
    SafetyHookMid gModelGate{};
    SafetyHookMid gModelEnd{};

    // entry+0x00 = box min, entry+0x10 = box max: the layout the area loop already tests.
    void Gate(SafetyHookContext& ctx, size_t rigOffset, uintptr_t skip, bool strict)
    {
        const uint8_t* base = *reinterpret_cast<const uint8_t* const*>(ctx.r14 + rigOffset);
        if (!base)
        {
            return;
        }
        const float* e = reinterpret_cast<const float*>(base + ctx.rdi);
        const float* v = reinterpret_cast<const float*>(ctx.rsi);
        const bool outside = strict
            ? (e[4] <= v[0] || e[0] >= v[0] || e[5] <= v[1] || e[1] >= v[1] || e[6] <= v[2] || e[2] >= v[2])
            : (v[0] > e[4] || e[0] > v[0] || v[1] > e[5] || e[1] > v[1] || v[2] > e[6] || e[2] > v[2]);
        if (outside)
        {
            ctx.rip = skip;
        }
    }

    // BP checks the box once for a whole model, and its rooms are one big model - so one
    // corner in range lights all of it. Carry the box with the light and check it per vertex.
    struct GroupBound { const void* entry; float mn[3]; float mx[3]; };
    GroupBound gLineGroups[64];
    int gLineGroupCount = 0;

    void LineCopy_Hook(SafetyHookContext& ctx)
    {
        if (gLineGroupCount >= 64)
        {
            return;
        }
        const uint8_t* group = reinterpret_cast<const uint8_t*>(ctx.r11 - 0x18);
        const float* mx = reinterpret_cast<const float*>(group);
        const float* mn = reinterpret_cast<const float*>(group + 0x10);
        GroupBound& g = gLineGroups[gLineGroupCount++];
        g.entry = reinterpret_cast<const void*>(ctx.rdx);
        for (int i = 0; i < 3; i++) { g.mn[i] = mn[i]; g.mx[i] = mx[i]; }
    }

    bool OutsideLineGroup(const void* entry, const float* v)
    {
        for (int i = 0; i < gLineGroupCount; i++)
        {
            if (gLineGroups[i].entry == entry)
            {
                const GroupBound& g = gLineGroups[i];
                return v[0] > g.mx[0] || v[0] < g.mn[0] || v[1] > g.mx[1] ||
                       v[1] < g.mn[1] || v[2] > g.mx[2] || v[2] < g.mn[2];
            }
        }
        return false;
    }

    void Spot_Hook(SafetyHookContext& ctx)  { Gate(ctx, 0x18, gSkipSpot,  false); }
    void Line_Hook(SafetyHookContext& ctx) { Gate(ctx, 0x28, gSkipLine, false); }
    void Black_Hook(SafetyHookContext& ctx) { Gate(ctx, 0x38, gSkipBlack, true); }

    // group: bound_max@0x00, bound_min@0x10, n_lights@0x20, type@0x24, lights@0x28
    SafetyHookInline gBoundHook{};

    size_t LightStride(int type)
    {
        if (type & 0x02) return 0x50;      // spot
        if (type & 0x04) return 0x50;      // line
        if (type & 0x10) return 0x40;      // black
        if (type & 0x20) return 0x40;      // the fifth type
        return 0;                          // point entries carry no authored box
    }

    void __fastcall BuildGroupBound(uint8_t* group)
    {
        if (!group)
        {
            gBoundHook.call<void>(group);
            return;
        }
        const int type = *reinterpret_cast<const int*>(group + 0x24);
        const int count = *reinterpret_cast<const int*>(group + 0x20);
        uint8_t* lights = *reinterpret_cast<uint8_t**>(group + 0x28);
        const size_t stride = LightStride(type);

        uint8_t groupBox[32];
        memcpy(groupBox, group, sizeof(groupBox));
        std::vector<uint8_t> lightBoxes;
        if (stride && lights && count > 0 && count < 4096)
        {
            lightBoxes.resize(static_cast<size_t>(count) * 32);
            for (int i = 0; i < count; i++)
            {
                memcpy(&lightBoxes[static_cast<size_t>(i) * 32], lights + i * stride, 32);
            }
        }

        gBoundHook.call<void>(group);

        memcpy(group, groupBox, sizeof(groupBox));
        for (size_t i = 0; i * 32 < lightBoxes.size(); i++)
        {
            memcpy(lights + i * stride, &lightBoxes[i * 32], 32);
        }
    }

    void SelEntry_Hook(SafetyHookContext&) { g_changeGroup = false; gLineGroupCount = 0; }



    // reached once a light group has passed its attribute and bounding tests; r13 -> group
    void GroupPass_Hook(SafetyHookContext& ctx)
    {
        const int type = *reinterpret_cast<const int*>(ctx.r13 + 0xc);
        if (type & kLitTypeChange)
        {
            g_changeGroup = true;
            g_anyChangeThisBake = true;
        }
    }

    // A PS2 kept a pre-baked copy of the lighting for anything that never changes. There is no
    // such copy here, so keep the one made at load and put it back for those models.
    std::mutex gCacheLock;
    struct Mapped { uint8_t* base; size_t bytes; };
    std::unordered_map<ID3D11Resource*, Mapped> gMapped;
    std::atomic<int> gMappedCount{ 0 };
    // Hold a reference to the buffer we key on: a freed one's address gets handed back, and the
    // bake we saved would then be copied over something else.
    struct Snapshot { ComPtr<ID3D11Resource> res; std::vector<uint8_t> data; };
    std::unordered_map<ID3D11Resource*, Snapshot> gSnapshot;
    struct { uint8_t* start; bool change; std::vector<uint8_t>* snap; size_t offset; } gModel{};

    SafetyHookInline gMapHook{};
    SafetyHookInline gUnmapHook{};

    HRESULT __stdcall HookedMap(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub,
        D3D11_MAP type, UINT flags, D3D11_MAPPED_SUBRESOURCE* out)
    {
        const HRESULT hr = gMapHook.call<HRESULT>(ctx, res, sub, type, flags, out);
        if (SUCCEEDED(hr) && out && out->pData && sub == 0)
        {
            // Ask the cheap question first - this runs on every map in the game, and asking for
            // the buffer interface up front costs a pair of refcount bumps every time.
            D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            res->GetType(&dim);
            if (dim == D3D11_RESOURCE_DIMENSION_BUFFER)
            {
                D3D11_BUFFER_DESC d;
                static_cast<ID3D11Buffer*>(res)->GetDesc(&d);
                if ((d.BindFlags & D3D11_BIND_VERTEX_BUFFER) && d.ByteWidth > 0x10000)
                {
                    std::lock_guard<std::mutex> lk(gCacheLock);
                    gMapped[res] = { static_cast<uint8_t*>(out->pData), d.ByteWidth };
                    gMappedCount.store(static_cast<int>(gMapped.size()), std::memory_order_release);
                }
            }
        }
        return hr;
    }

    void BakeEntry_Hook(SafetyHookContext&)
    {
        g_anyChangeThisBake = false;
        g_inBake = true;
    }
    void BakeExit_Hook(SafetyHookContext&)
    {
        g_inBake = false;
    }

    bool LooksLikeColourStream(const uint8_t* p, size_t bytes)
    {
        if (bytes < 0x10000 || bytes > 0x200000 || (bytes & 15) != 0)
        {
            return false;
        }
        const float* f = reinterpret_cast<const float*>(p);
        for (int v = 0; v < 8; v++)
        {
            if (f[v * 4 + 3] != 1.0f || f[v * 4] < 0.0f || f[v * 4] > 2.0f)
            {
                return false;
            }
        }
        return true;
    }

    void ModelGate_Hook(SafetyHookContext& ctx)
    {
        gModel = {};
        gModel.start = reinterpret_cast<uint8_t*>(ctx.rdi);
        gModel.change = g_changeGroup;
        if (!g_changeGroup)
        {
            std::lock_guard<std::mutex> lk(gCacheLock);
            for (auto& kv : gMapped)
            {
                uint8_t* base = kv.second.base;
                if (gModel.start >= base && gModel.start < base + kv.second.bytes)
                {
                    auto it = gSnapshot.find(kv.first);
                    if (it != gSnapshot.end() && !it->second.data.empty())
                    {
                        gModel.snap = &it->second.data;
                        gModel.offset = static_cast<size_t>(gModel.start - base);
                    }
                    break;
                }
            }
        }
    }

    void ModelEnd_Hook(SafetyHookContext& ctx)
    {
        if (gModel.change || !gModel.snap || !gModel.start)
        {
            return;
        }
        uint8_t* end = reinterpret_cast<uint8_t*>(ctx.rdi);
        if (end > gModel.start && gModel.offset + static_cast<size_t>(end - gModel.start) <= gModel.snap->size())
        {
            memcpy(gModel.start, gModel.snap->data() + gModel.offset, static_cast<size_t>(end - gModel.start));
        }
        gModel = {};
    }

    void __stdcall HookedUnmap(ID3D11DeviceContext* ctx, ID3D11Resource* res, UINT sub)
    {
        // Nothing tracked means nothing to look up, and this runs on every unmap in the game.
        if (gMappedCount.load(std::memory_order_acquire) != 0)
        {
            std::lock_guard<std::mutex> lk(gCacheLock);
            auto it = gMapped.find(res);
            if (it != gMapped.end() && g_inBake && LooksLikeColourStream(it->second.base, it->second.bytes))
            {
                auto& snap = gSnapshot[res];
                if (snap.data.empty())
                {
                    // first fill = the load-time bake with the stage's own rig
                    snap.res = res;
                    snap.data.assign(it->second.base, it->second.base + it->second.bytes);
                }
                else if (!g_anyChangeThisBake && snap.data.size() == it->second.bytes)
                {
                    // no changing light reached this map - pshade.c would not re-light it
                    memcpy(it->second.base, snap.data.data(), snap.data.size());
                }
                gMapped.erase(it);
                gMappedCount.store(static_cast<int>(gMapped.size()), std::memory_order_release);
            }
            else if (it != gMapped.end())
            {
                gMapped.erase(it);
                gMappedCount.store(static_cast<int>(gMapped.size()), std::memory_order_release);
            }
        }
        gUnmapHook.call<void>(ctx, res, sub);
    }


    struct GateSpec
    {
        const char* pattern;
        const char* label;
        const char* skipPattern;
        const char* skipLabel;
        SafetyHookMid* hook;
        uintptr_t* skip;
        void (*fn)(SafetyHookContext&);
    };

    const GateSpec kGates[] =
    {
        { "49 8B 6E ?? ?? ?? ?? ?? F3 44 0F 10 46 ?? F3 44 0F 10 4E ?? F3 0F 5C 74 2F ?? F3 44 0F 5C 44 2F ?? F3 44 0F 5C 4C 2F ?? 0F 28 C6 41 0F 28 D0 F3 0F 59 C6 F3 41 0F 59 D0 41 0F 28 C9 F3 41 0F 59 C9 F3 0F 58 D0 F3 0F 58 D1 0F 2F 54 2F ?? 0F 83 ?? ?? ?? ?? 0F 57 C0 0F 2E C2 77 ?? 0F 57 E4 F3 0F 51 E2 EB ?? 0F 28 C2 E8 ?? ?? ?? ?? 0F 28 E0 41 0F 28 D2 F3 0F 5E D4 0F 28 EA",
          "MGS 3: Map Relight: system\\libdg\\pshade.c -> spot light bound",
          "41 FF C7 48 81 C7 ?? ?? ?? ?? 45 3B 7E ?? 0F 8C ?? ?? ?? ?? 45 8B FD",
          "MGS 3: Map Relight: system\\libdg\\pshade.c -> spot light loop next",
          &gSpot, &gSkipSpot, Spot_Hook },
        { "49 8B 6E ?? F3 0F 10 76",
          "MGS 3: Map Relight: system\\libdg\\pshade.c -> line light bound",
          "41 FF C7 48 81 C7 ?? ?? ?? ?? 45 3B 7E ?? 0F 8C ?? ?? ?? ?? 44 0F 28 5C 24",
          "MGS 3: Map Relight: system\\libdg\\pshade.c -> line light loop next",
          &gLine, &gSkipLine, Line_Hook },
        { "49 8B 6E ?? ?? ?? ?? ?? F3 0F 10 56",
          "MGS 3: Map Relight: system\\libdg\\pshade.c -> black light bound",
          "41 FF C5 48 83 C7 ?? 44 3B E8",
          "MGS 3: Map Relight: system\\libdg\\pshade.c -> black light loop next",
          &gBlack, &gSkipBlack, Black_Hook },
    };
}

void MGS3MapRelight::Initialize()
{
    if (!(eGameType & MGS3))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 3: Map Relight: Disabled via config, skipping.");
        return;
    }

    for (const GateSpec& g : kGates)
    {
        uint8_t* head = Memory::PatternScan(baseModule, g.pattern, g.label);
        uint8_t* skip = Memory::PatternScan(baseModule, g.skipPattern, g.skipLabel);
        if (!head || !skip)
        {
            continue;
        }
        *g.skip = reinterpret_cast<uintptr_t>(skip);
        *g.hook = safetyhook::create_mid(head, g.fn);
        LOG_HOOK(*g.hook, g.label)
    }

    // ---- pshade.c hit_flag: only re-light models a changing light group reaches ----
    uint8_t* selEntry = Memory::PatternScan(baseModule,
        "48 8B C4 4C 89 40 ?? 53",
        "MGS 3: Map Relight: system\\libdg\\pshade.c -> CreateLightBuffer()");
    uint8_t* groupPass = Memory::PatternScan(baseModule,
        "41 8B 6D ?? 49 8B 75",
        "MGS 3: Map Relight: system\\libdg\\pshade.c -> CreateLightBuffer() group accepted");
    uint8_t* modelGate = Memory::PatternScan(baseModule, "41 8B 46 ?? ?? ?? ?? 89 44 24",
        "MGS 3: Map Relight: system\\libdg\\pshade.c -> ReshadeRGB() hit_flag");
    // Not the instruction before this one - the bake jumps there, into the middle of the patch.
    uint8_t* skipTo = Memory::PatternScan(baseModule,
        "41 FF CD 49 83 C1",
        "MGS 3: Map Relight: system\\libdg\\pshade.c -> ReshadeRGB() next model");
    if (uint8_t* bakeExit = Memory::PatternScan(baseModule,
        "48 8B 8D 90 00 00 00 48 33 CC E8 ?? ?? ?? ?? 4C 8D 9C 24 40 02 00 00 49 8B 5B 50 41 0F 28 73 F0",
        "MGS 3: Map Relight: system\\libdg\\pshade.c -> ShadeRGB() bake done"))
    {
        static SafetyHookMid done{};
        done = safetyhook::create_mid(bakeExit, BakeExit_Hook);
        LOG_HOOK(done, "MGS 3: Map Relight: system\\libdg\\pshade.c -> ShadeRGB() bake done")
    }

    if (uint8_t* bakeEntry = Memory::PatternScan(baseModule,
        "4C 8B DC 49 89 5B ?? 55 56 57 41 54 41 55 41 56 41 57 49 8D AB ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 41 0F 29 73",
        "MGS 3: Map Relight: system\\libdg\\pshade.c -> ShadeRGB() bake"))
    {
        static SafetyHookMid bake{};
        bake = safetyhook::create_mid(bakeEntry, BakeEntry_Hook);
        LOG_HOOK(bake, "MGS 3: Map Relight: system\\libdg\\pshade.c -> ShadeRGB() bake")
    }

    if (selEntry && groupPass && modelGate && skipTo)
    {
        gSkipModel = reinterpret_cast<uintptr_t>(skipTo);
        gSelEntry = safetyhook::create_mid(selEntry, SelEntry_Hook);
        LOG_HOOK(gSelEntry, "MGS 3: Map Relight: system\\libdg\\pshade.c -> CreateLightBuffer()")
        gGroupPass = safetyhook::create_mid(groupPass, GroupPass_Hook);
        LOG_HOOK(gGroupPass, "MGS 3: Map Relight: system\\libdg\\pshade.c -> CreateLightBuffer() group accepted")
        gModelGate = safetyhook::create_mid(modelGate, ModelGate_Hook);
        LOG_HOOK(gModelGate, "MGS 3: Map Relight: system\\libdg\\pshade.c -> ReshadeRGB() hit_flag")
        gModelEnd = safetyhook::create_mid(skipTo, ModelEnd_Hook);
        LOG_HOOK(gModelEnd, "MGS 3: Map Relight: system\\libdg\\pshade.c -> ReshadeRGB() next model")
    }
    else
    {
        spdlog::error("MGS 3: Map Relight: hit_flag sites not found, static bakes stay overwritten.");
    }



    if (uint8_t* build = Memory::PatternScan(baseModule,
        "48 8B C4 53 55 56 41 56 48 81 EC C8 00 00 00",
        "MGS 3: Map Relight: system\\libdg\\light.c -> light/group bound rebuild"))
    {
        gBoundHook = safetyhook::create_inline(build, reinterpret_cast<void*>(BuildGroupBound));
        LOG_HOOK(gBoundHook, "MGS 3: Map Relight: system\\libdg\\light.c -> light/group bound rebuild")
    }
}

// The vertex streams only exist once D3D is up; the bake's first fill is the authored look.
void MGS3MapRelight::OnDeviceReady()
{
    if (!(eGameType & MGS3) || !bEnabled || gMapHook)
    {
        return;
    }
    ID3D11DeviceContext* dc = g_D3D11Hooks.d3dDeviceContext.Get();
    if (!dc)
    {
        return;
    }
    void** vt = *reinterpret_cast<void***>(dc);
    gMapHook = safetyhook::create_inline(vt[14], reinterpret_cast<void*>(HookedMap));
    LOG_HOOK(gMapHook, "MGS 3: Map Relight: ID3D11DeviceContext::Map")
    gUnmapHook = safetyhook::create_inline(vt[15], reinterpret_cast<void*>(HookedUnmap));
    LOG_HOOK(gUnmapHook, "MGS 3: Map Relight: ID3D11DeviceContext::Unmap")
}
