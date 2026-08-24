#pragma once


// Trigger events on dll load (since MG1/MG2 don't immediately load their dlls.)
class ModuleLoadCallbacks
{
public:
    static inline void RegisterModuleLoadCallback(std::wstring_view moduleName, std::function<void()>&& fn)
    {
        if (moduleName.empty() || GetModuleHandleW(moduleName.data()) != nullptr)
        {
            fn();
            return;
        }

        RegisterDllNotification();
        GetOnModuleLoadCallbackList().emplace(moduleName, std::forward<std::function<void()>>(fn));
    }

    static inline void RegisterModuleUnloadCallback(std::wstring_view moduleName, std::function<void()>&& fn)
    {
        RegisterDllNotification();
        GetOnModuleUnloadCallbackList().emplace(moduleName, std::forward<std::function<void()>>(fn));
    }

private:
    static inline void InvokeOnModuleLoad(std::wstring_view moduleName)
    {
        auto& list = GetOnModuleLoadCallbackList();
        if (const auto it = list.find(moduleName.data()); it != list.end())
        {
            it->second();
            list.erase(it); // one-shot: a DLL only finishes loading once per process lifetime
        }
    }

    static inline void InvokeOnModuleUnload(std::wstring_view moduleName)
    {
        auto& list = GetOnModuleUnloadCallbackList();
        if (const auto it = list.find(moduleName.data()); it != list.end())
        {
            it->second();
            list.erase(it);
        }
    }

    struct CaseInsensitiveLess
    {
        bool operator()(const std::wstring& a, const std::wstring& b) const
        {
            std::wstring la(a.size(), L' '), lb(b.size(), L' ');
            std::transform(a.begin(), a.end(), la.begin(), ::towlower);
            std::transform(b.begin(), b.end(), lb.begin(), ::towlower);
            return la < lb;
        }
    };

    static inline auto& GetOnModuleLoadCallbackList()
    {
        static std::map<std::wstring, std::function<void()>, CaseInsensitiveLess> list;
        return list;
    }

    static inline auto& GetOnModuleUnloadCallbackList()
    {
        static std::map<std::wstring, std::function<void()>, CaseInsensitiveLess> list;
        return list;
    }

    using _LdrRegisterDllNotification = NTSTATUS(NTAPI*)(ULONG, PVOID, PVOID, PVOID);
    using _LdrUnregisterDllNotification = NTSTATUS(NTAPI*)(PVOID);

    struct LDR_DLL_NOTIFICATION_DATA_
    {
        ULONG Flags;
        PUNICODE_STRING FullDllName;
        PUNICODE_STRING BaseDllName;
        PVOID DllBase;
        ULONG SizeOfImage;
    };

    static inline void CALLBACK LdrDllNotification(ULONG notificationReason, LDR_DLL_NOTIFICATION_DATA_* data, PVOID)
    {
        constexpr ULONG LDR_DLL_NOTIFICATION_REASON_LOADED = 1;
        constexpr ULONG LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2;

        if (notificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
        {
            InvokeOnModuleLoad(data->BaseDllName->Buffer);
        }
        else if (notificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED)
        {
            InvokeOnModuleUnload(data->BaseDllName->Buffer);
        }
    }

    static inline void RegisterDllNotification()
    {
        if (bRegistered) return;

        if (const auto ldrRegister = reinterpret_cast<_LdrRegisterDllNotification>(
                GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrRegisterDllNotification")))
        {
            ldrRegister(0, reinterpret_cast<PVOID>(&LdrDllNotification), nullptr, &cookie);
            bRegistered = true;
        }
    }

    static inline void* cookie = nullptr;
    static inline bool bRegistered = false;
};
