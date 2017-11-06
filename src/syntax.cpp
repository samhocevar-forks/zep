#include "syntax.h"

namespace PicoVim
{

PicoVimSyntax::PicoVimSyntax(PicoVimBuffer& buffer)
    : PicoVimComponent(buffer.GetEditor()),
    m_buffer(buffer)
{
}

PicoVimSyntax::~PicoVimSyntax()
{
}

void PicoVimSyntax::Notify(std::shared_ptr<PicoVimMessage> spMsg)
{
    // Handle any interesting buffer messages
    if (spMsg->messageId == Msg_Buffer)
    {
        auto spBufferMsg = std::static_pointer_cast<BufferMessage>(spMsg);
        if (spBufferMsg->pBuffer != &m_buffer)
        {
            return;
        }
        if (spBufferMsg->type == BufferMessageType::PreBufferChange)
        {
            Interrupt();
        }
        else if (spBufferMsg->type == BufferMessageType::TextDeleted)
        {
            Interrupt();
            m_syntax.erase(m_syntax.begin() + spBufferMsg->startLocation, m_syntax.begin() + spBufferMsg->endLocation);

            m_processedChar = std::min(spBufferMsg->startLocation, long(m_processedChar));
            m_syntaxResult = m_buffer.GetThreadPool().enqueue([=]() { UpdateSyntax(spBufferMsg->startLocation, spBufferMsg->endLocation); });
        }
        else if (spBufferMsg->type == BufferMessageType::TextAdded)
        {
            Interrupt();
            m_syntax.insert(m_syntax.begin() + spBufferMsg->startLocation, spBufferMsg->endLocation - spBufferMsg->startLocation, SyntaxType::Normal);

            m_processedChar = std::min(spBufferMsg->startLocation, long(m_processedChar));
            m_syntaxResult = m_buffer.GetThreadPool().enqueue([=]() { UpdateSyntax(spBufferMsg->startLocation, spBufferMsg->endLocation); });
        }
        else if (spBufferMsg->type == BufferMessageType::TextChanged)
        {
            Interrupt();
            m_processedChar = std::min(spBufferMsg->startLocation, long(m_processedChar));
            m_syntaxResult = m_buffer.GetThreadPool().enqueue([=]() { UpdateSyntax(spBufferMsg->startLocation, spBufferMsg->endLocation); });
        }
    }
}

} // PicoVim
