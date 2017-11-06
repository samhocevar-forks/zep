#include "editor.h"
#include "buffer.h"
#include "display.h"
#include "mode_vim.h"
#include "mode_standard.h"

namespace PicoVim
{

const char* Msg_GetClipBoard = "GetClipboard";
const char* Msg_SetClipBoard = "SetClipboard";
const char* Msg_HandleCommand = "HandleCommand";

PicoVimComponent::PicoVimComponent(PicoVimEditor& editor)
    : m_editor(editor)
{
    m_editor.RegisterCallback(this);
}

PicoVimComponent::~PicoVimComponent()
{
    m_editor.UnRegisterCallback(this);
}


PicoVimEditor::PicoVimEditor(uint32_t flags)
    : m_flags(flags)
{
    AddBuffer("Scratch");
}

PicoVimEditor::~PicoVimEditor()
{

}

// Inform clients of an event in the buffer
bool PicoVimEditor::Broadcast(std::shared_ptr<PicoVimMessage> message)
{
    Notify(message);
    if (message->handled)
        return true;

    for (auto& client : m_notifyClients)
    {
        client->Notify(message);
        if (message->handled)
            break;
    }
    return message->handled;
}

PicoVimMode* PicoVimEditor::GetMode() const
{
    return m_pMode;
}

void PicoVimEditor::SetMode(PicoVimMode* pMode)
{
    m_pMode = pMode;
}

const std::deque<std::shared_ptr<PicoVimBuffer>>& PicoVimEditor::GetBuffers() const
{
    return m_buffers;
}

PicoVimBuffer* PicoVimEditor::AddBuffer(const std::string& str)
{
    auto spBuffer = std::make_shared<PicoVimBuffer>(*this, str);
    m_buffers.push_front(spBuffer);
    return spBuffer.get();
}

PicoVimBuffer* PicoVimEditor::GetMRUBuffer() const
{
    return m_buffers.front().get();
}

void PicoVimEditor::SetRegister(const std::string& reg, const Register& val)
{
    m_registers[reg] = val;
}

void PicoVimEditor::SetRegister(const char reg, const Register& val)
{
    std::string str({ reg });
    m_registers[str] = val;
}

void PicoVimEditor::SetRegister(const std::string& reg, const char* pszText)
{
    m_registers[reg] = Register(pszText);
}

void PicoVimEditor::SetRegister(const char reg, const char* pszText)
{
    std::string str({ reg });
    m_registers[str] = Register(pszText);
}

Register& PicoVimEditor::GetRegister(const std::string& reg)
{
    return m_registers[reg];
}

Register& PicoVimEditor::GetRegister(const char reg)
{
    std::string str({ reg });
    return m_registers[str];
}
const tRegisters& PicoVimEditor::GetRegisters() const
{
    return m_registers;
}

void PicoVimEditor::Notify(std::shared_ptr<PicoVimMessage> message)
{
}

} // namespace PicoVim
