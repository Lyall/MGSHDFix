
/*
void colorFilterFix()
{
    uintptr_t MGS2_ColorFilterAddress = (uintptr_t)Memory::PatternScan(baseModule, "41 0F B6 8C 19 ?? ?? ?? ?? 2B C1", "MGS 2: Color Filter Address", NULL, NULL);
    MGS2_ColorFilterAddress = Memory::GetAbsolute(MGS2_ColorFilterAddress + 5);
    //address =   + 5
#define NORMAL_FILTER "\x38"
#define LIGHT_GREEN_FILTER 570
#define DARK_BLUE_FILTER 571

41 0F B6 8C 19 ?? ?? ?? ?? 2B C1 = wildcard
41 0F B6 8C 19 38 02 00 = full
41 0F B6 8C 19 38 02 = normal
41 0F B6 8C 19 39 02 = green
41 0F B6 8C 19 3A 02 = blue
}
*/