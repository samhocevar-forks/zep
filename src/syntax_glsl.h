#pragma once
#include <string>
#include "syntax.h"

namespace PicoVim
{

class PicoVimSyntaxGlsl : public PicoVimSyntax
{
public:
    using TParent = PicoVimSyntax;
    PicoVimSyntaxGlsl(PicoVimBuffer& buffer);
    virtual ~PicoVimSyntaxGlsl();

    virtual void UpdateSyntax(long startOffset, long endOffset) override;

    virtual uint32_t GetColor(long i) const override;
    virtual SyntaxType GetType(long i) const override;
    virtual void Interrupt() override;

    std::set<std::string> keywords;
    std::atomic<bool> m_stop;
};

} // PicoVim
