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

#include <imgui.h>

#include "status_bar.hpp"

constexpr uint8_t margin_x = 8;
constexpr uint8_t margin_y = 3;
constexpr uint16_t millisecond_per_frame = 20;


StatusBar::StatusBar(uint16_t width, uint16_t height) :
    width(width),
    height(height),
    x(0),
    y(0),
    text_duration(0),
    active_flags(0)
{}


StatusBar::~StatusBar()
{
}


void StatusBar::show_text_for(const std::string& text, std::chrono::milliseconds duration)
{
    this->text = text;
    text_duration = duration.count() / millisecond_per_frame;
}


void StatusBar::set_flag(uint16_t flag, bool on)
{
    uint16_t old_flags = active_flags;

    if (on) {
        active_flags |= flag;
    } else {
        active_flags &= ~flag;
    }

    if (active_flags != old_flags) {
        std::string flags_string;

        if (active_flags & StatusbarFlags::loading) {
            flags_string.append("[Tape]");
        }
        if (active_flags & StatusbarFlags::warp_mode) {
            flags_string.append("[Warp]");
        }

        flags_text = flags_string;
    }
}


void StatusBar::render()
{
    if (text_duration > 0) {
        --text_duration;
        if (text_duration == 0) {
            text = "";
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 2.0f));
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

    ImGui::SetNextWindowPos(ImVec2(x+5, y), ImGuiCond_Always);
    ImGui::Begin("Status bar", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs);

    ImGui::TextUnformatted(text.c_str());

    const float text_width = ImGui::CalcTextSize(flags_text.c_str()).x;
    const float right_padding = ImGui::GetStyle().WindowPadding.x;
    const float avail = ImGui::GetContentRegionAvail().x;

    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - text_width - right_padding - 15);
    ImGui::TextUnformatted(flags_text.c_str());

    ImGui::End();
    ImGui::PopStyleVar(2);
}

