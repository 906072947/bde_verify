// csafmt_banner.cpp                                                  -*-C++-*-
// ----------------------------------------------------------------------------

#include <csabase_analyser.h>
#include <csabase_location.h>
#include <csabase_ppobserver.h>
#include <csabase_registercheck.h>
#include <csabase_util.h>
#include <llvm/Support/Regex.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <set>
#include <vector>

#ident "$Id$"

// ----------------------------------------------------------------------------

static std::string const check_name("banner");

// ----------------------------------------------------------------------------

using clang::FileID;
using clang::FixItHint;
using clang::SourceLocation;
using clang::SourceManager;
using clang::SourceRange;
using csabase::Analyser;
using csabase::Location;
using csabase::PPObserver;
using csabase::Range;
using csabase::Visitor;

namespace
{

struct comments
    // Data holding seen comments.
{
    typedef std::vector<SourceRange> Ranges;
    typedef std::map<std::string, Ranges> Comments;
    Comments d_comments;

    void append(Analyser& analyser, SourceRange range);
};

void comments::append(Analyser& analyser, SourceRange range)
{
    SourceManager& m = analyser.manager();
    comments::Ranges& c = d_comments[m.getFilename(range.getBegin())];
    if (c.size() != 0 && csabase::areConsecutive(m, c.back(), range)) {
        c.back().setEnd(range.getEnd());
    } else {
        c.push_back(range);
    }
}

struct files
    // Callback object for inspecting files.
{
    Analyser& d_analyser;                   // Analyser object.

    files(Analyser& analyser);
        // Create a 'files' object, accessing the specified 'analyser'.

    void operator()(SourceRange range);
        // The specified comment 'range' is added to the stored data.

    void check_comment(SourceRange comment_range);
        // Check banner in one comment.

    void check_file_comments(comments::Ranges& comments);
        // Check all banners in a file.

    void operator()();
        // Report improper banners.
};

files::files(Analyser& analyser)
: d_analyser(analyser)
{
}

void files::operator()(SourceRange range)
{
    d_analyser.attachment<comments>().append(d_analyser, range);
}

#undef  aba
#define aba(a, b) "(" a "(" b a ")*)"
#undef  SP
#define SP "[[:space:]]*"

static llvm::Regex generic_banner(      // things that look like banners
        "//"     SP     aba("[-=_]",        SP) SP          // 1, 2
        "//" "(" SP ")" aba("[_[:alnum:]]", SP) SP          // 3, 4, 5
        "//" "(" SP ")" aba("[-=_]",        SP) SP "$",     // 6, 7, 8
    llvm::Regex::Newline);

static llvm::Regex generic_separator(   // things that look like separators
        "(//.*[[:alnum:]].*)?\n?"
        "((//([[:space:]]*)[-=_](([[:space:]][-=_])*|[-=_]*))[[:space:]]*)$",
    llvm::Regex::Newline);

#undef SP
#undef aba

void files::check_comment(SourceRange comment_range)
{
    SourceManager& manager = d_analyser.manager();
    llvm::SmallVector<llvm::StringRef, 8> matches;

    llvm::StringRef comment = d_analyser.get_source(comment_range, true);

    size_t offset = 0;
    for (llvm::StringRef suffix = comment;
         generic_separator.match(suffix, &matches);
         suffix = comment.drop_front(offset)) {
        llvm::StringRef separator = matches[2];
        size_t separator_pos = offset + suffix.find(separator);
        offset = separator_pos + separator.size();
        SourceLocation separator_start =
            comment_range.getBegin().getLocWithOffset(separator_pos);
        if (manager.getPresumedColumnNumber(separator_start) == 1 &&
            matches[3].size() != 79 &&
            matches[4].size() <= 1 &&
            separator.size() != matches[1].size()) {
            std::string expected_banner = matches[3].trim();
            std::string extra =
                expected_banner.substr(expected_banner.size() - 2);
            while (expected_banner.size() < 79) {
                expected_banner += extra;
            }
            expected_banner = expected_banner.substr(0, 79);
            d_analyser.report(separator_start,
                              check_name, "BAN02",
                              "Banner ends at column %0 instead of 79")
                << static_cast<int>(matches[3].size());
            SourceRange line_range = d_analyser.get_line_range(separator_start);
            if (line_range.isValid()) {
                d_analyser.rewriter().ReplaceText(line_range, expected_banner);
            }
        }
    }

    offset = 0;
    for (llvm::StringRef suffix = comment; 
         generic_banner.match(suffix, &matches);
         suffix = comment.drop_front(offset)) {
        llvm::StringRef banner = matches[0];
        size_t banner_pos = offset + suffix.find(banner);
        offset = banner_pos + banner.rfind('\n');
        SourceLocation banner_start =
            comment_range.getBegin().getLocWithOffset(banner_pos);
        if (manager.getPresumedColumnNumber(banner_start) != 1) {
            continue;
        }
        llvm::StringRef text = matches[4];
        size_t text_pos = banner.find(text);
        size_t actual_last_space_pos =
            manager.getPresumedColumnNumber(
                    banner_start.getLocWithOffset(text_pos)) - 1;
        size_t banner_slack = std::strtoul(
            d_analyser.config()->value("banner_slack", banner_start).c_str(),
            0, 10);
        size_t expected_last_space_pos =
            ((79 - 2 - text.size()) / 2 + 2) & ~3;
        if (actual_last_space_pos < expected_last_space_pos - banner_slack ||
            actual_last_space_pos > expected_last_space_pos + banner_slack) {
            std::string expected_text =
                "//" + std::string(expected_last_space_pos - 2, ' ') +
                text.str();
            const char* error = (actual_last_space_pos & 3) ?
                "Improperly centered banner text"
                " (not reachable using tab key)" :
                "Improperly centered banner text";
            SourceLocation sl = banner_start.getLocWithOffset(text_pos);
            d_analyser.report(sl, check_name, "BAN03", error);
            d_analyser.report(sl, check_name, "BAN03",
                              "Correct text is\n%0",
                              false, clang::DiagnosticsEngine::Note)
                << expected_text;
            SourceRange line_range = d_analyser.get_line_range(sl);
            if (line_range.isValid()) {
                d_analyser.rewriter().ReplaceText(line_range, expected_text);
            }
        }

        llvm::StringRef bottom_rule = matches[7];
        if (banner.size() - banner.rfind('\n') != 80 &&
            text.size() == bottom_rule.size() &&
            matches[3] != matches[6]) {
            // It's a misaligned underline for the banner text.
            SourceLocation bottom_loc = banner_start.getLocWithOffset(
                banner.size() - bottom_rule.size());
            std::string expected_text =
                "//" + std::string(expected_last_space_pos - 2, ' ') +
                bottom_rule.str();
            d_analyser.report(bottom_loc,
                              check_name, "BAN04",
                              "Improperly centered underlining");
            d_analyser.report(bottom_loc,
                              check_name, "BAN04",
                              "Correct version is\n%0",
                              false, clang::DiagnosticsEngine::Note)
                << "//" + std::string(expected_last_space_pos - 2, ' ') +
                       bottom_rule.str();
            SourceRange line_range = d_analyser.get_line_range(bottom_loc);
            if (line_range.isValid()) {
                d_analyser.rewriter().ReplaceText(line_range, expected_text);
            }
        }
    }
}

void files::check_file_comments(comments::Ranges& comments)
{
    comments::Ranges::iterator b = comments.begin();
    comments::Ranges::iterator e = comments.end();
    for (comments::Ranges::iterator itr = b; itr != e; ++itr) {
        check_comment(*itr);
    }
}

void files::operator()()
{
    comments::Comments& file_comments =
        d_analyser.attachment<comments>().d_comments;
    comments::Comments::iterator b = file_comments.begin();
    comments::Comments::iterator e = file_comments.end();

    for (comments::Comments::iterator itr = b; itr != e; ++itr) {
        if (d_analyser.is_component(itr->first)) {
            check_file_comments(itr->second);
        }
    }
}

void subscribe(Analyser& analyser, Visitor&, PPObserver& observer)
    // Hook up the callback functions.
{
    analyser.onTranslationUnitDone += files(analyser);
    observer.onComment             += files(analyser);
}

}  // close anonymous namespace

// ----------------------------------------------------------------------------

static csabase::RegisterCheck c1(check_name, &subscribe);
