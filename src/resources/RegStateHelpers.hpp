#pragma once
#include <safetyhook.hpp>

// ==========================================================
// ContextHelpers
//
// Helper functions for working with SafetyHook Context:
//    8-bit GPRs (al/ah, bl/bh, ..., r8b-r15b)
//    16-bit GPRs (ax, bx, ..., r8w-r15w)
//    32-bit GPRs (eax, ebx, ..., r8d-r15d)
//    RFLAGS/EFLAGS bits (ZF, CF, SF, OF, PF, AF, etc.)
//
// On x64, writing to eax, ebx, ecx, edx, esi, edi, ebp, esp
// clears the upper 32 bits of the corresponding 64-bit register.
//
// Example usage:
/*
    void MyHook(safetyhook::Context& regs)
    {
        reghelpers::set_al(regs, 0x12);
        reghelpers::set_ah(regs, 0x34);
        reghelpers::set_ax(regs, 0x5678);
        reghelpers::set_eax(regs, 0x12345678);
        uint16_t ax = reghelpers::get_ax(regs);

        reghelpers::SetZF(regs, true);
        bool zero = reghelpers::GetZF(regs);
    }
*/
// ==========================================================

namespace reghelpers
{

#if defined(_M_X64) || defined(__x86_64__)

    // --- 8-bit helpers ---
    inline void set_al(safetyhook::Context& regs, uint8_t val)
    {
        regs.rax = (regs.rax & ~0xFFULL) | val;
    }
    inline uint8_t get_al(const safetyhook::Context& regs)
    {
        return regs.rax & 0xFF;
    }
    inline void set_ah(safetyhook::Context& regs, uint8_t val)
    {
        regs.rax = (regs.rax & ~(0xFFULL << 8)) | (uint64_t(val) << 8);
    }
    inline uint8_t get_ah(const safetyhook::Context& regs)
    {
        return (regs.rax >> 8) & 0xFF;
    }

    inline void set_bl(safetyhook::Context& regs, uint8_t val)
    {
        regs.rbx = (regs.rbx & ~0xFFULL) | val;
    }
    inline uint8_t get_bl(const safetyhook::Context& regs)
    {
        return regs.rbx & 0xFF;
    }
    inline void set_bh(safetyhook::Context& regs, uint8_t val)
    {
        regs.rbx = (regs.rbx & ~(0xFFULL << 8)) | (uint64_t(val) << 8);
    }
    inline uint8_t get_bh(const safetyhook::Context& regs)
    {
        return (regs.rbx >> 8) & 0xFF;
    }

    inline void set_cl(safetyhook::Context& regs, uint8_t val)
    {
        regs.rcx = (regs.rcx & ~0xFFULL) | val;
    }
    inline uint8_t get_cl(const safetyhook::Context& regs)
    {
        return regs.rcx & 0xFF;
    }
    inline void set_ch(safetyhook::Context& regs, uint8_t val)
    {
        regs.rcx = (regs.rcx & ~(0xFFULL << 8)) | (uint64_t(val) << 8);
    }
    inline uint8_t get_ch(const safetyhook::Context& regs)
    {
        return (regs.rcx >> 8) & 0xFF;
    }

    inline void set_dl(safetyhook::Context& regs, uint8_t val)
    {
        regs.rdx = (regs.rdx & ~0xFFULL) | val;
    }
    inline uint8_t get_dl(const safetyhook::Context& regs)
    {
        return regs.rdx & 0xFF;
    }
    inline void set_dh(safetyhook::Context& regs, uint8_t val)
    {
        regs.rdx = (regs.rdx & ~(0xFFULL << 8)) | (uint64_t(val) << 8);
    }
    inline uint8_t get_dh(const safetyhook::Context& regs)
    {
        return (regs.rdx >> 8) & 0xFF;
    }

#define DEFINE_8BIT_LOW(name, field) \
inline void set_##name##l(safetyhook::Context& regs, uint8_t val) { regs.field = (regs.field & ~0xFFULL) | val; } \
inline uint8_t get_##name##l(const safetyhook::Context& regs) { return regs.field & 0xFF; }

    DEFINE_8BIT_LOW(si, rsi) DEFINE_8BIT_LOW(di, rdi) DEFINE_8BIT_LOW(bp, rbp) DEFINE_8BIT_LOW(sp, rsp)
#undef DEFINE_8BIT_LOW

        // r8b - r15b 8-bit low registers
#define DEFINE_R8B(num) \
    inline void set_r##num##b(safetyhook::Context& regs, uint8_t val) { regs.r##num = (regs.r##num & ~0xFFULL) | val; } \
    inline uint8_t get_r##num##b(const safetyhook::Context& regs) { return regs.r##num & 0xFF; }

        DEFINE_R8B(8)  DEFINE_R8B(9)  DEFINE_R8B(10) DEFINE_R8B(11)
        DEFINE_R8B(12) DEFINE_R8B(13) DEFINE_R8B(14) DEFINE_R8B(15)
#undef DEFINE_R8B

        // --- 16-bit helpers ---
#define DEFINE_16BIT(reg, name) \
    inline void set_##name(safetyhook::Context& regs, uint16_t val) { regs.reg = (regs.reg & ~0xFFFFULL) | val; } \
    inline uint16_t get_##name(const safetyhook::Context& regs) { return regs.reg & 0xFFFF; }

        DEFINE_16BIT(rax, ax)   DEFINE_16BIT(rbx, bx)   DEFINE_16BIT(rcx, cx)   DEFINE_16BIT(rdx, dx)
        DEFINE_16BIT(rsi, si)   DEFINE_16BIT(rdi, di)   DEFINE_16BIT(rbp, bp)   DEFINE_16BIT(rsp, sp)
        DEFINE_16BIT(r8, r8w)   DEFINE_16BIT(r9, r9w)   DEFINE_16BIT(r10, r10w) DEFINE_16BIT(r11, r11w)
        DEFINE_16BIT(r12, r12w) DEFINE_16BIT(r13, r13w) DEFINE_16BIT(r14, r14w) DEFINE_16BIT(r15, r15w)
#undef DEFINE_16BIT

        // --- 32-bit helpers ---
#define DEFINE_32BIT(reg, name) \
    inline void set_##name(safetyhook::Context& regs, uint32_t val) { regs.reg = (regs.reg & ~0xFFFFFFFFULL) | val; } \
    inline uint32_t get_##name(const safetyhook::Context& regs) { return regs.reg & 0xFFFFFFFF; }

        DEFINE_32BIT(rax, eax)   DEFINE_32BIT(rbx, ebx)   DEFINE_32BIT(rcx, ecx)   DEFINE_32BIT(rdx, edx)
        DEFINE_32BIT(rsi, esi)   DEFINE_32BIT(rdi, edi)   DEFINE_32BIT(rbp, ebp)   DEFINE_32BIT(rsp, esp)
        DEFINE_32BIT(r8, r8d)    DEFINE_32BIT(r9, r9d)    DEFINE_32BIT(r10, r10d)  DEFINE_32BIT(r11, r11d)
        DEFINE_32BIT(r12, r12d)  DEFINE_32BIT(r13, r13d)  DEFINE_32BIT(r14, r14d)  DEFINE_32BIT(r15, r15d)
#undef DEFINE_32BIT

        // --- Flags ---
        constexpr uint64_t FLAG_CF = 1ULL << 0;
    constexpr uint64_t FLAG_PF = 1ULL << 2;
    constexpr uint64_t FLAG_AF = 1ULL << 4;
    constexpr uint64_t FLAG_ZF = 1ULL << 6;
    constexpr uint64_t FLAG_SF = 1ULL << 7;
    constexpr uint64_t FLAG_OF = 1ULL << 11;

    inline bool GetFlag(const safetyhook::Context& regs, uint64_t mask)
    {
        return (regs.rflags & mask) != 0;
    }
    inline void SetFlag(safetyhook::Context& regs, uint64_t mask, bool val)
    {
        if (val) regs.rflags |= mask;
        else regs.rflags &= ~mask;
    }

    inline bool GetZF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_ZF);
    }
    inline void SetZF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_ZF, val);
    }

    inline bool GetCF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_CF);
    }
    inline void SetCF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_CF, val);
    }

    inline bool GetSF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_SF);
    }
    inline void SetSF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_SF, val);
    }

    inline bool GetOF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_OF);
    }
    inline void SetOF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_OF, val);
    }

    inline bool GetPF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_PF);
    }
    inline void SetPF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_PF, val);
    }

    inline bool GetAF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_AF);
    }
    inline void SetAF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_AF, val);
    }

    // ==========================================================
// DWORDx and SDWORDx helpers
// ==========================================================

// DWORD1/DWORD2: lower and upper 32 bits of a 64-bit value (unsigned)

    inline uint32_t DWORD1(uint64_t val)
    {
        return static_cast<uint32_t>(val & 0xFFFFFFFF);
    }
    inline uint32_t DWORD2(uint64_t val)
    {
        return static_cast<uint32_t>((val >> 32) & 0xFFFFFFFF);
    }
    inline void set_DWORD1(uint64_t& val, uint32_t dword)
    {
        val = (val & 0xFFFFFFFF00000000ULL) | dword;
    }
    inline void set_DWORD2(uint64_t& val, uint32_t dword)
    {
        val = (val & 0x00000000FFFFFFFFULL) | (uint64_t(dword) << 32);
    }

    // SDWORD1/SDWORD2: lower and upper 32 bits of a 64-bit value (signed)
    inline int32_t SDWORD1(int64_t val)
    {
        return static_cast<int32_t>(val & 0xFFFFFFFF);
    }
    inline int32_t SDWORD2(int64_t val)
    {
        return static_cast<int32_t>((val >> 32) & 0xFFFFFFFF);
    }
    inline void set_SDWORD1(int64_t& val, int32_t dword)
    {
        val = (val & 0xFFFFFFFF00000000LL) | (uint32_t(dword) & 0xFFFFFFFF);
    }
    inline void set_SDWORD2(int64_t& val, int32_t dword)
    {
        val = (val & 0x00000000FFFFFFFFLL) | (int64_t(uint32_t(dword)) << 32);
    }


#elif defined(_M_IX86) || defined(__i386__)

    // --- 8-bit helpers ---
    inline void set_al(safetyhook::Context& regs, uint8_t val)
    {
        regs.eax = (regs.eax & ~0xFFU) | val;
    }
    inline uint8_t get_al(const safetyhook::Context& regs)
    {
        return regs.eax & 0xFF;
    }
    inline void set_ah(safetyhook::Context& regs, uint8_t val)
    {
        regs.eax = (regs.eax & ~(0xFFU << 8)) | (uint32_t(val) << 8);
    }
    inline uint8_t get_ah(const safetyhook::Context& regs)
    {
        return (regs.eax >> 8) & 0xFF;
    }

    inline void set_bl(safetyhook::Context& regs, uint8_t val)
    {
        regs.ebx = (regs.ebx & ~0xFFU) | val;
    }
    inline uint8_t get_bl(const safetyhook::Context& regs)
    {
        return regs.ebx & 0xFF;
    }
    inline void set_bh(safetyhook::Context& regs, uint8_t val)
    {
        regs.ebx = (regs.ebx & ~(0xFFU << 8)) | (uint32_t(val) << 8);
    }
    inline uint8_t get_bh(const safetyhook::Context& regs)
    {
        return (regs.ebx >> 8) & 0xFF;
    }

    inline void set_cl(safetyhook::Context& regs, uint8_t val)
    {
        regs.ecx = (regs.ecx & ~0xFFU) | val;
    }
    inline uint8_t get_cl(const safetyhook::Context& regs)
    {
        return regs.ecx & 0xFF;
    }
    inline void set_ch(safetyhook::Context& regs, uint8_t val)
    {
        regs.ecx = (regs.ecx & ~(0xFFU << 8)) | (uint32_t(val) << 8);
    }
    inline uint8_t get_ch(const safetyhook::Context& regs)
    {
        return (regs.ecx >> 8) & 0xFF;
    }

    inline void set_dl(safetyhook::Context& regs, uint8_t val)
    {
        regs.edx = (regs.edx & ~0xFFU) | val;
    }
    inline uint8_t get_dl(const safetyhook::Context& regs)
    {
        return regs.edx & 0xFF;
    }
    inline void set_dh(safetyhook::Context& regs, uint8_t val)
    {
        regs.edx = (regs.edx & ~(0xFFU << 8)) | (uint32_t(val) << 8);
    }
    inline uint8_t get_dh(const safetyhook::Context& regs)
    {
        return (regs.edx >> 8) & 0xFF;
    }

    // --- 16-bit helpers ---
#define DEFINE_16BIT(reg, name) \
    inline void set_##name(safetyhook::Context& regs, uint16_t val) { regs.reg = (regs.reg & ~0xFFFFU) | val; } \
    inline uint16_t get_##name(const safetyhook::Context& regs) { return regs.reg & 0xFFFF; }

    DEFINE_16BIT(eax, ax) DEFINE_16BIT(ebx, bx) DEFINE_16BIT(ecx, cx) DEFINE_16BIT(edx, dx)
        DEFINE_16BIT(esi, si) DEFINE_16BIT(edi, di) DEFINE_16BIT(ebp, bp) DEFINE_16BIT(esp, sp)
#undef DEFINE_16BIT

        // --- Flags ---
        constexpr uint32_t FLAG_CF = 1U << 0;
    constexpr uint32_t FLAG_PF = 1U << 2;
    constexpr uint32_t FLAG_AF = 1U << 4;
    constexpr uint32_t FLAG_ZF = 1U << 6;
    constexpr uint32_t FLAG_SF = 1U << 7;
    constexpr uint32_t FLAG_OF = 1U << 11;

    inline bool GetFlag(const safetyhook::Context& regs, uint32_t mask)
    {
        return (regs.eflags & mask) != 0;
    }
    inline void SetFlag(safetyhook::Context& regs, uint32_t mask, bool val)
    {
        if (val) regs.eflags |= mask;
        else regs.eflags &= ~mask;
    }

    inline bool GetZF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_ZF);
    }
    inline void SetZF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_ZF, val);
    }

    inline bool GetCF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_CF);
    }
    inline void SetCF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_CF, val);
    }

    inline bool GetSF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_SF);
    }
    inline void SetSF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_SF, val);
    }

    inline bool GetOF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_OF);
    }
    inline void SetOF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_OF, val);
    }

    inline bool GetPF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_PF);
    }
    inline void SetPF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_PF, val);
    }

    inline bool GetAF(const safetyhook::Context& regs)
    {
        return GetFlag(regs, FLAG_AF);
    }
    inline void SetAF(safetyhook::Context& regs, bool val)
    {
        SetFlag(regs, FLAG_AF, val);
    }

#else
#error Unsupported architecture: only x86 and x64 supported
#endif

} // namespace reghelpers
