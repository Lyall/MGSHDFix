#pragma once

// MGS2: the Tanker intro demo keeps one Snake per shot, and each copy walks the same take from the
// same spot. The crane shot cuts 54 frames before the demo swaps copies, so it opens on the old
// copy - 20,000 units further down the bridge - and then yanks him back. Rewind the leftover to
// where his take started, so the jump lands on the cut instead, where nobody can see it.
namespace MGS2TankerSnakeSnap
{
    inline bool bEnabled = true;
    void Initialize();
}
