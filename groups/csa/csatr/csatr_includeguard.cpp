// csatr_includeguard.cpp                                             -*-C++-*-

#include <clang/Basic/IdentifierTable.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Token.h>
#include <csabase_analyser.h>
#include <csabase_debug.h>
#include <csabase_diagnostic_builder.h>
#include <csabase_ppobserver.h>
#include <csabase_registercheck.h>
#include <csabase_report.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Regex.h>
#include <utils/event.hpp>
#include <utils/function.hpp>
#include <string>

namespace clang { class MacroDirective; }
namespace csabase { class Visitor; }

using namespace clang;
using namespace csabase;

// ----------------------------------------------------------------------------

static std::string const check_name("include-guard");

// -----------------------------------------------------------------------------

namespace
{

struct data
    // Data needed for include guard checking inside headers.
{
    data();
        // Create an object of this type.

    bool        d_test;     // true after guard is tested
    bool        d_define;   // trur after guard is defined
};

data::data()
: d_test(false)
, d_define(false)
{
}

struct report : Report<data>
    // Detect incorrect guard use in headers.
{
    INHERIT_REPORT_CTOR(report, Report, data);

    std::string guard();
        // Return the expected include guard.

    void operator()(SourceLocation, SourceRange range);
        // Process '#if' at the specified 'range'.

    void operator()(SourceLocation where, Token const& token);
        // Process '#ifndef' of the specified 'token' at the specified location
        // 'where'.

    void operator()(Token const& token, MacroDirective const*);
        // Process the '#define' of the specified 'token'.

    void operator()(SourceLocation location,
                    std::string const&,
                    std::string const& filename);
        // Process close of the specified 'file' at the specified 'location'.
};

std::string report::guard()
{
    return "INCLUDED_" + llvm::StringRef(a.component()).upper();
}

void report::operator()(SourceLocation, SourceRange range)
{
    if (a.is_component_header(range.getBegin()) && !d.d_test) {
        llvm::StringRef value = a.get_source(range);
        llvm::Regex re("^ *! *defined *[(]? *" + guard() + " *[)]? *$");
        if (!re.match(value)) {
            a.report(range.getBegin(), check_name, "TR14",
                              "Wrong include guard (expected '!defined(%0)')")
                << guard();
        }
        d.d_test = true;
    }
}

void report::operator()(SourceLocation where, Token const& token)
{
    if (a.is_component_header(token.getLocation()) &&
        !d.d_test) {
        if (IdentifierInfo const* id = token.getIdentifierInfo()) {
            if (id->getNameStart() != guard()) {
                a.report(token.getLocation(), check_name, "TR14",
                                  "Wrong name for include guard "
                                  "(expected '%0')")
                    << guard();
            }
            d.d_test = true;
        }
    }
}

void report::operator()(Token const& token, MacroDirective const*)
{
    if (a.is_component_header(token.getLocation())
        && !d.d_define
        && token.getIdentifierInfo()
        && token.getIdentifierInfo()->getNameStart() == guard()
        ) {
        d.d_define = true;
    }
}

void report::operator()(SourceLocation location,
                        std::string const&,
                        std::string const& filename)
{
    if (a.is_component_header(filename) &&
        !(d.d_test && d.d_define)) {
        a.report(location, check_name, "TR14",
                 d.d_test
                 ? "Missing define for include guard %0"
                 : d.d_define
                 ? "Missing test for include guard %0"
                 : "Missing include guard %0")
            << guard();
    }
}

// -----------------------------------------------------------------------------

void subscribe(Analyser& analyser, Visitor&, PPObserver& observer)
{
    observer.onIfndef       += report(analyser);
    observer.onIf           += report(analyser);
    observer.onMacroDefined += report(analyser);
    observer.onCloseFile    += report(analyser);
}

}

// ----------------------------------------------------------------------------

static RegisterCheck register_observer(check_name, &subscribe);

// ----------------------------------------------------------------------------
// Copyright (C) 2014 Bloomberg Finance L.P.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------- END-OF-FILE ----------------------------------
