#pragma once
#include "config_keys.hpp"

namespace SwapMenuButtons
{
    void SetMenuButtonInputs();

    inline std::string force_menu_buttons = ConfigKeys::MenuButton_Option_Default;
}
