#pragma once

#include "zep/mode.h"
#include <future>
#include <memory>
#include <regex>

namespace Zep
{

// A provider that can handle repl commands
// This is just a default repl that does nothing; if you want to provide a repl 
// you need to register this interface and handle the messages to run the repl.
struct IZepReplProvider
{
    virtual std::string ReplParse(const std::string& input) 
    {
        // Just reflect the input
        return input;
    };
    virtual bool ReplIsFormComplete(const std::string& input, int& depth)
    {
        // The default repl assumes all commands are complete.
        ZEP_UNUSED(input);
        depth = 0;
        return true;
    }
};

class ZepReplExCommand : public ZepExCommand
{
public:
    ZepReplExCommand(ZepEditor& editor, IZepReplProvider* pProvider)
        : ZepExCommand(editor),
        m_pProvider(pProvider)
    {}

    static void Register(ZepEditor& editor, IZepReplProvider* pProvider);
    
    virtual void Notify(std::shared_ptr<ZepMessage> message) override { ZEP_UNUSED(message); }
    virtual void Run(const std::vector<std::string>& args) override;
    virtual const char* Name() const override { return "ZRepl"; }
private:
    IZepReplProvider* m_pProvider = nullptr;
};

class ZepMode_Repl : public ZepMode
{
public:
    ZepMode_Repl(ZepEditor& editor, IZepReplProvider* provider);
    ~ZepMode_Repl();

    virtual void AddKeyPress(uint32_t key, uint32_t modifiers = 0) override;
    virtual void Begin(ZepWindow* pWindow) override;
    virtual void Notify(std::shared_ptr<ZepMessage> message) override;
    
    static const char* StaticName()
    {
        return "REPL";
    }
    virtual const char* Name() const override
    {
        return StaticName();
    }

    void Prompt();
    void MoveToEnd();
private:
    void Close();

private:
    ByteIndex m_startLocation = ByteIndex{ 0 };
    IZepReplProvider* m_pRepl;
};

} // namespace Zep
