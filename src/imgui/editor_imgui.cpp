#include "editor_imgui.h"
#include "display_imgui.h"
#include "editor.h"
#include "mode_standard.h"
#include "mode_vim.h"
#include "syntax.h"
#include "tab_window.h"
#include <imgui.h>
#include <string>
#include "usb_hid_keys.h"

namespace Zep
{

ZepEditor_ImGui::ZepEditor_ImGui(const fs::path& root)
    : ZepEditor(new ZepDisplay_ImGui(), root)
{
}

void ZepEditor_ImGui::HandleInput()
{
    auto& io = ImGui::GetIO();

    bool inputChanged = false;
    bool handled = false;

    uint32_t mod = 0;

    if (io.MouseDelta.x != 0 ||
        io.MouseDelta.y != 0)
    {
        OnMouseMove(toNVec2f(io.MousePos));
    }

    if (io.MouseClicked[0])
    {
        OnMouseDown(toNVec2f(io.MousePos), ZepMouseButton::Left);
    }

    if (io.MouseClicked[1])
    {
        OnMouseDown(toNVec2f(io.MousePos), ZepMouseButton::Right);
    }

    if (io.MouseReleased[0])
    {
        OnMouseUp(toNVec2f(io.MousePos), ZepMouseButton::Left);
    }

    if (io.MouseReleased[1])
    {
        OnMouseUp(toNVec2f(io.MousePos), ZepMouseButton::Right);
    }

    if (io.KeyCtrl)
    {
        mod |= ModifierKey::Ctrl;
    }
    if (io.KeyShift)
    {
        mod |= ModifierKey::Shift;
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::TAB, mod);
    }
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::ESCAPE, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::RETURN, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::DEL, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Home)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::HOME, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_End)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::END, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::BACKSPACE, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::RIGHT, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::LEFT, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::UP, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::DOWN, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageDown)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::PAGEDOWN, mod);
    }
    else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_PageUp)))
    {
        GetCurrentMode()->AddKeyPress(ExtKeys::PAGEUP, mod);
    }
    else if (io.KeyCtrl)
    {
        if (ImGui::IsKeyPressed(KEY_1))
        {
            SetMode(ZepMode_Standard::StaticName());
            handled = true;
        }
        else if (ImGui::IsKeyPressed(KEY_2))
        {
            SetMode(ZepMode_Vim::StaticName());
            handled = true;
        }
        else
        {
            for (int ch = KEY_A; ch <= KEY_Z; ch++)
            {
                if (ImGui::IsKeyPressed(ch))
                {
                    GetCurrentMode()->AddKeyPress((ch - KEY_A) + 'a', mod);
                    handled = true;
                }
            }

            if (ImGui::IsKeyPressed(KEY_SPACE))
            {
                GetCurrentMode()->AddKeyPress(' ', mod);
                handled = true;
            }
        }
    }

    if (!handled)
    {
        for (int n = 0; n < io.InputQueueCharacters.Size && io.InputQueueCharacters[n]; n++)
        {
            GetCurrentMode()->AddKeyPress(io.InputQueueCharacters[n], mod);
        }
    }
}

} // namespace Zep
