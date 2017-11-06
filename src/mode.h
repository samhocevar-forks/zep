#pragma once

#include <stack>
#include "buffer.h"
#include "display.h"

namespace PicoVim
{

class PicoVimEditor;
class PicoVimCommand;

struct ExtKeys
{
    enum Key
    {
        RETURN,
        ESCAPE,
        BACKSPACE,
        LEFT,
        RIGHT,
        UP,
        DOWN,
        TAB,
        DEL
    };
};

struct ModifierKey
{
    enum Key
    {
        None = (0),
        Ctrl = (1 << 0),
        Alt = (1 << 1),
        Shift = (1 << 2)
    };
};

class PicoVimMode : public PicoVimComponent
{
public:
    PicoVimMode(PicoVimEditor& editor);
    virtual ~PicoVimMode();

    // Keys handled by modes
    virtual void AddCommandText(std::string strText);
    virtual void AddKeyPress(uint32_t key, uint32_t modifierKeys = ModifierKey::None) = 0;
    virtual void Notify(std::shared_ptr<PicoVimMessage> message) override {}
    virtual void AddCommand(std::shared_ptr<PicoVimCommand> spCmd);
    virtual void SetCurrentWindow(PicoVimWindow* pWindow);

    virtual void Undo();
    virtual void Redo();


    virtual std::shared_ptr<PicoVimCommand> GetLastCommand() const;
protected:
    std::stack<std::shared_ptr<PicoVimCommand>> m_undoStack;
    std::stack<std::shared_ptr<PicoVimCommand>> m_redoStack;
    PicoVimWindow* m_pCurrentWindow = nullptr;
};

} // PicoVim
