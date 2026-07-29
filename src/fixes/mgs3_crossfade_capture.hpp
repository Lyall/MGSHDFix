#pragma once

namespace MGS3_CrossfadeCapture
{
    void Initialize();
    bool ReleaseWindowTick(); // call once per present; true while inside a crossfade-teardown window
}
