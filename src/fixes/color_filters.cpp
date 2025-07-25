#include "common.hpp"
#include "color_filters.hpp"

#include <logging.hpp>


#define NORMAL_FILTER "\x38"
#define LIGHT_GREEN_FILTER 570
#define DARK_BLUE_FILTER 571

/*
void colorFilterFix()
{

}
*/

void ColorFilterFix::Initialize()
{
    if (!(eGameType & MGS2))
    {
        return;
    }
    /* PLACEHOLDER CODE. DON'T ACTIVATE.
    if (uint8_t* MGS2_ColorFilterResult = Memory::PatternScan(baseModule, "41 0F B6 8C 19 ?? ?? ?? ?? 2B C1", "MGS 2: Color Filter Fix", nullptr, nullptr))
    {

        41 0F B6 8C 19 ?? ?? ?? ?? 2B C1 = wildcard
        41 0F B6 8C 19 38 02 00 = full
        41 0F B6 8C 19 38 02 = normal
        41 0F B6 8C 19 39 02 = green
        41 0F B6 8C 19 3A 02 = blue

        uintptr_t MGS2_ColorFilterAddress = Memory::GetAbsolute((uintptr_t)MGS2_ColorFilterResult + 5);
        
        // Patch the color filter to use normal filter (0x38) instead of green (0x39) or blue (0x3A)
        Memory::PatchBytes(MGS2_ColorFilterAddress, "\x38", 1);
        
        spdlog::info("MGS 2: Color Filter Fix: Patched color filter to normal at address {:s}+{:X}", 
                     sExeName.c_str(), (uintptr_t)MGS2_ColorFilterResult - (uintptr_t)baseModule);
    }
    */
}
