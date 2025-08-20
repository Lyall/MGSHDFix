#include "common.hpp"
#include "distance_culling.hpp"
#include "logging.hpp"



void DistanceCulling::Initialize() const
{
    if (!(eGameType & MGS3))
    {
        return;
    }


    if (eGameType & MGS3)
    {

        MAKE_HOOK_MID(baseModule, "F3 0F 11 83 ?? ?? ?? ?? 41 8B FC", "MGS3: Grass Farclip", {
           ctx.xmm0.f32[0] = std::numeric_limits<float>::max();
            })
    }
}
