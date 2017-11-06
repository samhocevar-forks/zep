#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include <memory>
#include <threadpool/ThreadPool.hpp>

// Basic Architecture

// Editor
//      Buffers
//      Modes -> (Active BufferRegion)
// Display
//      BufferRegions (->Buffers)
//
// A buffer is just an array of chars in a gap buffer, with simple operations to insert, delete and search
// A display is something that can display a collection of regions and the editor controls in a window
// A buffer region is a single view onto a buffer inside the main display
//
// The editor has a list of PicoVimBuffers.
// The editor has different editor modes (vim/standard)
// PicoVimDisplay can render the editor (with imgui or something else)
// The display has multiple BufferRegions, each a window onto a buffer.
// Multiple regions can refer to the same buffer (N Regions : N Buffers)
// The Modes receive key presses and act on a buffer region
namespace PicoVim
{

class PicoVimBuffer;
class PicoVimMode;
class PicoVimMode_Vim;
class PicoVimMode_Standard;
class PicoVimEditor;

// Helper for 2D operations
template<class T>
struct NVec2
{
    NVec2(T xVal, T yVal)
        : x(xVal),
        y(yVal)
    {}

    NVec2()
        : x(0),
        y(0)
    {}

    T x;
    T y;
};
template<class T> inline NVec2<T> operator+ (const NVec2<T>& lhs, const NVec2<T>& rhs) { return NVec2<T>(lhs.x + rhs.x, lhs.y + rhs.y); }
template<class T> inline NVec2<T> operator- (const NVec2<T>& lhs, const NVec2<T>& rhs) { return NVec2<T>(lhs.x - rhs.x, lhs.y - rhs.y); }
template<class T> inline NVec2<T>& operator+= (NVec2<T>& lhs, const NVec2<T>& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
template<class T> inline NVec2<T>& operator-= (NVec2<T>& lhs, const NVec2<T>& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
template<class T> inline NVec2<T> operator* (const NVec2<T>& lhs, float val) { return NVec2<T>(lhs.x * val, lhs.y * val); }
template<class T> inline NVec2<T>& operator*= (NVec2<T>& lhs, float val) { lhs.x *= val; lhs.y *= val; return lhs; }
template<class T> inline NVec2<T> Clamp(const NVec2<T>& val, const NVec2<T>& min, const NVec2<T>& max)
{
    return NVec2<T>(std::min(max.x, std::max(min.x, val.x)), std::min(max.y, std::max(min.y, val.y)));
}

using NVec2f = NVec2<float>;
using NVec2i = NVec2<long>;

using utf8 = uint8_t;

extern const char* Msg_GetClipBoard;
extern const char* Msg_SetClipBoard;
extern const char* Msg_HandleCommand;

class PicoVimMessage
{
public:
    PicoVimMessage(const char* id, const std::string& strIn = std::string())
        : messageId(id),
        str(strIn)
    { }

    const char* messageId;      // Message ID 
    std::string str;            // Generic string for simple messages
    bool handled = false;       // If the message was handled
};

struct IPicoVimClient
{
    virtual void Notify(std::shared_ptr<PicoVimMessage> message) = 0;
    virtual PicoVimEditor& GetEditor() const = 0;
};

class PicoVimComponent : public IPicoVimClient
{
public:
    PicoVimComponent(PicoVimEditor& editor);
    virtual ~PicoVimComponent();
    PicoVimEditor& GetEditor() const override { return m_editor; }

private:
    PicoVimEditor& m_editor;
};

// Registers are used by the editor to store/retrieve text fragments
struct Register
{
    Register() : text(""), lineWise(false) {}
    Register(const char* ch, bool lw = false) : text(ch), lineWise(lw) {}
    Register(utf8* ch, bool lw = false) : text((const char*)ch), lineWise(lw) {}
    Register(const std::string& str, bool lw = false) : text(str), lineWise(lw) {}

    std::string text;
    bool lineWise = false;
};

using tRegisters = std::map<std::string, Register>;
using tBuffers = std::deque<std::shared_ptr<PicoVimBuffer>>;

namespace PicoVimEditorFlags
{
enum
{
    None = (0),
    DisableThreads = (1 << 0)
};
};

class PicoVimEditor
{
public:

    PicoVimEditor(uint32_t flags = 0);
    ~PicoVimEditor();
    PicoVimMode* GetMode() const;
    void SetMode(PicoVimMode* pMode);

    bool Broadcast(std::shared_ptr<PicoVimMessage> payload);
    void RegisterCallback(IPicoVimClient* pClient) { m_notifyClients.insert(pClient); }
    void UnRegisterCallback(IPicoVimClient* pClient) { m_notifyClients.erase(pClient); }

    const tBuffers& GetBuffers() const;
    PicoVimBuffer* AddBuffer(const std::string& str);
    PicoVimBuffer* GetMRUBuffer() const;

    void SetRegister(const std::string& reg, const Register& val);
    void SetRegister(const char reg, const Register& val);
    void SetRegister(const std::string& reg, const char* pszText);
    void SetRegister(const char reg, const char* pszText);
    Register& GetRegister(const std::string& reg);
    Register& GetRegister(const char reg);
    const tRegisters& GetRegisters() const;

    void Notify(std::shared_ptr<PicoVimMessage> message);
    uint32_t GetFlags() const { return m_flags; }

private:
    std::set<IPicoVimClient*> m_notifyClients;
    mutable tRegisters m_registers;

    // Active mode
    PicoVimMode* m_pMode = nullptr;

    // List of buffers that the editor is managing
    // May or may not be visible
    tBuffers m_buffers;
    uint32_t m_flags = 0;
};

} // PicoVim
