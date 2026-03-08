// =========================================================================
//   Copyright (C) 2009-2026 by Anders Piniesjö <pugo@pugo.org>
//
//   This program is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program.  If not, see <http://www.gnu.org/licenses/>
// =========================================================================

#ifndef FRONTENDS_GUI_STATUSBAR_H
#define FRONTENDS_GUI_STATUSBAR_H

#include <chrono>
#include <string>
#include <cstddef>
#include <optional>

#include "frontends/flags.hpp"

/**
 * Handle status bar.
 */
class StatusBar
{
public:
    StatusBar(uint16_t width, uint16_t height);
    ~StatusBar();

    void set_size(uint16_t width, uint16_t height)
    {
        this->width = width;
        this->height = height;
    }

    void set_position(uint16_t x, uint16_t y)
    {
        this->x = x;
        this->y = y;
    }

    /**
     * Notify status bar that the contents should be repainted.
     */
    void render();

    /**
     * Show given string for a certain duration. Triggers an update.
     * @param text text to show
     * @param duration duration in number of frames
     */
    void show_text_for(const std::string& text, std::chrono::milliseconds duration);

    /**
     * Set flag to wanted state. Triggers update if flags are changed.
     * @param flag flag to set
     * @param on flag state
     */
    void set_flag(uint16_t flag, bool on);

private:
    uint16_t width;
    uint16_t height;
    uint16_t x;
    uint16_t y;

    std::string text;
    uint16_t text_duration;

    uint16_t active_flags;

    std::string flags_text;
};


#endif // FRONTENDS_GUI_STATUSBAR_H
