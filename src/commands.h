#pragma once

#include "mode.h"

namespace PicoVim
{

namespace CommandFlags
{
enum
{
    GroupBoundary = (1 << 0),
};

}

class PicoVimCommand
{
public:
    PicoVimCommand(PicoVimBuffer& mode) 
    : m_buffer(mode)
    {}

    virtual ~PicoVimCommand() {}

    virtual void Redo() = 0;
    virtual void Undo() = 0;

    virtual void SetFlags(uint32_t flags) { m_flags = flags; }
    virtual uint32_t GetFlags() const { return m_flags; }

protected:
    PicoVimBuffer& m_buffer;
    uint32_t m_flags = 0;
};

class PicoVimCommand_DeleteRange : public PicoVimCommand
{
public:
    PicoVimCommand_DeleteRange(PicoVimBuffer& buffer, const BufferLocation& startOffset, const BufferLocation& endOffset, const BufferLocation& cursorAfter = BufferLocation{ -1 });
    virtual ~PicoVimCommand_DeleteRange() {};

    virtual void Redo() override;
    virtual void Undo() override;

    BufferLocation m_startOffset;
    BufferLocation m_endOffset;
    BufferLocation m_cursorAfter;

    std::string m_deleted;
};

class PicoVimCommand_Insert : public PicoVimCommand
{
public:
    PicoVimCommand_Insert(PicoVimBuffer& buffer, const BufferLocation& startOffset, const std::string& str, const BufferLocation& cursorAfter = BufferLocation{ -1 });
    virtual ~PicoVimCommand_Insert() {};

    virtual void Redo() override;
    virtual void Undo() override;

    BufferLocation m_startOffset;
    std::string m_strInsert;

    BufferLocation m_endOffsetInserted;
    BufferLocation m_cursorAfter;
};

} // PicoVim