// csatr_includeguard.cpp                                             -*-C++-*-

#include <csabase_analyser.h>
#include <csabase_debug.h>
#include <csabase_location.h>
#include <csabase_ppobserver.h>
#include <csabase_registercheck.h>
#include <llvm/Support/Regex.h>
#include <cctype>

using namespace clang;
using namespace bde_verify::csabase;

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

struct report
    // Detect incorrect guard use in headers.
{
    report(Analyser& analyser);
        // Create an object of this type using the specified analyser.

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

    Analyser& d_analyser;
    data& d_data;
};

report::report(Analyser& analyser)
: d_analyser(analyser)
, d_data(analyser.attachment<data>())
{
}

std::string report::guard()
{
    return "INCLUDED_" + llvm::StringRef(d_analyser.component()).upper();
}

void report::operator()(SourceLocation, SourceRange range)
{
    if (d_analyser.is_component_header(range.getBegin()) && !d_data.d_test) {
        llvm::StringRef value = d_analyser.get_source(range);
        llvm::Regex re("^ *! *defined *[(]? *" + guard() + " *[)]? *$");
        if (!re.match(value)) {
            d_analyser.report(range.getBegin(), check_name, "TR14",
                              "Wrong include guard (expected '!defined(%0)')")
                << guard();
        }
        d_data.d_test = true;
    }
}

void report::operator()(SourceLocation where, Token const& token)
{
    if (d_analyser.is_component_header(token.getLocation()) &&
        !d_data.d_test) {
        if (IdentifierInfo const* id = token.getIdentifierInfo()) {
            if (id->getNameStart() != guard()) {
                d_analyser.report(token.getLocation(), check_name, "TR14",
                                  "Wrong name for include guard "
                                  "(expected '%0')")
                    << guard();
            }
            d_data.d_test = true;
        }
    }
}

void report::operator()(Token const& token, MacroDirective const*)
{
    if (d_analyser.is_component_header(token.getLocation())
        && !d_data.d_define
        && token.getIdentifierInfo()
        && token.getIdentifierInfo()->getNameStart() == guard()
        ) {
        d_data.d_define = true;
    }
}

void report::operator()(SourceLocation location,
                        std::string const&,
                        std::string const& filename)
{
    if (d_analyser.is_component_header(filename) &&
        !(d_data.d_test && d_data.d_define)) {
        d_analyser.report(location, check_name, "TR14",
                           d_data.d_test
                           ? "Missing define for include guard %0"
                           : d_data.d_define
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
