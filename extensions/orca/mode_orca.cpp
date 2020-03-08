#include <cctype>
#include <sstream>
#include <thread>

#include "zep/mcommon/animation/timer.h"
#include "zep/mcommon/logger.h"
#include "zep/mcommon/string/stringutils.h"

#include "zep/buffer.h"
#include "zep/keymap.h"
#include "zep/tab_window.h"
#include "zep/theme.h"
#include "zep/window.h"

#include "mode_orca.h"
#include "syntax_orca.h"

namespace Zep
{

ZepMode_Orca::ZepMode_Orca(ZepEditor& editor)
    : ZepMode_Vim(editor)
{
}

ZepMode_Orca::~ZepMode_Orca()
{
}

void ZepMode_Orca::SetupKeyMaps()
{
    // Standard choices
    AddGlobalKeyMaps();
    AddNavigationKeyMaps(true);
    AddSearchKeyMaps();
    AddOverStrikeMaps();
    AddCopyMaps();

    // Mode switching
    AddKeyMapWithCountRegisters({ &m_normalMap, &m_visualMap }, { "<Escape>" }, id_NormalMode);
    keymap_add({ &m_insertMap }, { "jk" }, id_NormalMode);
    keymap_add({ &m_insertMap }, { "<Escape>" }, id_NormalMode);
    AddKeyMapWithCountRegisters({ &m_visualMap }, { ":", "/", "?" }, id_ExMode);

    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<Return>" }, id_MotionNextFirstChar);

    // Undo redo
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<C-r>" }, id_Redo);
    AddKeyMapWithCountRegisters({ &m_normalMap }, { "<C-z>", "u" }, id_Undo);
}

void ZepMode_Orca::Begin(ZepWindow* pWindow)
{
    ZepMode_Vim::Begin(pWindow);

    auto itr = m_mapOrca.find(&pWindow->GetBuffer());
    if (itr != m_mapOrca.end())
    {
        return;
    }

    auto pOrca = std::make_shared<Orca>();
    m_mapOrca[&pWindow->GetBuffer()] = pOrca;
    pOrca->Init();
    pOrca->ReadFromBuffer(&pWindow->GetBuffer());
    pOrca->Enable(true);
}

// Modify flags for the window in this mode
uint32_t ZepMode_Orca::ModifyWindowFlags(uint32_t flags)
{
    flags = ZClearFlags(flags, WindowFlags::WrapText);
    flags = ZClearFlags(flags, WindowFlags::ShowLineNumbers);
    flags = ZClearFlags(flags, WindowFlags::ShowIndicators);
    flags = ZSetFlags(flags, WindowFlags::GridStyle);
    flags = ZSetFlags(flags, WindowFlags::HideScrollBar);
    //flags = ZSetFlags(flags, WindowFlags::HideSplitMark);
    return flags;
}

void ZepMode_Orca::PreDisplay(ZepWindow& window)
{
    auto itrFound = m_mapOrca.find(&window.GetBuffer());
    if (itrFound != m_mapOrca.end())
    {
        itrFound->second->WriteToBuffer(&window.GetBuffer());
    }

    ZepMode_Vim::PreDisplay(window);
}

static std::unordered_set<std::string> orca_keywords = {};
static std::unordered_set<std::string> orca_identifiers = {};

void ZepMode_Orca::Register(ZepEditor& editor)
{
    editor.RegisterSyntaxFactory(
        { ".orca" },
        SyntaxProvider{ "orca", tSyntaxFactory([](ZepBuffer* pBuffer) {
                           return std::make_shared<ZepSyntax_Orca>(*pBuffer, orca_keywords, orca_identifiers, ZepSyntaxFlags::CaseInsensitive);
                       }) });

    editor.RegisterBufferMode(".orca", std::make_shared<ZepMode_Orca>(editor));
}

Orca::Orca()
{
}

Orca::~Orca()
{
    Quit();

    mbuf_reusable_deinit(&m_mbuf_r);
    oevent_list_deinit(&m_oevent_list);
    field_deinit(&m_field);
}

void Orca::Init()
{
    field_init(&m_field);
    oevent_list_init(&m_oevent_list);
    mbuf_reusable_init(&m_mbuf_r);

    m_thread = std::thread([&]() {
        RunThread();
    });
}

void Orca::ReadFromBuffer(ZepBuffer* pBuffer)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    ByteIndex start, end;
    pBuffer->GetLineOffsets(0, start, end);

    field_resize_raw_if_necessary(&m_field, pBuffer->GetLineCount(), (end - start - 1));

    auto& text = pBuffer->GetText();
    for (int y = 0; y < m_field.height; y++)
    {
        for (int x = 0; x < m_field.width; x++)
        {
            m_field.buffer[y * m_field.width + x] = text[y * (m_field.width + 1) + x];
        }
    }

    mbuf_reusable_ensure_size(&m_mbuf_r, m_field.height, m_field.width);
}

void Orca::WriteToBuffer(ZepBuffer* pBuffer)
{
    if (!m_updated.load())
    {
        return;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    auto& text = pBuffer->GetMutableText();
    for (int y = 0; y < m_field.height; y++)
    {
        for (int x = 0; x < m_field.width; x++)
        {
            text[y * (m_field.width + 1) + x] = m_field.buffer[y * m_field.width + x];
        }
    }

    // Special syntax trigger, since we do things differently with orca
    auto pSyntax = dynamic_cast<ZepSyntax_Orca*>(pBuffer->GetSyntax());
    if (pSyntax)
    {
        pSyntax->UpdateSyntax();
    }
    
    m_updated.store(false);
}

void Orca::RunThread()
{
    for (;;)
    {
        if (m_quit.load())
            break;

        if (!m_enable)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (m_updated.load())
        { 
            std::this_thread::yield();
            continue;
        }

        // Lock zone
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            TIME_SCOPE(OrcaUpdate)
            mbuffer_clear(m_mbuf_r.buffer, m_field.height, m_field.width);
            oevent_list_clear(&m_oevent_list);
            orca_run(m_field.buffer, m_mbuf_r.buffer, m_field.height, m_field.width, m_tickCount++, &m_oevent_list, 0);
            m_updated.store(true);
        }
    }
}

void Orca::SetTickInterval(float time)
{
    m_tickInterval.store(time);
}

void Orca::SetTickCount(int count)
{
    m_tickCount = count;
}

void Orca::SetTestField(const NVec2i& fieldSize)
{
    ZEP_UNUSED(fieldSize);
}

void Orca::Enable(bool enable)
{
    m_enable.store(enable);
}

void Orca::Quit()
{
    // Check if already quit
    if (m_quit.load())
    {
        return;
    }

    // Quit and wait
    m_quit.store(true);
    m_thread.join();
}

} // namespace Zep
