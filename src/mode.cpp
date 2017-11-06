#include "buffer.h"
#include "mode.h"
#include "editor.h"
#include "commands.h"

namespace PicoVim
{

PicoVimMode::PicoVimMode(PicoVimEditor& editor)
    : PicoVimComponent(editor)
{
}

PicoVimMode::~PicoVimMode()
{

}

void PicoVimMode::SetCurrentWindow(PicoVimWindow* pDisplay)
{
    m_pCurrentWindow = pDisplay;
}

void PicoVimMode::AddCommandText(std::string strText)
{
    for (auto& ch : strText)
    {
        AddKeyPress(ch);
    }
}

void PicoVimMode::AddCommand(std::shared_ptr<PicoVimCommand> spCmd)
{
    spCmd->Redo();
    m_undoStack.push(spCmd);

    // Can't redo anything beyond this point
    std::stack<std::shared_ptr<PicoVimCommand>> empty;
    m_redoStack.swap(empty);
}

std::shared_ptr<PicoVimCommand> PicoVimMode::GetLastCommand() const
{
    if (m_undoStack.empty())
    {
        return nullptr;
    }
    return m_undoStack.top();
}

void PicoVimMode::Redo()
{
    bool inGroup = false;
    do 
    {
        if (!m_redoStack.empty())
        {
            auto& spCommand = m_redoStack.top();
            spCommand->Redo();

            if (spCommand->GetFlags() & CommandFlags::GroupBoundary)
            {
                inGroup = !inGroup;
            }

            m_undoStack.push(spCommand);
            m_redoStack.pop();
        }
        else
        {
            break;
        }
    } while (inGroup);
}

void PicoVimMode::Undo()
{
    bool inGroup = false;
    do
    {
        if (!m_undoStack.empty())
        {
            auto& spCommand = m_undoStack.top();
            spCommand->Undo();

            if (spCommand->GetFlags() & CommandFlags::GroupBoundary)
            {
                inGroup = !inGroup;
            }

            m_redoStack.push(spCommand);
            m_undoStack.pop();
        }
        else
        {
            break;
        }
    } 
    while (inGroup);
}

}

