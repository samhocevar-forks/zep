#pragma once

#include "mode.h"

namespace PicoVim
{


enum class VimMode
{
    None,
    Normal,
    Insert,
    Visual,
    Command
};

enum class VimMotion
{
    LineBegin,   
    LineEnd,
    NonWhiteSpaceBegin,
    NonWhiteSpaceEnd
};

class PicoVimMode_Vim : public PicoVimMode
{
public:
    PicoVimMode_Vim(PicoVimEditor& editor);
    ~PicoVimMode_Vim();

    virtual void AddKeyPress(uint32_t key, uint32_t modifiers = 0) override;
    virtual void SetCurrentWindow(PicoVimWindow* pWindow) override;

private:
    void HandleInsert(uint32_t key);
    std::string GetCommandAndCount(std::string strCommand, int& count);
    virtual void SwitchMode(VimMode mode);
    virtual void AfterCommand(std::shared_ptr<PicoVimCommand> spCmd);
    virtual void ResetCommand();
    virtual std::shared_ptr<PicoVimCommand> GetCommand(std::string strCommand, uint32_t lastKey, uint32_t modifiers, VimMode mode, bool& handled);
    virtual void Init();

    VimMode m_switchToMode = VimMode::None;
    VimMode m_currentMode;
    bool initCount = true;
    std::string m_currentCommand;
    std::string m_lastCommand;
    int m_lastCommandCount;

    BufferLocation m_insertBegin;
    BufferLocation m_visualBegin;
    BufferLocation m_visualEnd;

    bool m_lineWise = false;
    bool m_pendingEscape = false;
    Timer m_insertEscapeTimer;
};

} // PicoVim
