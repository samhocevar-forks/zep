#include "mode_standard.h"
#include "commands.h"

// Note:
// This is a version of the buffer that behaves like notepad.
// Since Vim is my usual buffer, this mode doesn't yet have many advanced operations, such as line-wise select, etc.
// These can all be copied from the Vim mode as and when needed

// TODO STANDARD: 
//    CTRL_C
//    CTRL_D
//    CTRL_F
//    CTRL_H
//    CTRL_L
//    CTRL_Q
//    CTRL_S
//    CTRL_U
//    HOME_KEY,
//    END_KEY,
//    PAGE_UP,
//    PAGE_DOWN

namespace PicoVim
{
PicoVimMode_Standard::PicoVimMode_Standard(PicoVimEditor& editor)
    : PicoVimMode(editor),
    m_currentMode(StandardMode::Insert)
{
}

PicoVimMode_Standard::~PicoVimMode_Standard()
{

}

void PicoVimMode_Standard::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    if (m_pCurrentWindow)
    {
        m_pCurrentWindow->GetCurrentBuffer()->Insert(m_pCurrentWindow->DisplayToBuffer(), std::string((char*)&key));
    }
}

} // PicoVim namespace