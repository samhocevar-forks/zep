#include "zep/editor.h"
#include "zep/theme.h"

#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

#include "syntax_orca.h"

#include <string>
#include <vector>

namespace Zep
{

ZepSyntax_Orca::ZepSyntax_Orca(ZepBuffer& buffer,
    const std::unordered_set<std::string>& keywords,
    const std::unordered_set<std::string>& identifiers,
    uint32_t flags)
    : ZepSyntax(buffer, keywords, identifiers, flags)
{
    // Don't need default
    m_adornments.clear();
}

/*
typedef enum
{
    Glyph_class_unknown,
    Glyph_class_grid,
    Glyph_class_comment,
    Glyph_class_uppercase,
    Glyph_class_lowercase,
    Glyph_class_movement,
    Glyph_class_numeric,
    Glyph_class_bang,
} Glyph_class;

static Glyph_class glyph_class_of(Glyph glyph)
{
    if (glyph == '.')
        return Glyph_class_grid;
    if (glyph >= '0' && glyph <= '9')
        return Glyph_class_numeric;
    switch (glyph)
    {
    case 'N':
    case 'n':
    case 'E':
    case 'e':
    case 'S':
    case 's':
    case 'W':
    case 'w':
    case 'Z':
    case 'z':
        return Glyph_class_movement;
    case '!':
    case ':':
    case ';':
    case '=':
    case '%':
    case '?':
        return Glyph_class_lowercase;
    case '*':
        return Glyph_class_bang;
    case '#':
        return Glyph_class_comment;
    }
    if (glyph >= 'A' && glyph <= 'Z')
        return Glyph_class_uppercase;
    if (glyph >= 'a' && glyph <= 'z')
        return Glyph_class_lowercase;
    return Glyph_class_unknown;
}

static attr_t term_attrs_of_cell(Glyph g, Mark m) {
  Glyph_class gclass = glyph_class_of(g);
  attr_t attr = A_normal;
  switch (gclass) {
  case Glyph_class_unknown:
    attr = A_bold | fg_bg(C_red, C_natural);
    break;
  case Glyph_class_grid:
    attr = A_bold | fg_bg(C_black, C_natural);
    break;
  case Glyph_class_comment:
    attr = A_dim | Cdef_normal;
    break;
  case Glyph_class_uppercase:
    attr = A_normal | fg_bg(C_black, C_cyan);
    break;
  case Glyph_class_lowercase:
  case Glyph_class_movement:
  case Glyph_class_numeric:
    attr = A_bold | Cdef_normal;
    break;
  case Glyph_class_bang:
    attr = A_bold | Cdef_normal;
    break;
  }
  if (gclass != Glyph_class_comment) {
    if ((m & (Mark_flag_lock | Mark_flag_input)) ==
        (Mark_flag_lock | Mark_flag_input)) {
      // Standard locking input
      attr = A_normal | Cdef_normal;
    } else if ((m & Mark_flag_input) == Mark_flag_input) {
      // Non-locking input
      attr = A_normal | Cdef_normal;
    } else if (m & Mark_flag_lock) {
      // Locked only
      attr = A_dim | Cdef_normal;
    }
  }
  if (m & Mark_flag_output) {
    attr = A_reverse;
  }
  if (m & Mark_flag_haste_input) {
    attr = A_bold | fg_bg(C_cyan, C_natural);
  }
  return attr;
}
*/

NVec4f cyan = NVec4f(0, 1.0f, 1.0f, 1.0f);
SyntaxResult ZepSyntax_Orca::GetSyntaxAt(long index) const
{
    SyntaxResult res;
    res.foreground = m_syntax[index].foreground;
    res.background = m_syntax[index].background;
    return res;
    /*
    auto& buffer = m_buffer.GetText();
    Glyph g = buffer[index];

    SyntaxResult data;
    if (m_markBuffer.size() <= index)
    {
        return data;
    }

    //auto m = m_markBuffer[index];
    Glyph_class gclass = glyph_class_of(g);
    switch (gclass)
    {
    case Glyph_class_unknown:
        data.background = ThemeColor::Error; // Bold
        break;
    case Glyph_class_grid:
        data.foreground = ThemeColor::Background; // Bold
        break;
    case Glyph_class_comment:
        data.foreground = ThemeColor::TabInactive;
        break;
    case Glyph_class_uppercase:
        data.background = ThemeColor::Custom;
        data.customBackgroundColor = cyan;
        data.foreground = ThemeColor::Background;
        break;
    case Glyph_class_lowercase:
        data.foreground = ThemeColor::Custom;
        data.customForegroundColor = cyan;
        break;
    case Glyph_class_movement:
        data.foreground = ThemeColor::Background;
        data.background = ThemeColor::Text;
        break;
    case Glyph_class_numeric:
    case Glyph_class_bang:
        break;
    }

    /*
    if (gclass != Glyph_class_comment)
    {
        if ((m & (Mark_flag_lock | Mark_flag_input)) == (Mark_flag_lock | Mark_flag_input))
        {
            // Standard locking input
            //attr = A_normal | Cdef_normal;
            //data.foreground = ThemeColor::Text;
        }
        else if ((m & Mark_flag_input) == Mark_flag_input)
        {
            // Non-locking input
            //attr = A_normal | Cdef_normal;
           // data.foreground = ThemeColor::Text;
        }
        else if (m & Mark_flag_lock)
        {
            // Locked only
            // attr = A_dim | Cdef_normal;
            data.foreground = ThemeColor::TabInactive;
        }
    }
    if (m & Mark_flag_output)
    {
        // Reverse
        data.foreground = ThemeColor::Background;
        data.background = ThemeColor::Text;
    }
    if (m & Mark_flag_haste_input)
    {
        data.foreground = ThemeColor::Custom;
        data.customForegroundColor = cyan;
    }
    return data;
    */
}

void ZepSyntax_Orca::UpdateSyntax()
{
    auto& buffer = m_buffer.GetText();

    std::unordered_map<uint8_t, int> outputOpCount;
    outputOpCount[':'] = 6;

    m_syntax.resize(buffer.size());
    ByteIndex i = 0;
    for (;;)
    {
        // Skip spaces and newlines
        while (buffer[i] != 0 && buffer[i] == ' ' || buffer[i] == '.' || buffer[i] == '\n')
        {
            m_syntax[i].background = ThemeColor::Background;
            m_syntax[i].foreground = ThemeColor::Whitespace;
            i++;
        }

        // Skip comments
        if (buffer[i] == '#')
        {
            m_syntax[i].background = ThemeColor::Background;
            m_syntax[i].foreground = ThemeColor::Comment;
            i++;

            while (buffer[i] != 0 && buffer[i] != '\n')
            {
                m_syntax[i].background = ThemeColor::Background;
                m_syntax[i].foreground = ThemeColor::Comment;

                if (buffer[i] == '#')
                {
                    i++;
                    break;
                }
                i++;
            }
            continue;
        }

        bool flash = false;
        if (buffer[i] >= 'A' && buffer[i] <= 'Z')
        {
            m_syntax[i].background = ThemeColor::Background;
            m_syntax[i].foreground = ThemeColor::Keyword;
        }
        else if (buffer[i] >= 'a' && buffer[i] <= 'z')
        {
            m_syntax[i].background = ThemeColor::Background;
            m_syntax[i].foreground = ThemeColor::Identifier;
        }
        else if (buffer[i] >= '0' && buffer[i] <= '9')
        {
            m_syntax[i].background = ThemeColor::Background;
            m_syntax[i].foreground = ThemeColor::Number;
        }
        else if (buffer[i] == '*')
        {
            m_syntax[i].background = ThemeColor::FlashColor;
            m_syntax[i].foreground = ThemeColor::Normal;
            flash = true;
            i++;
        }

        // Last check
        auto itrOut = outputOpCount.find(buffer[i]);
        if (itrOut != outputOpCount.end())
        { 
            int count = itrOut->second;
            while (count > 0 && buffer[i] != 0 && buffer[i] != '\n')
            {
                m_syntax[i].background = ThemeColor::Background;
                m_syntax[i].foreground = ThemeColor::String;
                i++;
                count--;
            }
            continue;
        }

        if (buffer[i] == 0)
            break;

        i++;
    }

    // We don't do anything in orca mode, we get dynamically because Orca tells us the colors
    m_targetChar = long(0);
    m_processedChar = long(buffer.size() - 1);
}

} // namespace Zep
