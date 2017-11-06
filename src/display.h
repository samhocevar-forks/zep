#pragma once

#include "buffer.h"
#include "window.h"
#include "utils/timer.h"

namespace PicoVim
{

const float bottomBorder = 4.0f;
const float textBorder = 4.0f;
const float leftBorder = 30.0f;

class PicoVimDisplay : public PicoVimComponent
{
public:
    using tWindows = std::set<std::shared_ptr<PicoVimWindow>>;

    PicoVimDisplay(PicoVimEditor& editor);
    virtual ~PicoVimDisplay();

    void SetDisplaySize(const NVec2f& topLeft, const NVec2f& bottomRight);

    void PreDisplay();
    void Display();

    virtual void Notify(std::shared_ptr<PicoVimMessage> spMsg);

    // Renderer specific overrides
    // Implement these to draw the buffer using whichever system you prefer
    virtual NVec2f GetTextSize(const utf8* pBegin, const utf8* pEnd = nullptr) const = 0;
    virtual float GetFontSize() const = 0;
    virtual void DrawLine(const NVec2f& start, const NVec2f& end, uint32_t color = 0xFFFFFFFF, float width = 1.0f) const = 0;
    virtual void DrawChars(const NVec2f& pos, uint32_t col, const utf8* text_begin, const utf8* text_end = nullptr) const = 0;
    virtual void DrawRectFilled(const NVec2f& a, const NVec2f& b, uint32_t col = 0xFFFFFFFF) const = 0;

    void SetCommandText(const std::string& strCommand);

    // Setup display for any window if there is none
    void AssignDefaultWindow();

    // Window management
    void SetCurrentWindow(PicoVimWindow* pWindow);
    PicoVimWindow* GetCurrentWindow() const;
    PicoVimWindow* AddWindow();
    void RemoveWindow(PicoVimWindow* pWindow);
    const tWindows& GetWindows() const;

    void RequestRefresh();
    bool RefreshRequired() const;

    bool GetCursorBlinkState() const;
    void ResetCursorTimer() { m_cursorTimer.Restart(); }

protected:
    void DrawRegion(const Region& region);

protected:
    // TODO: A splitter manager
    DisplayRegion m_windowRegion;
    DisplayRegion m_commandRegion;

    NVec2f m_topLeftPx;
    NVec2f m_bottomRightPx;

    // Blinking cursor
    Timer m_cursorTimer;
    mutable bool m_bPendingRefresh = true;
    mutable bool m_lastCursorBlink = false;

    // One window for now
    tWindows m_windows;
    PicoVimWindow* m_pCurrentWindow = nullptr;

    std::vector<std::string> m_commandLines;        // Command information, shown under the buffer
};

// A NULL renderer, used for testing
// Discards all drawing, and returns text size of 1 pixel per char!
// This is the only work you need to do to make a new renderer, other than ImGui
class PicoVimDisplayNull : public PicoVimDisplay
{
public:
    PicoVimDisplayNull(PicoVimEditor& editor)
        : PicoVimDisplay(editor)
    {

    }
    virtual NVec2f GetTextSize(const utf8* pBegin, const utf8* pEnd = nullptr) const { return NVec2f(float(pEnd - pBegin), 10.0f); }
    virtual float GetFontSize() const { return 10; };
    virtual void DrawLine(const NVec2f& start, const NVec2f& end, uint32_t color = 0xFFFFFFFF, float width = 1.0f) const { };
    virtual void DrawChars(const NVec2f& pos, uint32_t col, const utf8* text_begin, const utf8* text_end = nullptr) const { };
    virtual void DrawRectFilled(const NVec2f& a, const NVec2f& b, uint32_t col = 0xFFFFFFFF) const {};
};

} // PicoVim
