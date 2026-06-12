#include "stdafx.h"
#include "depth_of_field.hpp"

#include "common.hpp"
#include "custom_resolution_and_borderless.hpp"
#include "gamevars.hpp"
#include "helper.hpp"
#ifndef RELEASE_BUILD
#include "input_handler.hpp"
#endif 
#include "logging.hpp"

namespace
{
    constexpr int kRegUvOffset0 = 96;
    constexpr int kRegUvOffset3 = 99;
    constexpr int kFarFocusMaxPlaneCount = 16;
    constexpr float kFarFocusBlurWeight0 = 4.0f;
    constexpr float kFarFocusBlurWeight12 = 0.2f;
    constexpr float kFarFocusBlurWeight34 = 0.03125f;
    constexpr float kFarFocusBlurWeight56 = 0.00625f;
    constexpr float kNearFocusMinDepthSpan = 0.00025f;
    constexpr int kNearFocusMinPlaneCount = 6;
    constexpr uint64_t kDepthFuncLEqual = 3;
    constexpr uint64_t kDepthFuncGEqual = 6;
    constexpr size_t kTrackedNearFocusPacketCount = 16;
    constexpr size_t kTrackedNearFocusWorkCount = 16;
    constexpr unsigned int kCmdPostFxFarFocus = 0x3F;
    constexpr int kDmapackNormal = 0x0001;
    constexpr int kDmapackInvisible123 = 0x0020 | 0x0040 | 0x0080;
    constexpr int kDmapackPhaseAfter = 0x04;
    constexpr ptrdiff_t kFocusWorkDisableOffset = 0x60;
    constexpr ptrdiff_t kFocusWorkMaxPlaneOffset = 0x64;
    constexpr ptrdiff_t kFocusWorkFocusNearOffset = 0x70;
    constexpr ptrdiff_t kFocusWorkFocusFarOffset = 0x74;
    constexpr ptrdiff_t kFocusWorkDmapackOffset = 0x80;
    constexpr ptrdiff_t kDmapackBpCallbackParamOffset = 0x28;
    constexpr ptrdiff_t kDmapackBpRenderCallbackOffset = 0x30;
    constexpr uint32_t kNearFocusSetId = 0x00BBAD24;
    constexpr uint32_t kNearFocusDemoId = 0x01000002;

    using BpRbAllocFn = void*(__fastcall*)(int);
    using BpRbAddCommandFn = void(__fastcall*)(unsigned int, void*);
    using DmapackRenderCallbackFn = void(__fastcall*)(void*);
    using SetDepthFuncFn = void(__fastcall*)(void*, uint64_t);

    bool bCutsceneNeedsSpecialHandling = false; //for per-cutscene effect skip handling.
    bool bIsD12T3 = false;
    int iNearEffectCount = 0;


    struct FocusSourcePacket
    {
        int maxPlane;
        float focusNear;
        float focusFar;
    };

    struct TrackedFocusPacket
    {
        uintptr_t address = 0;
        FocusSourcePacket packet {};
    };

    struct TrackedNearFocusWork
    {
        uintptr_t work = 0;
        uintptr_t dmapack = 0;
        uintptr_t originalParam = 0;
        DmapackRenderCallbackFn originalRender = nullptr;
    };

    SafetyHookInline SetVertexRegistersHook {};
    SafetyHookMid FarFocusBlurBeginHook {};
    SafetyHookMid FarFocusBlurEndHook {};
    SafetyHookMid NearFocusDepthFuncHook {};
    SafetyHookInline NearFocusSetHook {};
    SafetyHookInline NearFocusDemoHook {};
    SafetyHookInline BpRbAddCommandHook {};
    SafetyHookInline FarFocusCommandHook {};
    std::array<TrackedFocusPacket, kTrackedNearFocusPacketCount> gNearFocusPackets {};
    std::array<TrackedNearFocusWork, kTrackedNearFocusWorkCount> gNearFocusWorks {};
    size_t gNearFocusPacketWriteIndex = 0;
    size_t gNearFocusWorkWriteIndex = 0;
    BpRbAllocFn gBpRbAlloc = nullptr;
    BpRbAddCommandFn gBpRbAddCommand = nullptr;
    DmapackRenderCallbackFn gNearFocusOriginalRender = nullptr;
    SetDepthFuncFn gSetDepthFunc = nullptr;
    void* gDepthFuncBackend = nullptr;
    thread_local bool gInsideFarFocusBlur = false;
    thread_local bool gInsideNearFocusComposite = false;
    thread_local bool gExecutingNearFocusCommand = false;
    thread_local bool gInsideNearFocusAddCommand = false;
    thread_local bool gInsideOriginalNearFocusCallback = false;
    thread_local bool gOriginalNearFocusCommandSeen = false;
    thread_local uintptr_t gActiveNearFocusWork = 0;
    thread_local const FocusSourcePacket* gActiveNearFocusSource = nullptr;

    bool QueueNearFocusPacket(void* work);
    bool QueueNearFocusPacketFromSource(FocusSourcePacket source);
    bool BuildNearFocusSourceFromParam(uintptr_t paramAddress, FocusSourcePacket& source);

    bool IsUltrawide()
    {
        if (CustomResolutionAndBorderless::iInternalResX <= 0 || CustomResolutionAndBorderless::iInternalResY <= 0)
        {
            return false;
        }

        constexpr float nativeAspect = 16.0f / 9.0f;
        const float aspect = static_cast<float>(CustomResolutionAndBorderless::iInternalResX) / static_cast<float>(CustomResolutionAndBorderless::iInternalResY);
        return aspect > nativeAspect;
    }

    bool LooksLikeBlurUvOffset(const float* regs)
    {
        if (!regs)
        {
            return false;
        }

        float maxAbs = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            if (!std::isfinite(regs[i]))
            {
                return false;
            }

            maxAbs = std::max(maxAbs, std::abs(regs[i]));
        }

        return maxAbs > 0.000001f && maxAbs <= 0.08f;
    }

    void ScaleActiveBlurAxis(float* regs, float multiplier)
    {
        const bool horizontalPass = (std::abs(regs[0]) > std::abs(regs[1])) || (std::abs(regs[2]) > std::abs(regs[3]));
        const bool verticalPass = (std::abs(regs[1]) > std::abs(regs[0])) || (std::abs(regs[3]) > std::abs(regs[2]));

        if (horizontalPass && !verticalPass)
        {
            regs[0] *= multiplier;
            regs[2] *= multiplier;
            return;
        }

        if (verticalPass && !horizontalPass)
        {
            regs[1] *= multiplier;
            regs[3] *= multiplier;
            return;
        }

        for (int i = 0; i < 4; ++i)
        {
            regs[i] *= multiplier;
        }
    }

    bool IsReasonableFocusSourcePacket(const FocusSourcePacket* packet)
    {
        return packet &&
               packet->maxPlane >= 2 &&
               packet->maxPlane <= 64 &&
               std::isfinite(packet->focusNear) &&
               std::isfinite(packet->focusFar) &&
               packet->focusNear > 0.0f &&
               packet->focusNear < 1.0f &&
               packet->focusFar > 0.0f &&
               packet->focusFar < 1.0f &&
               packet->focusNear > packet->focusFar;
    }

    float NormalizeDepthValue(float depth)
    {
        if (!std::isfinite(depth))
        {
            return depth;
        }

        if (depth < 0.0f)
        {
            depth = (depth + 1.0f) * 0.5f;
        }

        return std::clamp(depth, 0.000001f, 0.999999f);
    }

    bool SameFocusPacket(const FocusSourcePacket& lhs, const FocusSourcePacket& rhs)
    {
        return lhs.maxPlane == rhs.maxPlane &&
               lhs.focusNear == rhs.focusNear &&
               lhs.focusFar == rhs.focusFar;
    }

    void WidenNearFocusPacket(FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return;
        }

        const float span = std::abs(packet->focusNear - packet->focusFar);
        if (span >= kNearFocusMinDepthSpan)
        {
            return;
        }

        const float center = (packet->focusNear + packet->focusFar) * 0.5f;
        const float halfSpan = kNearFocusMinDepthSpan * 0.5f;
        packet->focusNear = std::clamp(center + halfSpan, 0.000001f, 0.999999f);
        packet->focusFar = std::clamp(center - halfSpan, 0.000001f, 0.999999f);
    }

    bool PrepareFocusSource(FocusSourcePacket& source)
    {
        if (source.maxPlane < 1 ||
            source.maxPlane > 64 ||
            !std::isfinite(source.focusNear) ||
            !std::isfinite(source.focusFar))
        {
            return false;
        }

        source.maxPlane = std::max(source.maxPlane, kNearFocusMinPlaneCount);
        source.focusNear = NormalizeDepthValue(source.focusNear);
        source.focusFar = NormalizeDepthValue(source.focusFar);

        if (source.focusNear < source.focusFar)
        {
            std::swap(source.focusNear, source.focusFar);
        }

        if (source.focusNear <= source.focusFar)
        {
            const float center = (source.focusNear + source.focusFar) * 0.5f;
            const float halfSpan = kNearFocusMinDepthSpan * 0.5f;
            source.focusNear = std::clamp(center + halfSpan, 0.000001f, 0.999999f);
            source.focusFar = std::clamp(center - halfSpan, 0.000001f, 0.999999f);
        }

        WidenNearFocusPacket(&source);
        return IsReasonableFocusSourcePacket(&source);
    }

    ptrdiff_t ModuleOffset(uintptr_t address)
    {
        const uintptr_t moduleBase = reinterpret_cast<uintptr_t>(baseModule);
        return address >= moduleBase ? static_cast<ptrdiff_t>(address - moduleBase) : -1;
    }

    std::vector<uintptr_t> FindStageEntryFunctions(uint32_t id)
    {
        std::vector<uintptr_t> functions {};
        auto* dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(baseModule);
        auto* ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uint8_t*>(baseModule) + dosHeader->e_lfanew);
        const size_t sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto* scanBytes = reinterpret_cast<uint8_t*>(baseModule);

        for (size_t i = 0; i + 16 <= sizeOfImage; ++i)
        {
            if (*reinterpret_cast<uint32_t*>(scanBytes + i) != id)
            {
                continue;
            }

            const uintptr_t function = *reinterpret_cast<uintptr_t*>(scanBytes + i + 8);
            if (!Memory::IsExecutable(reinterpret_cast<void*>(function)))
            {
                continue;
            }

            if (std::find(functions.begin(), functions.end(), function) == functions.end())
            {
                functions.push_back(function);
            }
        }

        return functions;
    }

    uint8_t* FindFunctionStart(uint8_t* interior)
    {
        if (!interior)
        {
            return nullptr;
        }

        constexpr size_t searchBack = 0x1000;
        for (size_t offset = 1; offset < searchBack; ++offset)
        {
            uint8_t* candidate = interior - offset;
            if (!Memory::IsReadable(candidate - 1, 17))
            {
                continue;
            }

            if (candidate[-1] != 0xCC || candidate[0] == 0xCC)
            {
                continue;
            }

            if (candidate[0] == 0x40 || candidate[0] == 0x48 || candidate[0] == 0x4C || candidate[0] == 0x53 ||
                candidate[0] == 0x55 || candidate[0] == 0x56 || candidate[0] == 0x57)
            {
                return candidate;
            }
        }

        return nullptr;
    }

    TrackedNearFocusWork* FindTrackedNearFocusWork(uintptr_t work)
    {
        for (TrackedNearFocusWork& tracked : gNearFocusWorks)
        {
            if (tracked.work == work)
            {
                return &tracked;
            }
        }

        return nullptr;
    }

    bool IsTrackedNearFocusWork(uintptr_t work)
    {
        return FindTrackedNearFocusWork(work) != nullptr;
    }

    TrackedNearFocusWork& TrackNearFocusWorkAddress(uintptr_t work, uintptr_t dmapack)
    {
        if (TrackedNearFocusWork* tracked = FindTrackedNearFocusWork(work))
        {
            tracked->dmapack = dmapack;
            return *tracked;
        }

        TrackedNearFocusWork& tracked = gNearFocusWorks[gNearFocusWorkWriteIndex++ % gNearFocusWorks.size()];
        tracked = {};
        tracked.work = work;
        tracked.dmapack = dmapack;
        return tracked;
    }

    void NearFocusRenderCallback(void* work)
    {
        bool originalCommandQueued = false;

        auto runOriginalCallback = [&](DmapackRenderCallbackFn callback, void* callbackParam, uintptr_t activeWork, const FocusSourcePacket* activeSource) {
            const bool previousInsideCallback = gInsideOriginalNearFocusCallback;
            const bool previousCommandSeen = gOriginalNearFocusCommandSeen;
            const uintptr_t previousActiveWork = gActiveNearFocusWork;
            const FocusSourcePacket* previousActiveSource = gActiveNearFocusSource;

            gInsideOriginalNearFocusCallback = true;
            gOriginalNearFocusCommandSeen = false;
            gActiveNearFocusWork = activeWork;
            gActiveNearFocusSource = activeSource;

            callback(callbackParam);
            const bool commandSeen = gOriginalNearFocusCommandSeen;

            gInsideOriginalNearFocusCallback = previousInsideCallback;
            gOriginalNearFocusCommandSeen = previousCommandSeen;
            gActiveNearFocusWork = previousActiveWork;
            gActiveNearFocusSource = previousActiveSource;

            return commandSeen;
        };

        if (TrackedNearFocusWork* tracked = FindTrackedNearFocusWork(reinterpret_cast<uintptr_t>(work)))
        {
            if (tracked->originalRender)
            {
                originalCommandQueued = runOriginalCallback(
                    tracked->originalRender,
                    reinterpret_cast<void*>(tracked->originalParam),
                    tracked->work,
                    nullptr);
            }

            if (!originalCommandQueued)
            {
                QueueNearFocusPacket(reinterpret_cast<void*>(tracked->work));
            }

            return;
        }

        FocusSourcePacket bufferedSource {};
        if (gNearFocusOriginalRender &&
            BuildNearFocusSourceFromParam(reinterpret_cast<uintptr_t>(work), bufferedSource))
        {
            originalCommandQueued = runOriginalCallback(gNearFocusOriginalRender, work, 0, &bufferedSource);
            if (!originalCommandQueued)
            {
                QueueNearFocusPacketFromSource(bufferedSource);
            }
            return;
        }

        QueueNearFocusPacket(work);
    }

    bool LooksLikeNearFocusDmapack(uintptr_t dmapack)
    {
        if (!dmapack ||
            !Memory::IsReadable(reinterpret_cast<void*>(dmapack), kDmapackBpRenderCallbackOffset + sizeof(uintptr_t)))
        {
            return false;
        }

        const int flag = Memory::ReadField<int>(dmapack, 0x00);
        const int16_t phase = Memory::ReadField<int16_t>(dmapack, 0x04);
        const int16_t priority = Memory::ReadField<int16_t>(dmapack, 0x06);

        if ((flag & kDmapackNormal) == 0 ||
            (flag & kDmapackInvisible123) != kDmapackInvisible123 ||
            phase != kDmapackPhaseAfter ||
            priority < 0 ||
            priority > 255)
        {
            return false;
        }

        return Memory::IsWritable(reinterpret_cast<void*>(dmapack + kDmapackBpCallbackParamOffset), sizeof(uintptr_t)) &&
               Memory::IsWritable(reinterpret_cast<void*>(dmapack + kDmapackBpRenderCallbackOffset), sizeof(uintptr_t));
    }

    bool LooksLikeNearFocusWork(uintptr_t workAddress)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(workAddress), kFocusWorkDmapackOffset + sizeof(uintptr_t)))
        {
            return false;
        }

        const int disable = Memory::ReadField<int>(workAddress, kFocusWorkDisableOffset);
        const int maxPlane = Memory::ReadField<int>(workAddress, kFocusWorkMaxPlaneOffset);
        const float focusNear = Memory::ReadField<float>(workAddress, kFocusWorkFocusNearOffset);
        const float focusFar = Memory::ReadField<float>(workAddress, kFocusWorkFocusFarOffset);

        if ((disable != 0 && disable != 1) ||
            maxPlane < 1 ||
            maxPlane > 64 ||
            !std::isfinite(focusNear) ||
            !std::isfinite(focusFar) ||
            std::abs(focusNear) > 16.0f ||
            std::abs(focusFar) > 16.0f)
        {
            return false;
        }

        return LooksLikeNearFocusDmapack(Memory::ReadField<uintptr_t>(workAddress, kFocusWorkDmapackOffset));
    }

    void InstallNearFocusDmapackCallback(void* work)
    {
        const uintptr_t workAddress = reinterpret_cast<uintptr_t>(work);
        const uintptr_t dmapack = Memory::ReadField<uintptr_t>(workAddress, kFocusWorkDmapackOffset);
        if (!LooksLikeNearFocusDmapack(dmapack))
        {
            return;
        }

        const uintptr_t originalParam = Memory::ReadField<uintptr_t>(dmapack, kDmapackBpCallbackParamOffset);
        const uintptr_t originalRender = Memory::ReadField<uintptr_t>(dmapack, kDmapackBpRenderCallbackOffset);
        TrackedNearFocusWork& tracked = TrackNearFocusWorkAddress(workAddress, dmapack);

        if (originalRender != reinterpret_cast<uintptr_t>(NearFocusRenderCallback))
        {
            tracked.originalParam = originalParam;
            tracked.originalRender = Memory::IsExecutable(reinterpret_cast<void*>(originalRender))
                ? reinterpret_cast<DmapackRenderCallbackFn>(originalRender)
                : nullptr;

            if (!gNearFocusOriginalRender && tracked.originalRender)
            {
                gNearFocusOriginalRender = tracked.originalRender;
            }
        }

        *reinterpret_cast<uintptr_t*>(dmapack + kDmapackBpCallbackParamOffset) = workAddress;
        *reinterpret_cast<uintptr_t*>(dmapack + kDmapackBpRenderCallbackOffset) = reinterpret_cast<uintptr_t>(NearFocusRenderCallback);

        static bool logged = false;
        if (!logged)
        {
            logged = true;
            spdlog::info("MGS 2: Depth of Field: near focus callback installed.");
        }
    }

    void TrackNearFocusWork(void* work)
    {
        const uintptr_t workAddress = reinterpret_cast<uintptr_t>(work);
        if (!workAddress)
        {
            return;
        }

        if (!LooksLikeNearFocusWork(workAddress))
        {
            return;
        }

        InstallNearFocusDmapackCallback(work);
    }

    bool BuildNearFocusSourceFromWork(uintptr_t workAddress, FocusSourcePacket& source)
    {
        if (!IsTrackedNearFocusWork(workAddress) ||
            Memory::ReadField<int>(workAddress, kFocusWorkDisableOffset) != 0)
        {
            return false;
        }

        source.maxPlane = std::max(Memory::ReadField<int>(workAddress, kFocusWorkMaxPlaneOffset), kNearFocusMinPlaneCount);
        source.focusNear = NormalizeDepthValue(Memory::ReadField<float>(workAddress, kFocusWorkFocusNearOffset));
        source.focusFar = NormalizeDepthValue(Memory::ReadField<float>(workAddress, kFocusWorkFocusFarOffset));

        return PrepareFocusSource(source);
    }

    bool BuildNearFocusSourceFromParam(uintptr_t paramAddress, FocusSourcePacket& source)
    {
        if (!Memory::IsReadable(reinterpret_cast<void*>(paramAddress), sizeof(FocusSourcePacket)))
        {
            return false;
        }

        source = *reinterpret_cast<const FocusSourcePacket*>(paramAddress);
        return PrepareFocusSource(source);
    }

    void TrackNearFocusPacket(const FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return;
        }

        TrackedFocusPacket& tracked = gNearFocusPackets[gNearFocusPacketWriteIndex++ % gNearFocusPackets.size()];
        tracked.address = reinterpret_cast<uintptr_t>(packet);
        tracked.packet = *packet;
    }

    bool ConsumeNearFocusPacket(const FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return false;
        }

        const uintptr_t address = reinterpret_cast<uintptr_t>(packet);
        for (TrackedFocusPacket& tracked : gNearFocusPackets)
        {
            if (tracked.address == address && SameFocusPacket(tracked.packet, *packet))
            {
                tracked.address = 0;
                return true;
            }
        }

        return false;
    }

    bool IsTrackedNearFocusPacket(const FocusSourcePacket* packet)
    {
        if (!IsReasonableFocusSourcePacket(packet))
        {
            return false;
        }

        const uintptr_t address = reinterpret_cast<uintptr_t>(packet);
        for (const TrackedFocusPacket& tracked : gNearFocusPackets)
        {
            if (tracked.address == address && SameFocusPacket(tracked.packet, *packet))
            {
                return true;
            }
        }

        return false;
    }

    void RestoreDefaultDepthFunc()
    {
        if (!gSetDepthFunc || !gDepthFuncBackend)
        {
            return;
        }

        gSetDepthFunc(gDepthFuncBackend, kDepthFuncGEqual);
    }

    void __fastcall FarFocusCommand_Hook(void* packet)
    {
        auto* focusPacket = static_cast<FocusSourcePacket*>(packet);
        const bool isNearFocusPacket = IsTrackedNearFocusPacket(focusPacket);
        const bool previousExecutingNearFocus = gExecutingNearFocusCommand;

        gExecutingNearFocusCommand = isNearFocusPacket;
        FarFocusCommandHook.fastcall<void>(packet);

        if (isNearFocusPacket)
        {
            RestoreDefaultDepthFunc();
            ConsumeNearFocusPacket(focusPacket);
            gInsideNearFocusComposite = false;
        }

        gExecutingNearFocusCommand = previousExecutingNearFocus;
    }

    bool QueueNearFocusPacketFromSource(FocusSourcePacket source)
    {
        if (!gBpRbAlloc || !gBpRbAddCommand || gInsideNearFocusAddCommand)
        {
            return false;
        }
    //    if (bCutsceneNeedsSpecialHandling)
        {
            if (bIsD12T3)
            {
                iNearEffectCount++;
                if (iNearEffectCount >= 80 && iNearEffectCount < 440)
                {
                    spdlog::info("MGS 2: Depth of Field: skipping near focus packet {:d} for cutscene special handling.", iNearEffectCount);
                    return false;
                }
            }
        }

        if (!PrepareFocusSource(source))
        {
            return false;
        }

        auto* packet = static_cast<FocusSourcePacket*>(gBpRbAlloc(sizeof(FocusSourcePacket)));
        if (!packet)
        {
            return false;
        }

        *packet = source;
        WidenNearFocusPacket(packet);
        TrackNearFocusPacket(packet);

        gInsideNearFocusAddCommand = true;
        gBpRbAddCommand(kCmdPostFxFarFocus, packet);
        gInsideNearFocusAddCommand = false;

        return true;
    }

    bool QueueNearFocusPacket(void* work)
    {
        FocusSourcePacket source {};
        if (!BuildNearFocusSourceFromWork(reinterpret_cast<uintptr_t>(work), source))
        {
            return false;
        }

        return QueueNearFocusPacketFromSource(source);
    }

    bool PrepareOriginalNearFocusCommand(FocusSourcePacket* packet)
    {
        if (!packet || !Memory::IsReadable(packet, sizeof(FocusSourcePacket)) || !Memory::IsWritable(packet, sizeof(FocusSourcePacket)))
        {
            return false;
        }

        FocusSourcePacket source = *packet;
        if (!PrepareFocusSource(source))
        {
            if (gActiveNearFocusSource)
            {
                source = *gActiveNearFocusSource;
            }
            else if (gActiveNearFocusWork)
            {
                if (!BuildNearFocusSourceFromWork(gActiveNearFocusWork, source))
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }

        if (!PrepareFocusSource(source))
        {
            return false;
        }

        *packet = source;
        TrackNearFocusPacket(packet);
        return true;
    }

    void __fastcall BpRbAddCommand_Hook(unsigned int command, void* packet)
    {
        if (gInsideOriginalNearFocusCallback && command == kCmdPostFxFarFocus)
        {
            if (PrepareOriginalNearFocusCommand(static_cast<FocusSourcePacket*>(packet)))
            {
                gOriginalNearFocusCommandSeen = true;
            }
        }

        BpRbAddCommandHook.fastcall<void>(command, packet);
    }

    void* __fastcall NearFocusSet_Hook(int name, int map)
    {
        void* result = NearFocusSetHook.fastcall<void*>(name, map);
        TrackNearFocusWork(result);
        return result;
    }

    void* __fastcall NearFocusDemo_Hook(int id, void* argv)
    {
        void* result = NearFocusDemoHook.fastcall<void*>(id, argv);
        TrackNearFocusWork(result);
        return result;
    }

    uint8_t* FindFarFocusBlurCallSetup(const char* label)
    {
        return Memory::PatternScan(
            baseModule,
            "F3 44 0F 11 44 24 ?? E8 ?? ?? ?? ?? 48 8B 0D",
            label);
    }

    uint8_t* FindFarFocusMaxPlaneClamp(const char* label)
    {
        return Memory::PatternScan(
            baseModule,
            "39 1D ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 0F 4C 1D ?? ?? ?? ?? 89 9D",
            label);
    }

    bool ResolveRenderBufferHelpers()
    {
        const auto commands = Memory::FindMultiplePatternMatches(
            baseModule,
            "B9 0C 00 00 00 E8 ?? ?? ?? ?? 8B 53 64 89 10 8B 53 70 89 50 04 48 8B D0 8B 4B 74 89 48 08 B9 3F 00 00 00 48 83 C4 20 5B E9");

        if (commands.empty())
        {
            spdlog::error("MGS 2: Depth of Field: focus packet command pattern scan failed.");
            return false;
        }

        uint8_t* allocCall = commands.front() + 0x05;
        uint8_t* addCommandJump = commands.front() + 0x28;
        if (*allocCall != 0xE8 || *addCommandJump != 0xE9)
        {
            spdlog::error("MGS 2: Depth of Field: render-buffer helper call sites were not found.");
            return false;
        }

        gBpRbAlloc = reinterpret_cast<BpRbAllocFn>(Memory::GetRelativeOffset(allocCall + 1));
        gBpRbAddCommand = reinterpret_cast<BpRbAddCommandFn>(Memory::GetRelativeOffset(addCommandJump + 1));

        if (!BpRbAddCommandHook)
        {
            BpRbAddCommandHook = safetyhook::create_inline(
                reinterpret_cast<void*>(gBpRbAddCommand),
                reinterpret_cast<void*>(BpRbAddCommand_Hook));
            LOG_HOOK(BpRbAddCommandHook, "MGS 2: Depth of Field: render-buffer add command")
        }

        spdlog::info("MGS 2: Depth of Field: render-buffer helpers resolved at {:s}+{:X} and {:s}+{:X}.",
                     sExeName.c_str(),
                     ModuleOffset(reinterpret_cast<uintptr_t>(gBpRbAlloc)),
                     sExeName.c_str(),
                     ModuleOffset(reinterpret_cast<uintptr_t>(gBpRbAddCommand)));
        return true;
    }

    void InstallNearFocusCreationHooks()
    {
        const std::vector<uintptr_t> nearSetFunctions = FindStageEntryFunctions(kNearFocusSetId);
        if (!nearSetFunctions.empty())
        {
            NearFocusSetHook = safetyhook::create_inline(reinterpret_cast<void*>(nearSetFunctions.front()), reinterpret_cast<void*>(NearFocusSet_Hook));
            LOG_HOOK(NearFocusSetHook, "MGS 2: Depth of Field: near focus set")
        }
        else
        {
            spdlog::warn("MGS 2: Depth of Field: near focus set stage entry was not found.");
        }

        const std::vector<uintptr_t> nearDemoFunctions = FindStageEntryFunctions(kNearFocusDemoId);
        if (!nearDemoFunctions.empty())
        {
            NearFocusDemoHook = safetyhook::create_inline(reinterpret_cast<void*>(nearDemoFunctions.front()), reinterpret_cast<void*>(NearFocusDemo_Hook));
            LOG_HOOK(NearFocusDemoHook, "MGS 2: Depth of Field: near focus demo")
        }
        else
        {
            spdlog::warn("MGS 2: Depth of Field: near focus demo stage entry was not found.");
        }
    }

    void __fastcall SetVertexRegisters_Hook(void* backend, int startRegister, int numVectors, const float* regs)
    {
        thread_local float scaledRegs[4];
        if (gInsideFarFocusBlur &&
            startRegister > kRegUvOffset0 &&
            startRegister <= kRegUvOffset3 &&
            numVectors == 1 &&
            LooksLikeBlurUvOffset(regs))
        {
            std::copy(regs, regs + 4, scaledRegs);
            ScaleActiveBlurAxis(scaledRegs, g_DepthOfFieldFixes.fBlurUvMultiplier);

            SetVertexRegistersHook.fastcall<void>(backend, startRegister, numVectors, scaledRegs);
            return;
        }

        SetVertexRegistersHook.fastcall<void>(backend, startRegister, numVectors, regs);
    }

    void InstallFarFocusBlurScopeHooks()
    {
        uint8_t* farFocusBlurCallSetup = FindFarFocusBlurCallSetup("MGS 2: Depth of Field: far focus blur scope");

        if (!farFocusBlurCallSetup)
        {
            return;
        }

        FarFocusBlurBeginHook = safetyhook::create_mid(farFocusBlurCallSetup, [](SafetyHookContext&) {
            gInsideFarFocusBlur = true;
        });
        LOG_HOOK(FarFocusBlurBeginHook, "MGS 2: Depth of Field: far focus blur scope begin")

        FarFocusBlurEndHook = safetyhook::create_mid(farFocusBlurCallSetup + 0x0C, [](SafetyHookContext&) {
            gInsideFarFocusBlur = false;
            gInsideNearFocusComposite = false;
        });
        LOG_HOOK(FarFocusBlurEndHook, "MGS 2: Depth of Field: far focus blur scope end")
    }

    bool InstallNearFocusDepthFuncHook()
    {
        uint8_t* maxPlaneClamp = FindFarFocusMaxPlaneClamp("MGS 2: Depth of Field: near focus depth function");
        if (!maxPlaneClamp)
        {
            return false;
        }

        uint8_t* setDepthFuncCall = maxPlaneClamp - 0x1B;
        if (*setDepthFuncCall != 0xE8)
        {
            spdlog::error("MGS 2: Depth of Field: expected far-focus SetDepthFunc call was not found; near focus disabled.");
            return false;
        }

        gSetDepthFunc = reinterpret_cast<SetDepthFuncFn>(Memory::GetRelativeOffset(setDepthFuncCall + 1));

        NearFocusDepthFuncHook = safetyhook::create_mid(setDepthFuncCall, [](SafetyHookContext& ctx) {
            auto* packet = reinterpret_cast<FocusSourcePacket*>(ctx.r13);
            if (gExecutingNearFocusCommand || IsTrackedNearFocusPacket(packet))
            {
                gDepthFuncBackend = reinterpret_cast<void*>(ctx.rcx);
                ctx.rdx = kDepthFuncLEqual;
                gInsideNearFocusComposite = true;
            }
        });
        LOG_HOOK(NearFocusDepthFuncHook, "MGS 2: Depth of Field: near focus depth function")

        return static_cast<bool>(NearFocusDepthFuncHook);
    }

    bool InstallFarFocusCommandHook()
    {
        uint8_t* maxPlaneClamp = FindFarFocusMaxPlaneClamp("MGS 2: Depth of Field: far focus command");
        if (!maxPlaneClamp)
        {
            return false;
        }

        uint8_t* farFocusCommand = FindFunctionStart(maxPlaneClamp);
        if (!farFocusCommand)
        {
            spdlog::warn("MGS 2: Depth of Field: far focus command function start was not found; depth restore disabled.");
            return false;
        }

        FarFocusCommandHook = safetyhook::create_inline(farFocusCommand, reinterpret_cast<void*>(FarFocusCommand_Hook));
        LOG_HOOK(FarFocusCommandHook, "MGS 2: Depth of Field: far focus command")
        spdlog::info("MGS 2: Depth of Field: far focus command hook target {:s}+{:X}.",
                     sExeName.c_str(),
                     ModuleOffset(reinterpret_cast<uintptr_t>(farFocusCommand)));

        return static_cast<bool>(FarFocusCommandHook);
    }

    void ForceFarFocusBlurEnabled()
    {
        uint8_t* blurGate = Memory::PatternScan(
            baseModule,
            "83 3D ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? F3 0F 10 15 ?? ?? ?? ?? 45 0F 28 D4",
            "MGS 2: Depth of Field: far focus blur enable gate");

        if (!blurGate)
        {
            return;
        }

        uintptr_t blurEnableAddress = Memory::GetRipRelativeAddress(blurGate, 0x02, 0x07);
        Memory::Write<int>(blurEnableAddress, 1);

        spdlog::info("MGS 2: Depth of Field: far focus blur enabled at {:s}+{:X}.",
                     sExeName.c_str(),
                     blurEnableAddress - reinterpret_cast<uintptr_t>(baseModule));
    }

    void ForceFarFocusMaxPlaneCount()
    {
        uint8_t* maxPlaneClamp = FindFarFocusMaxPlaneClamp("MGS 2: Depth of Field: far focus max plane count");

        if (!maxPlaneClamp)
        {
            return;
        }

        uintptr_t maxPlaneCountAddress = Memory::GetRipRelativeAddress(maxPlaneClamp, 0x02, 0x06);
        Memory::Write<int>(maxPlaneCountAddress, kFarFocusMaxPlaneCount);
        Memory::Write<float>(maxPlaneCountAddress + 0x04, kFarFocusBlurWeight0);
        Memory::Write<float>(maxPlaneCountAddress + 0x08, kFarFocusBlurWeight12);
        Memory::Write<float>(maxPlaneCountAddress + 0x0C, kFarFocusBlurWeight34);
        Memory::Write<float>(maxPlaneCountAddress + 0x10, kFarFocusBlurWeight56);

        spdlog::info("MGS 2: Depth of Field: far focus max plane count set to {} at {:s}+{:X}.",
                     kFarFocusMaxPlaneCount,
                     sExeName.c_str(),
                     maxPlaneCountAddress - reinterpret_cast<uintptr_t>(baseModule));
        spdlog::info("MGS 2: Depth of Field: far focus blur weights set to {}, {}, {}, {}.",
                     kFarFocusBlurWeight0,
                     kFarFocusBlurWeight12,
                     kFarFocusBlurWeight34,
                     kFarFocusBlurWeight56);
    }

    void InstallBlurUvScaleHook()
    {
        uint8_t* blurUvRegisterUpload = Memory::PatternScan(
            baseModule,
            "48 8B 0D ?? ?? ?? ?? 4C 8D 4D ?? 8D 57 60 44 8D 47 01 E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 4C 8D 4C 24 ?? 8D 57 61 44 8D 47 01 E8",
            "MGS 2: Depth of Field: blur UV register upload");

        if (!blurUvRegisterUpload)
        {
            return;
        }

        uint8_t* setVertexRegistersCall = blurUvRegisterUpload + 0x12;
        if (*setVertexRegistersCall != 0xE8)
        {
            spdlog::error("MGS 2: Depth of Field: expected SetVertexRegisters call was not found; blur UV scale disabled.");
            return;
        }

        const uintptr_t setVertexRegisters = Memory::GetRelativeOffset(setVertexRegistersCall + 1);
        SetVertexRegistersHook = safetyhook::create_inline(reinterpret_cast<void*>(setVertexRegisters), reinterpret_cast<void*>(SetVertexRegisters_Hook));
        LOG_HOOK(SetVertexRegistersHook, "MGS 2: Depth of Field: blur UV register scale")
    }
}

void DepthOfFieldFixes::HandleLevelTransition()
{
    if (!bEnabled)
    {
        return;
    }
    iNearEffectCount = 0;
    bIsD12T3 = g_GameVars.IsStage(MGS2Stages::D12T3);
}

void DepthOfFieldFixes::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }

    if (!bEnabled)
    {
        spdlog::info("MGS 2: Depth of Field: disabled by config.");
        return;
    }

    if (IsUltrawide())
    {
        bEnabled = false;
        spdlog::info("MGS 2: Depth of Field: disabled for ultrawide aspect ratio.");
        return;
    }

    fBlurUvMultiplier = std::clamp(fBlurUvMultiplier, 0.0f, 20.0f);
    spdlog::info("MGS 2: Depth of Field: blur UV multiplier set to {}.", fBlurUvMultiplier);

    ForceFarFocusBlurEnabled();
    ForceFarFocusMaxPlaneCount();
    InstallFarFocusBlurScopeHooks();
    if (ResolveRenderBufferHelpers() && InstallNearFocusDepthFuncHook())
    {
        InstallFarFocusCommandHook();
        InstallNearFocusCreationHooks();
    }
    else
    {
        spdlog::warn("MGS 2: Depth of Field: failed to resolve necessary functions for near focus fixes; near focus adjustments disabled.");
    }
    InstallBlurUvScaleHook();

#ifndef RELEASE_BUILD
    g_InputHandler.RegisterHotkey(VK_ADD, "print", []
                                  {
                                      spdlog::info("iNearEffectCount = {}, bIsD12T3 = {}", iNearEffectCount, bIsD12T3);
                                  });

    /*
    MAKE_HOOK_MID(baseModule, "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC ?? 41 8B F9 41 8B F0 45 33 C9 8B EA 44 8B F1 BA ?? ?? ?? ?? 41 B8 ?? ?? ?? ?? 41 8D 49 ?? E8 A4 1C B2 FF", "NewNearFocusEffect -> Focal Points", {
    spdlog::info("ctx.r8 = {}, ctx.r9 = {}", ctx.r8, ctx.r9);
    ctx.r9 = 0;
    ctx.r8 = 4000;
    //r8 = var_near
    //r9 = var_far
    })*/
#endif
}
