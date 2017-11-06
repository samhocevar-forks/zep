#pragma once

#include "mode.h"

namespace PicoVim
{

enum class StandardMode
{
    None,
    Insert,
    Visual,
};

class PicoVimMode_Standard : public PicoVimMode
{
public:
    PicoVimMode_Standard(PicoVimEditor& editor);
    ~PicoVimMode_Standard();

    virtual void SwitchMode(StandardMode mode) {};
    virtual void AddKeyPress(uint32_t key, uint32_t modifiers = 0) override;

    StandardMode m_currentMode;
    BufferLocation m_insertBegin;
    BufferLocation m_visualBegin;
    BufferLocation m_visualEnd;
};

} // PicoVim
