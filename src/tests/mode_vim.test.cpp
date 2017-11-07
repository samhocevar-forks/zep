#include "m3rdparty.h"
#include <gtest/gtest.h>
#include "src/editor.h"
#include "src/mode_vim.h"
#include "src/buffer.h"
#include "src/display.h"
#include "src/syntax_glsl.h"

using namespace PicoVim;
class VimTest : public testing::Test
{
public:
    VimTest()
    {
        // Disable threads for consistent tests, at the expense of not catching thread errors!
        spEditor = std::make_shared<PicoVimEditor>(PicoVimEditorFlags::DisableThreads);
        spMode = std::make_shared<PicoVimMode_Vim>(*spEditor);
        spBuffer = spEditor->AddBuffer("Test Buffer");

        // Add a syntax highlighting checker, to increase test coverage
        // (seperate tests to come)
        auto spSyntax = std::make_shared<PicoVimSyntaxGlsl>(*spBuffer);
        spBuffer->SetSyntax(std::static_pointer_cast<PicoVimSyntax>(spSyntax));

        // Some vim commands depend on the shape of the view onto the buffer.  For example, 
        // moving 'down' might depend on word wrapping, etc. as to where in the buffer you actually land.
        // This is the reason for the difference between a 'Window' and a 'Buffer' (as well as that you can
        // have multiple windows onto a buffer)
        spDisplay = std::make_shared<PicoVimDisplayNull>(*spEditor);
        spDisplay->SetDisplaySize(NVec2f(0.0f, 0.0f), NVec2f(1024.0f, 1024.0f));

        pWindow = spDisplay->AddWindow();
        pWindow->SetCurrentBuffer(spBuffer);
        spMode->SetCurrentWindow(pWindow);
        pWindow->cursorCL = NVec2i(0, 0);
    }

public:
    std::shared_ptr<PicoVimEditor> spEditor;
    std::shared_ptr<PicoVimDisplayNull> spDisplay;
    PicoVimBuffer* spBuffer;
    PicoVimWindow* pWindow;
    std::shared_ptr<PicoVimMode_Vim> spMode;
};

TEST_F(VimTest, CheckDisplaySucceeds)
{
    spBuffer->SetText("Some text to display\nThis is a test.");
    ASSERT_NO_FATAL_FAILURE(spDisplay->Display());
}

// Given a sample text, a keystroke list and a target text, check the test returns the right thing
#define COMMAND_TEST(name, source, command, target) \
TEST_F(VimTest, name)                              \
{                                                   \
    spBuffer->SetText(source);                      \
    spMode->AddCommandText(command);                \
    ASSERT_STREQ(spBuffer->GetText().string().c_str(), target);     \
};

// The various rules of vim keystrokes are hard to consolidate.
// diw deletes inner word for example, but also deletes just whitespace if you are on it.
// daw deletes the whole word, but also the first spaces up to it, or the last spaces after it....
// The best way to check these behaviours is of course with some unit that check each behaviour
// These are below.  I wil likely never write enough of these!

// daw
COMMAND_TEST(delete_daw_single_char, "(one) two three", "daw", "one) two three");
COMMAND_TEST(delete_daw_start_space, "(one) two three", "llldaw", "() two three");
COMMAND_TEST(delete_daw_inside_string, "(one) two three", "llllldaw", "(one) three");
COMMAND_TEST(delete_daw_2, "(one) two three", "2daw", ") two three");

// daW
COMMAND_TEST(delete_daW_single_char, "(one) two three", "daW", "two three");
COMMAND_TEST(delete_daW_start_space, "(one) two three", "llldaW", "two three");
COMMAND_TEST(delete_daW_inside_string, "(one) two three", "llllldaW", "(one) three");
COMMAND_TEST(delete_daW_2, "one two three", "2daW", "three");

// diw
COMMAND_TEST(delete_diw_first, "one two three", "diw", " two three");
COMMAND_TEST(delete_diw_start_space, "one two three", "llldiw", "onetwo three");
COMMAND_TEST(delete_diw_inner_spaces, "one    two three", "llllldiw", "onetwo three");
COMMAND_TEST(delete_diw_inside_string, "one two three", "ldiw", " two three");
COMMAND_TEST(delete_diw_2, "one two three", "2diw", "two three");
COMMAND_TEST(delete_diw_space_start, " one two three", "diw", "one two three");

// dw
COMMAND_TEST(delete_dw, "one two three", "dw", "two three");
COMMAND_TEST(delete_dw_inside, "one two three", "ldw", "otwo three");
COMMAND_TEST(delete_dw_inside_2, "one two three", "lllllllldw", "one two ");

// paste
COMMAND_TEST(paste_p_at_end_cr, "(one) two three\r\n", "vllllxlllllllllllljp", " two three\r\n(one)");
COMMAND_TEST(paste_p_at_end, "(one) two three", "vllllxllllllllllllp", " two three(one)");
COMMAND_TEST(paste_P_at_end, "(one) two three", "vllllxllllllllllllP", " two thre(one)e");
COMMAND_TEST(paste_P_middle, "(one) two three", "llllllvlylp", "(one) twotw three");

// Join
COMMAND_TEST(join_lines, "one\ntwo", "J", "onetwo");

// Insert
COMMAND_TEST(insert_a_text,"one three", "lllatwo ", "one two three")
COMMAND_TEST(insert_i_text,"one three", "lllitwo", "onetwo three")
COMMAND_TEST(insert_A_end,"one three", "lllA four", "one three four")
COMMAND_TEST(insert_I,"   one three", "llllllllIzero jk", "   zero one three")

COMMAND_TEST(registers,"one", ":reg\n", "one")
COMMAND_TEST(buffers,"one", ":ls\n", "one")

#define CURSOR_TEST(name, source, command, xcoord, ycoord) \
TEST_F(VimTest, name)                               \
{                                                   \
    spBuffer->SetText(source);                      \
    spMode->AddCommandText(command);                \
    ASSERT_EQ(pWindow->cursorCL.x, xcoord);     \
    ASSERT_EQ(pWindow->cursorCL.y, ycoord);     \
};

// Motions
CURSOR_TEST(motion_lright, "one", "ll", 2, 0);
CURSOR_TEST(motion_lright_2, "one", "2l", 2, 0);
CURSOR_TEST(motion_lright_limit, "one", "llllll", 2, 0);
CURSOR_TEST(motion_hleft_limit, "one", "h", 0, 0);
CURSOR_TEST(motion_hleft, "one", "llh", 1, 0);
CURSOR_TEST(motion_jdown, "one\ntwo", "j", 0, 1);
CURSOR_TEST(motion_jdown_lright, "one\ntwo", "jl", 1, 1);
CURSOR_TEST(motion_kup, "one\ntwo", "jk", 0, 0);
CURSOR_TEST(motion_kup_lright, "one\ntwo", "jkl", 1, 0);
CURSOR_TEST(motion_kup_limit, "one\ntwo", "kkkkkkkk", 0, 0);
CURSOR_TEST(motion_jdown_limit, "one\ntwo", "jjjjjjjjj", 0, 1);
CURSOR_TEST(motion_jklh_find_center, "one\ntwo\nthree", "jjlk", 1, 1);

CURSOR_TEST(motion_goto_endline, "one two", "$", 6, 0);
CURSOR_TEST(motion_goto_enddoc, "one\ntwo", "G", 2, 1);
CURSOR_TEST(motion_goto_begindoc, "one\ntwo", "lljgg", 0, 0);
CURSOR_TEST(motion_goto_beginline, "one two", "lllll0", 0, 0);
CURSOR_TEST(motion_goto_firstlinechar, "   one two", "^", 3, 0);

CURSOR_TEST(motion_w, "one! two three", "w", 3, 0);
CURSOR_TEST(motion_w_space, "one two three", "lllw", 4, 0);
CURSOR_TEST(motion_W, "one! two three", "W", 5, 0);
CURSOR_TEST(motion_b, "one! two three", "wwb", 3, 0);
CURSOR_TEST(motion_B, "one! two three", "wwwB", 5, 0);

CURSOR_TEST(motion_e, "one! two three", "eee", 7, 0);
CURSOR_TEST(motion_e_space, "one two three", "llle", 6, 0);
CURSOR_TEST(motion_E, "one! two three", "EE", 7, 0);
CURSOR_TEST(motion_ge, "one! two three", "wwwge", 7, 0);
