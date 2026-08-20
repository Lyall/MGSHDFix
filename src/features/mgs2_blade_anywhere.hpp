#pragma once

// The High-Frequency Blade on any stage: NewPluginBlade is registered in every stage's chara
// table but only late-Plant scripts ever invoke it, and only those stages pack its assets.
// This calls the plugin with a synthesized `-m rai_blade` line each stage and rides the
// bp_assets merge to make the blade's asset set resident everywhere.
namespace MGS2BladeAnywhere
{
    void Initialize();

    inline bool bEnabled = false;
}
