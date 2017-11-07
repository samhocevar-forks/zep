#include <cctype>
#include <sstream>

#include "mode_vim.h"
#include "commands.h"
#include "utils/stringutils.h"

// Note:
// This is a very basic implementation of the common Vim commands that I use: the bare minimum I can live with.
// I do use more, and depending on how much pain I suffer, will add them over time.
// My aim is to make it easy to add commands, so if you want to put something in, please send me a PR.
// The buffer/display search and find support makes it easy to gather the info you need, and the basic insert/delete undo redo commands
// make it easy to find the locations in the buffer
// Important to note: I'm not trying to beat/better Vim here.  Just make an editor I can use in a viewport without feeling pain ;)
// See further down for what is implemented, and what's on my todo list

// IMPLEMENTED VIM:
// Command counts
// hjkl Motions   
// . dot command
// TAB
// w,W,e,E,ge,gE,b,B Word motions
// u,CTRL+r  Undo, Redo
// i,I,a,A Insert mode (pending undo/redo fix)
// DELETE/BACKSPACE in insert and normal mode; match vim
// Command status bar
// Arrow keys
// '$'
// 'jk' to insert mode 
// gg Jump to end
// G Jump to beginning
// 'J' join
// D
// dd,d$,x  Delete line, to end of line, chars
// 'v' + 'x'/'d'
// 'y'
// 'p'/'P'
// a-z&a-Z, 0->9, _ " registers
// '$'
// 'yy'
// cc
// c$  Change to end of line
// C
// S
// 0
// ^
// 'O', 'o'
// 'V' (linewise v)
// Y, D, linewise yank/paste

// VIM TODO:

// d[a]<count>w/e  Delete words
// di})]"'
// c[a]<count>w/e  Change word
// ci})]"'

// % Jump
// f (find) / next, previous
// /Searching

// 'R'/'r' overstrike, replace
// s substitute

// Standardize buffer format (remove \r\n, use just \n)
// Standard Mode


namespace
{

}
namespace PicoVim
{

// Given a searched block, find the next word 
BufferLocation WordMotion(const BufferBlock& block)
{
    // If on a space, move to the first block
    // Otherwise, we are on a word, and need to move to the second block
    if (block.direction == 1)
    {
        if (block.spaceBefore)
            return block.firstBlock;
        else
            return block.secondBlock;
    }
    else
    {
        if (block.blockSearchPos == (block.firstNonBlock - block.direction))
        {
            return block.secondNonBlock - block.direction;
        }
        return block.firstNonBlock - block.direction;
    }
}

BufferLocation WordEndMotion(const BufferBlock& block)
{
    // If on a space, move to the first block
    // Otherwise, we are on a word, and need to move to the second block
    if (block.direction == 1)
    {
        // If we are sitting on the end of the block, move to the next one
        if (block.blockSearchPos == block.firstNonBlock - block.direction)
        {
            return block.secondNonBlock - block.direction;
        }
        else
        {
            return block.firstNonBlock - block.direction;
        }
    }
    else
    {
        // 'ge'
        // Back to the end of the word
        if (block.spaceBefore)
        {
            return block.firstBlock;
        }
        else
        {
            return block.secondBlock;
        }
    }
}

std::pair<BufferLocation, BufferLocation> Word(const BufferBlock& block)
{
    if (block.spaceBefore)
    {
        return std::make_pair(block.blockSearchPos, block.firstNonBlock);
    }
    else
    {
        return std::make_pair(block.firstBlock, block.secondBlock);
    }
}

std::pair<BufferLocation, BufferLocation> InnerWord(const BufferBlock& block)
{
    if (block.spaceBefore)
    {
        return std::make_pair(block.spaceBeforeStart, block.firstBlock);
    }
    return std::make_pair(block.firstBlock, block.firstNonBlock);
}

PicoVimMode_Vim::PicoVimMode_Vim(PicoVimEditor& editor)
    : PicoVimMode(editor),
    m_currentMode(VimMode::Normal)
{
    Init();
}

PicoVimMode_Vim::~PicoVimMode_Vim()
{

}

void PicoVimMode_Vim::Init()
{
    for (int i = 0; i <= 9; i++)
    {
        GetEditor().SetRegister('0' + i, "");
    }
    GetEditor().SetRegister('"', "");
}

void PicoVimMode_Vim::ResetCommand()
{
    m_currentCommand.clear();
}

void PicoVimMode_Vim::SwitchMode(VimMode mode)
{
    m_currentMode = mode;
    switch (mode)
    {
    case VimMode::Normal:
        m_pCurrentWindow->SetCursorMode(CursorMode::Normal);
        ResetCommand();
        break;
    case VimMode::Insert:
        m_insertBegin = m_pCurrentWindow->DisplayToBuffer();
        m_pCurrentWindow->SetCursorMode(CursorMode::Insert);
        m_pendingEscape = false;
        break;
    case VimMode::Visual:
        m_pCurrentWindow->SetCursorMode(CursorMode::Visual);
        ResetCommand();
        m_pendingEscape = false;
        break;
    }
}

void PicoVimMode_Vim::AfterCommand(std::shared_ptr<PicoVimCommand> spCommand)
{

    // A mode to switch to after the command is done
    if (m_switchToMode != VimMode::None)
    {
        SwitchMode(m_switchToMode);
        m_switchToMode = VimMode::None;
    }

    // Visual mode update - after a command
    if (m_currentMode == VimMode::Visual)
    {
        // Update the visual range
        if (m_lineWise)
        {
            auto pLineInfo = &m_pCurrentWindow->visibleLines[m_pCurrentWindow->cursorCL.y];
            m_visualEnd = m_pCurrentWindow->GetCurrentBuffer()->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd) - 1;
        }
        else
        {
            m_visualEnd = m_pCurrentWindow->DisplayToBuffer();
        }
        m_pCurrentWindow->SetSelectionRange(m_pCurrentWindow->BufferToDisplay(m_visualBegin), m_pCurrentWindow->BufferToDisplay(m_visualEnd));
    }

    // Ensure the cursor is in the show state with each command
    if (m_pCurrentWindow)
    {
        m_pCurrentWindow->GetDisplay().ResetCursorTimer();
    }

}

std::shared_ptr<PicoVimCommand> PicoVimMode_Vim::GetCommand(std::string command, uint32_t lastKey, uint32_t modifierKeys, VimMode mode, bool& handled)
{
    handled = true;
    auto cursor = m_pCurrentWindow->cursorCL;
    const LineInfo* pLineInfo = nullptr;
    if (m_pCurrentWindow->visibleLines.size() > cursor.y)
    {
        pLineInfo = &m_pCurrentWindow->visibleLines[cursor.y];
    }
    std::shared_ptr<PicoVimCommand> spCommand;

    enum class CommandOperation
    {
        None,
        Delete,
        DeleteLines,
        Insert,
        Copy,
        CopyLines
    };

    m_switchToMode = VimMode::None;
    BufferLocation beginRange{ -1 };
    BufferLocation endRange{ -1 };
    BufferLocation cursorAfter{ -1 };
    std::stack<char> registers;
    registers.push('"');
    CommandOperation op = CommandOperation::None;
    Register tempReg("", false);
    const Register* pRegister = &tempReg;

    auto pBuffer = m_pCurrentWindow->m_pCurrentBuffer;
    const auto bufferCursor = m_pCurrentWindow->DisplayToBuffer(cursor);

    // Store the register source
    if (command[0] == '"' && command.size() > 2)
    {
        // Null register
        if (command[1] == '_')
        {
            std::stack<char> temp;
            registers.swap(temp);
        }
        else
        {
            registers.push(command[1]);
            char reg = command[1];

            // Demote capitals to lower registers when pasting (all both)
            if (reg >= 'A' && reg <= 'Z')
            {
                reg = std::tolower(reg);
            }

            if (GetEditor().GetRegisters().find(std::string({ reg })) != GetEditor().GetRegisters().end())
            {
                pRegister = &GetEditor().GetRegister(reg);
            }
        }
        command = command.substr(2);
    }
    else
    {
        // Default register
        if (pRegister->text.empty())
            pRegister = &GetEditor().GetRegister('"');
    }

    // Motion
    if (command == "$")
    {
        m_pCurrentWindow->MoveCursorTo(pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineCRBegin));
    }
    else if (command == "0")
    {
        m_pCurrentWindow->MoveCursorTo(pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin));
    }
    else if (command == "^")
    {
        m_pCurrentWindow->MoveCursorTo(pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineFirstGraphChar));
    }
    else if (command == "j" || command == "+" || lastKey == ExtKeys::DOWN)
    {
        m_pCurrentWindow->MoveCursor(PicoVim::NVec2i(0, 1));
    }
    else if (command == "k" || command == "-" || lastKey == ExtKeys::UP)
    {
        m_pCurrentWindow->MoveCursor(PicoVim::NVec2i(0, -1));
    }
    else if (command == "l" || lastKey == ExtKeys::RIGHT)
    {
        m_pCurrentWindow->MoveCursor(PicoVim::NVec2i(1, 0));
    }
    else if (command == "h" || lastKey == ExtKeys::LEFT)
    {
        m_pCurrentWindow->MoveCursor(PicoVim::NVec2i(-1, 0));
    }
    else if (command == "G")
    {
        if (m_currentCommand != "G")
        {
            // Goto line
            m_pCurrentWindow->MoveCursorTo(pBuffer->GetLinePos(1, LineLocation::LineBegin));
        }
        else
        {
            m_pCurrentWindow->MoveCursorTo(pBuffer->EndLocation());
        }
    }
    else if (lastKey == ExtKeys::BACKSPACE)
    {
        auto loc = bufferCursor;

        // Insert-mode command
        if (mode == VimMode::Insert)
        {
            // In insert mode, we are 'on' the character after the one we want to delete
            beginRange = pBuffer->LocationFromOffsetByChars(loc, -1);
            endRange = pBuffer->LocationFromOffsetByChars(loc, 1);
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else
        {
            // Normal mode moves over the chars, and wraps
            m_pCurrentWindow->MoveCursorTo(pBuffer->LocationFromOffsetByChars(loc, -1));
        }
    }
    else if (command == "w")
    {
        auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Forward);
        m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordMotion(block));
    }
    else if (command == "W")
    {
        auto block = pBuffer->GetBlock(SearchType::Word, bufferCursor, SearchDirection::Forward);
        m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordMotion(block));
    }
    else if (command == "b")
    {
        auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Backward);
        m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordMotion(block));
    }
    else if (command == "B")
    {
        auto block = pBuffer->GetBlock(SearchType::Word, bufferCursor, SearchDirection::Backward);
        m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordMotion(block));
    }
    else if (command == "e")
    {
        auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Forward);
        m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordEndMotion(block));
    }
    else if (command == "E")
    {
        auto block = pBuffer->GetBlock(SearchType::Word, bufferCursor, SearchDirection::Forward);
        m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordEndMotion(block));
    }
    else if (command[0] == 'g')
    {
        if (command == "ge")
        {
            auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Backward);
            m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordEndMotion(block));
        }
        else if (command == "gE")
        {
            auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Backward);
            m_pCurrentWindow->cursorCL = m_pCurrentWindow->BufferToDisplay(WordEndMotion(block));
        }
        else if (command == "gg")
        {
            m_pCurrentWindow->MoveCursorTo(BufferLocation{ 0 });
        }
        else
        {
            // Unknown
            handled = false;
            return nullptr;
        }
    }
    else if (command == "J")
    {
        // Delete the CR (and thus join lines)
        beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineCRBegin);
        endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd);
        cursorAfter = bufferCursor;
        op = CommandOperation::Delete;
    }
    else if (command == "v" ||
        command == "V")
    {
        if (m_currentMode == VimMode::Visual)
        {
            m_switchToMode = VimMode::Normal;
        }
        else
        {
            if (command == "V")
            {
                m_visualBegin = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
                m_visualEnd = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd) - 1;
            }
            else
            {
                m_visualBegin = bufferCursor;
                m_visualEnd = m_visualBegin;
            }
            m_switchToMode = VimMode::Visual;
        }
        m_lineWise = command == "V" ? true : false;
    }
    else if (command == "x" || lastKey == ExtKeys::DEL)
    {
        auto loc = bufferCursor;

        if (m_currentMode == VimMode::Visual)
        {
            beginRange = m_visualBegin;
            endRange = pBuffer->LocationFromOffsetByChars(m_visualEnd, 1);
            cursorAfter = m_visualBegin;
            op = CommandOperation::Delete;
            m_switchToMode = VimMode::Normal;
        }
        else
        {
            // Don't allow x to delete beyond the end of the line
            if (command != "x" ||
                std::isgraph(pBuffer->GetText()[loc]) ||
                std::isblank(pBuffer->GetText()[loc]))
            {
                beginRange = loc;
                endRange = pBuffer->LocationFromOffsetByChars(loc, 1);
                cursorAfter = loc;
                op = CommandOperation::Delete;
            }
            else
            {
                ResetCommand();
            }
        }
    }
    else if (command == "ss" ||
        command == "S")
    {
        if (pLineInfo)
        {
            // Delete whole line
            beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
            endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineCRBegin);
            cursorAfter = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineFirstGraphChar);
            op = CommandOperation::Delete;
        }
    }
    else if (command[0] == 'o')
    {
        beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd);
        tempReg.text = "\r\n";
        pRegister = &tempReg;
        cursorAfter = beginRange;
        op = CommandOperation::Insert;
        m_switchToMode = VimMode::Insert;
    }
    else if (command[0] == 'O')
    {
        beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
        tempReg.text = "\r\n";
        pRegister = &tempReg;
        cursorAfter = beginRange;
        op = CommandOperation::Insert;
        m_switchToMode = VimMode::Insert;
    }
    else if (command[0] == 'd' ||
        command == "D")
    {
        if (command == "d")
        {
            if (mode == VimMode::Visual)
            {
                beginRange = m_visualBegin;
                endRange = pBuffer->LocationFromOffsetByChars(m_visualEnd, 1);
                cursorAfter = m_visualBegin;
                op = CommandOperation::Delete;
                m_switchToMode = VimMode::Normal;
            }
            else
            {
                handled = false;
                return nullptr;
            }
        }
        else if (command == "dd")
        {
            if (pLineInfo)
            {
                // Delete whole line, including CR
                beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
                endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd);
                cursorAfter = beginRange;
                op = CommandOperation::DeleteLines;
            }
        }
        else if (command == "d$" ||
            command == "D")
        {
            if (pLineInfo)
            {
                // Delete rest of line
                beginRange = bufferCursor;
                endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineCRBegin);
                cursorAfter = beginRange;
                op = CommandOperation::Delete;
            }
        }
        else if (command == "dw")
        {
            auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Forward);
            beginRange = block.blockSearchPos;
            endRange = WordMotion(block);
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else if (command == "dW")
        {
            auto block = pBuffer->GetBlock(SearchType::Word, bufferCursor, SearchDirection::Forward);
            beginRange = block.blockSearchPos;
            endRange = WordMotion(block);
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else if (command == "daw")
        {
            auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Forward);
            beginRange = Word(block).first;
            endRange = Word(block).second;
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else if (command == "daW")
        {
            auto block = pBuffer->GetBlock(SearchType::Word, bufferCursor, SearchDirection::Forward);
            beginRange = Word(block).first;
            endRange = Word(block).second;
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else if (command == "diw")
        {
            auto block = pBuffer->GetBlock(SearchType::AlphaNumeric | SearchType::Word, bufferCursor, SearchDirection::Forward);
            beginRange = InnerWord(block).first;
            endRange = InnerWord(block).second;
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else if (command == "diW")
        {
            auto block = pBuffer->GetBlock(SearchType::Word, bufferCursor, SearchDirection::Forward);
            beginRange = InnerWord(block).first;
            endRange = InnerWord(block).second;
            cursorAfter = beginRange;
            op = CommandOperation::Delete;
        }
        else
        {
            // Unknown
            handled = false;
            return nullptr;
        }
    }
    else if (command[0] == 'C' ||
        command[0] == 'c')
    {
        if (command == "C" ||
            command == "c$")
        {
            // Delete rest of line before change
            if (pLineInfo)
            {
                beginRange = bufferCursor;
                endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineCRBegin);
                cursorAfter = command == "C" ? pBuffer->LocationFromOffsetByChars(beginRange, 1) : beginRange;
                op = CommandOperation::Delete;
            }
        }
        else if (command == "cc")
        {
            if (pLineInfo)
            {
                // Delete whole line
                beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
                endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineCRBegin);
                cursorAfter = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineFirstGraphChar);
                op = CommandOperation::Delete;
            }
        }
        else
        {
            // keep looking
            handled = false;
            return nullptr;
        }
        m_switchToMode = VimMode::Insert;
    }
    else if (command == "p")
    {
        if (!pRegister->text.empty())
        {
            if (pRegister->lineWise)
            {
                beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd);
                cursorAfter = beginRange;
            }
            else
            {
                beginRange = pBuffer->LocationFromOffsetByChars(bufferCursor, 1);
                cursorAfter = pBuffer->LocationFromOffset(beginRange, long(StringUtils::Utf8Length(pRegister->text.c_str())) - 1);
            }
            op = CommandOperation::Insert;
        }
    }
    else if (command == "P")
    {
        if (!pRegister->text.empty())
        {
            if (pRegister->lineWise)
            {
                beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
                cursorAfter = beginRange;
            }
            else
            {
                beginRange = bufferCursor;
                cursorAfter = pBuffer->LocationFromOffsetByChars(beginRange, long(StringUtils::Utf8Length(pRegister->text.c_str()) - 1));
            }
            op = CommandOperation::Insert;
        }
    }
    else if (command[0] == 'y')
    {
        if (mode == VimMode::Visual)
        {
            registers.push('0');
            beginRange = m_visualBegin;
            endRange = pBuffer->LocationFromOffsetByChars(m_visualEnd, 1);
            cursorAfter = m_visualBegin;
            m_switchToMode = VimMode::Normal;
            op = m_lineWise ? CommandOperation::CopyLines : CommandOperation::Copy;
        }
        else if (mode == VimMode::Normal)
        {
            if (command == "yy")
            {
                if (pLineInfo)
                {
                    // Copy the whole line, including the CR
                    registers.push('0');
                    beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
                    endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd) + 1;
                    op = CommandOperation::CopyLines;
                }
            }
            else
            {
                handled = false;
                return nullptr;
            }
        }
        else
        {
            handled = false;
            return nullptr;
        }
    }
    else if (command == "Y")
    {
        if (mode == VimMode::Visual)
        {
            registers.push('0');
            beginRange = m_visualBegin;
            endRange = pBuffer->LocationFromOffsetByChars(m_visualEnd, 1);
            cursorAfter = m_visualBegin;
            m_switchToMode = VimMode::Normal;
            op = CommandOperation::CopyLines;
        }
        else
        {
            if (pLineInfo)
            {
                // Copy the whole line, including the CR
                registers.push('0');
                beginRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineBegin);
                endRange = pBuffer->GetLinePos(pLineInfo->lineNumber, LineLocation::LineEnd);
                op = CommandOperation::CopyLines;
            }
        }
    }
    else if (command == "u")
    {
        Undo();
    }
    else if (command == "r" && modifierKeys == ModifierKey::Ctrl)
    {
        Redo();
    }
    else if (command == "i")
    {
        m_switchToMode = VimMode::Insert;
    }
    else if (command == "a")
    {
        // Cursor append
        m_pCurrentWindow->MoveCursor(NVec2i(1, 0), LineLocation::LineCRBegin);
        m_switchToMode = VimMode::Insert;
    }
    else if (command == "A")
    {
        // Cursor append to end of line
        m_pCurrentWindow->MoveCursor(LineLocation::LineCRBegin);
        m_switchToMode = VimMode::Insert;
    }
    else if (command == "I")
    {
        // Cursor Insert beginning char of line 
        m_pCurrentWindow->MoveCursor(LineLocation::LineFirstGraphChar);
        m_switchToMode = VimMode::Insert;
    }
    else if (lastKey == ExtKeys::RETURN)
    {
        if (command[0] == ':')
        {
            if (command == ":reg")
            {
                std::ostringstream str;
                str << "--- Registers ---" << '\n';
                for (auto& reg : GetEditor().GetRegisters())
                {
                    if (!reg.second.text.empty())
                    {
                        std::string displayText = reg.second.text;
                        displayText = StringUtils::ReplaceString(displayText, "\r\n", "^J");
                        str << "\"" << reg.first << "   " << displayText << '\n';
                    }
                }
                m_pCurrentWindow->GetDisplay().SetCommandText(str.str());
            }
            else if (command == ":ls")
            {
                std::ostringstream str;
                str << "--- Buffers ---" << '\n';
                int index = 0;
                for (auto& buffer : GetEditor().GetBuffers())
                {
                    if (!buffer->GetName().empty())
                    {
                        std::string displayText = buffer->GetName();
                        displayText = StringUtils::ReplaceString(displayText, "\r\n", "^J");
                        if (pBuffer == buffer.get())
                        {
                            str << "*";
                        }
                        else
                        {
                            str << " ";
                        }
                        str << index++ << " : " << displayText << '\n';
                    }
                }
                m_pCurrentWindow->GetDisplay().SetCommandText(str.str());
            }
            else if (command.find(":bu") == 0)
            {
                auto strTok = StringUtils::Split(command, " ");

                if (strTok.size() > 1)
                {
                    try
                    {
                        auto index = std::stoi(strTok[1]);
                        auto current = 0;
                        for (auto& buffer : GetEditor().GetBuffers())
                        {
                            if (index == current)
                            {
                                m_pCurrentWindow->SetCurrentBuffer(buffer.get());
                            }
                            current++;
                        }
                    }
                    catch (std::exception&)
                    {
                    }
                }
            }
            else if (!GetEditor().Broadcast(std::make_shared<PicoVimMessage>(Msg_HandleCommand, command)))
            {
                m_pCurrentWindow->GetDisplay().SetCommandText("Not a command");
            }
            m_currentCommand.clear();
            return nullptr;
        }
    }
    else
    {
        // Unknown, keep trying
        handled = false;
        return nullptr;
    }


    // Store in a register
    if (!registers.empty())
    {
        if (op == CommandOperation::Delete ||
            op == CommandOperation::DeleteLines)
        {
            assert(beginRange <= endRange);
            std::string str = std::string(pBuffer->GetText().begin() + beginRange, pBuffer->GetText().begin() + endRange);

            // Delete commands fill up 1-9 registers
            if (command[0] == 'd' ||
                command[0] == 'D')
            {
                for (int i = 9; i > 1; i--)
                {
                    GetEditor().SetRegister('0' + i, GetEditor().GetRegister('0' + i - 1));
                }
                GetEditor().SetRegister('1', Register(str, (op == CommandOperation::DeleteLines)));
            }

            // Fill up any other required registers
            while (!registers.empty())
            {
                GetEditor().SetRegister(registers.top(), Register(str, (op == CommandOperation::DeleteLines)));
                registers.pop();
            }
        }
        else if (op == CommandOperation::Copy ||
            op == CommandOperation::CopyLines)
        {
            std::string str = std::string(pBuffer->GetText().begin() + beginRange, pBuffer->GetText().begin() + endRange);
            while (!registers.empty())
            {
                // Capital letters append to registers instead of replacing them
                if (registers.top() >= 'A' && registers.top() <= 'Z')
                {
                    auto chlow = std::tolower(registers.top());
                    GetEditor().GetRegister(chlow).text += str;
                    GetEditor().GetRegister(chlow).lineWise = (op == CommandOperation::CopyLines);
                }
                else
                {
                    GetEditor().GetRegister(registers.top()).text = str;
                    GetEditor().GetRegister(registers.top()).lineWise = (op == CommandOperation::CopyLines);
                }
                registers.pop();
            }
        }
    }

    // Handle command
    if (op == CommandOperation::Delete ||
        op == CommandOperation::DeleteLines)
    {
        auto cmd = std::make_shared<PicoVimCommand_DeleteRange>(*pBuffer,
            beginRange,
            endRange,
            cursorAfter != -1 ? cursorAfter : beginRange
            );
        spCommand = std::static_pointer_cast<PicoVimCommand>(cmd);
    }
    else if (op == CommandOperation::Insert && !pRegister->text.empty())
    {
        auto cmd = std::make_shared<PicoVimCommand_Insert>(*pBuffer,
            beginRange,
            pRegister->text,
            cursorAfter
            );
        spCommand = std::static_pointer_cast<PicoVimCommand>(cmd);
    }

    if (spCommand != nullptr)
        handled = true;

    return spCommand;
}

// Parse the command into:
// [count1] opA [count2] opB
// And generate (count1 * count2), opAopB
std::string PicoVimMode_Vim::GetCommandAndCount(std::string strCommand, int& count)
{
    std::string count1;
    std::string command1;
    std::string count2;
    std::string command2;

    if (strCommand == ".")
    {
        strCommand = m_lastCommand;
        count = m_lastCommandCount;
    }

    auto itr = strCommand.begin();
    while (itr != strCommand.end() && std::isdigit(*itr))
    {
        count1 += *itr;
        itr++;
    }

    while (itr != strCommand.end()
        && std::isgraph(*itr) && !std::isdigit(*itr))
    {
        command1 += *itr;
        itr++;
    }

    // If not a register target, then another count
    if (command1[0] != '\"' &&
        command1[0] != ':')
    {
        while (itr != strCommand.end()
            && std::isdigit(*itr))
        {
            count2 += *itr;
            itr++;
        }
    }

    while (itr != strCommand.end()
        && (std::isgraph(*itr) || *itr == ' '))
    {
        command2 += *itr;
        itr++;
    }

    count = 1;
    if (!count1.empty())
    {
        count = std::stoi(count1);
    }
    if (!count2.empty())
    {
        // When 2 counts are specified, they multiply!
        // Such as 2d2d, which deletes 4 lines
        count *= std::stoi(count2);
    }

    // Concatentate the parts of the command into a single command string
    std::string command = command1 + command2;

    // Short circuit - 0 is special, first on line.  Since we don't want to confuse it 
    // with a command count
    if (count == 0)
    {
        count = 1;
        return "0";
    }

    return command;
}

void PicoVimMode_Vim::AddKeyPress(uint32_t key, uint32_t modifierKeys)
{
    if (!m_pCurrentWindow)
        return;

    // Reset command text - we will update it later
    m_pCurrentWindow->GetDisplay().SetCommandText("");

    bool handled = true;
    if (m_currentMode == VimMode::Normal ||
        m_currentMode == VimMode::Visual)
    {
        // Escape wins all
        if (key == ExtKeys::ESCAPE)
        {
            SwitchMode(VimMode::Normal);
            return;
        }

        // Update the typed command
        m_currentCommand += char(key);

        // ... and show it in the command bar
        m_pCurrentWindow->GetDisplay().SetCommandText(m_currentCommand);

        // Retrieve the vim command
        int count;
        std::string command = GetCommandAndCount(m_currentCommand, count);
        if (command.empty())
        {
            // Nothing to do yet
            return;
        }

        for (int i = 0; i < count; i++)
        {
            auto spCommand = GetCommand(command, key, modifierKeys, m_currentMode, handled);
            if (spCommand)
            {
                if (count > 1)
                {
                    // Make command boundardies for groups of commands
                    if (i == 0 || i == (count - 1))
                    {
                        spCommand->SetFlags(CommandFlags::GroupBoundary);
                    }
                }

                // Remember the executed command for '.' operations
                if (i == 0)
                {
                    m_lastCommand = command;
                    m_lastCommandCount = count;
                }

                AddCommand(spCommand);
            }

            if (handled)
            {
                AfterCommand(spCommand);
            }
        }
    }
    // Inserting text...
    else if (m_currentMode == VimMode::Insert)
    {
        HandleInsert(key);
        handled = true;
    }

    if (handled)
    {
        ResetCommand();
    }
}

void PicoVimMode_Vim::HandleInsert(uint32_t key)
{
    auto cursor = m_pCurrentWindow->cursorCL;

    // Operations outside of inserts will pack up the insert operation 
    // and start a new one
    bool packCommand = false;
    switch (key)
    {
    case ExtKeys::ESCAPE:
    case ExtKeys::BACKSPACE:
    case ExtKeys::DEL:
    case ExtKeys::RIGHT:
    case ExtKeys::LEFT:
    case ExtKeys::UP:
    case ExtKeys::DOWN:
        packCommand = true;
        break;
    default:
        break;
    }

    if (m_pendingEscape)
    {
        // My custom 'jk' escape option
        auto canEscape = m_insertEscapeTimer.GetDelta() < .25f;
        if (canEscape && key == 'k')
        {
            packCommand = true;
            key = ExtKeys::ESCAPE;
        }
        m_pendingEscape = false;
    }

    const auto bufferCursor = m_pCurrentWindow->DisplayToBuffer(cursor);
    auto pBuffer = m_pCurrentWindow->GetCurrentBuffer();

    // Escape back to normal mode
    if (packCommand)
    {
        // End location is where we just finished typing
        auto insertEnd = bufferCursor;
        if (insertEnd > m_insertBegin)
        {
            // Get the string we inserted
            auto strInserted = std::string(pBuffer->GetText().begin() + m_insertBegin, pBuffer->GetText().begin() + insertEnd);

            // Temporarily remove it
            pBuffer->Delete(m_insertBegin, insertEnd);

            // Generate a command to put it in with undoable state
            // Leave cusor at the end
            auto cmd = std::make_shared<PicoVimCommand_Insert>(*pBuffer, m_insertBegin, strInserted, insertEnd);
            AddCommand(std::static_pointer_cast<PicoVimCommand>(cmd));
        }

        // Finished escaping
        if (key == ExtKeys::ESCAPE)
        {
            if (cursor.x != 0)
            {
                auto finalCursor = pBuffer->LocationFromOffsetByChars(insertEnd, -1);
                m_pCurrentWindow->MoveCursorTo(finalCursor);
            }

            // Back to normal mode
            SwitchMode(VimMode::Normal);
        }
        else
        {
            // Any other key here is a command while in insert mode
            /*
            auto spCommand = GetCommand("", key, modifierKeys, VimMode::Insert, handled);
            if (spCommand)
            {
                AddCommand(spCommand);
            }
            */
            assert(!"Fixme");
            SwitchMode(VimMode::Insert);
        }
        return;
    }

    auto buf = bufferCursor;
    std::string ch((char*)&key);
    if (key == ExtKeys::RETURN)
    {
        ch = "\r\n";
    }
    else if (key == ExtKeys::TAB)
    {
        // 4 Spaces, obviously :)
        ch = "    ";
    }

    if (key == 'j' && !m_pendingEscape)
    {
        m_insertEscapeTimer.Restart();
        m_pendingEscape = true;
    }
    else
    {
        // If we thought it was an escape but it wasn't, put the 'j' back in!
        if (m_pendingEscape)
        {
            ch = "j" + ch;
        }
        m_pendingEscape = false;

        pBuffer->Insert(buf, ch);

        // Insert back to normal mode should put the cursor on top of the last character typed.
        auto newCursor = pBuffer->LocationFromOffset(buf, long(ch.size()));
        m_pCurrentWindow->MoveCursorTo(newCursor, LineLocation::LineCRBegin);
    }
}

void PicoVimMode_Vim::SetCurrentWindow(PicoVimWindow* pWindow)
{
    PicoVimMode::SetCurrentWindow(pWindow);

    // If we thought it was an escape but it wasn't, put the 'j' back in!
    // TODO: Move to a more sensible place where we can check the time
    if (m_pendingEscape &&
        m_insertEscapeTimer.GetDelta() > .25f)
    {
        m_pendingEscape = false;
        auto buf = pWindow->DisplayToBuffer();
        pWindow->m_pCurrentBuffer->Insert(buf, "j");
        pWindow->MoveCursor(NVec2i(1, 0), LineLocation::LineCRBegin);
    }
}
} // PicoVim
