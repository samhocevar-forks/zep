#pragma once

#include "zep/mode_vim.h"
#include "zep/keymap.h"

extern "C" {
#include <orca-c/field.h>
#include <orca-c/gbuffer.h>
#include <orca-c/sim.h>
}

#include <mutex>

namespace Zep
{

class Orca
{
public:
    Orca();
    virtual ~Orca();

    void Init();
    void WriteToBuffer(ZepBuffer* pBuffer);
    void ReadFromBuffer(ZepBuffer* pBuffer);
    void SetTickInterval(float time);
    void SetTickCount(int count);
    void SetTestField(const NVec2i& fieldSize);
    void Enable(bool enable);
    void Quit();

    void RunThread();

private:
    std::mutex m_mutex;
    std::atomic_bool m_quit = false;
    std::atomic<float> m_tickInterval = .5f;
    std::atomic_bool m_enable = false;
    std::atomic_bool m_updated = false;
    std::thread m_thread;
    
    Field m_field;
    Mbuf_reusable m_mbuf_r;
    Oevent_list m_oevent_list;
    int m_tickCount = 0;
};

class ZepMode_Orca : public ZepMode_Vim
{
public:
    ZepMode_Orca(ZepEditor& editor);
    ~ZepMode_Orca();

    static const char* StaticName()
    {
        return "Orca";
    }

    static void Register(ZepEditor& editor);

    // Zep Mode
    virtual void Begin(ZepWindow* pWindow) override;
    virtual const char* Name() const override { return StaticName(); }
    virtual void PreDisplay(ZepWindow& win) override;

    virtual void SetupKeyMaps() override;

    virtual uint32_t ModifyWindowFlags(uint32_t flags) override;
private:
    ZepWindow* m_pCurrentWindow;
    std::map<ZepBuffer*, std::shared_ptr<Orca>> m_mapOrca;
};

} // namespace Zep
