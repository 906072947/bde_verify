// csabbg_midreturn.cpp                                               -*-C++-*-
// ----------------------------------------------------------------------------

#include <csabase_analyser.h>
#include <csabase_debug.h>
#include <csabase_location.h>
#include <csabase_ppobserver.h>
#include <csabase_registercheck.h>
#include <csabase_util.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>
#include <string>
#include <sstream>

#ident "$Id$"

// ----------------------------------------------------------------------------

static std::string const check_name("mid-return");

// ----------------------------------------------------------------------------

using namespace clang;
using namespace clang::ast_matchers;
using namespace bde_verify::csabase;

namespace
{

// Data attached to analyzer for this check.
struct data
{
    std::set<const Stmt*>    d_last_returns;  // Last top-level 'return'
    std::set<const Stmt*>    d_all_returns;   // All 'return'
    std::set<SourceLocation> d_rcs;           // Suppression comments
};

// Callback object for inspecting comments.
struct comments
{
    Analyser& d_analyser;

    comments(Analyser& analyser) : d_analyser(analyser) {}

    void operator()(SourceRange range)
    {
        Location location(d_analyser.get_location(range.getBegin()));
        if (d_analyser.is_component(location.file())) {
            std::string comment(d_analyser.get_source(range));
            size_t rpos = comment.rfind("// RETURN");
            if (rpos != comment.npos) {
                d_analyser.attachment<data>().d_rcs.insert(
                    range.getBegin().getLocWithOffset(rpos));
            }
        }
    }
};


const internal::DynTypedMatcher &
return_matcher()
    // Return an AST matcher which looks for return statements.
{
    static const internal::DynTypedMatcher matcher =
        decl(forEachDescendant(returnStmt().bind("return")));
    return matcher;
}

// Callback object invoked upon completion.
struct report
{
    Analyser& d_analyser;
    data& d_data;

    report(Analyser& analyser)
        : d_analyser(analyser)
        , d_data(analyser.attachment<data>()) { }

    // Function for searching for final return statements.
    const ReturnStmt* last_return(ConstStmtRange s)
    {
        const ReturnStmt* ret = 0;
        for (; s; ++s) {
            if (llvm::dyn_cast<CompoundStmt>(*s)) {
                // Recurse into simple compound statements.
                ret = last_return((*s)->children());
            } else {
                // Try to cast each statement to a ReturnStmt. Therefore 'ret'
                // will only be non-zero if the final statement is a 'return'.
                ret = llvm::dyn_cast<ReturnStmt>(*s);
            }
        }
        return ret;
    }

    void match_return(const BoundNodes& nodes)
    {
        const ReturnStmt *ret = nodes.getNodeAs<ReturnStmt>("return");

        if (!d_analyser.is_component(ret->getLocStart())) {
            return;                                                   // RETURN
        }

        // If the statement is contained in a function template specialization
        // (even nested within local classes) ignore it - the original in the
        // template will be processed.
        const FunctionDecl* func = d_analyser.get_parent<FunctionDecl>(ret);
        d_data.d_last_returns.insert(last_return(func->getBody()->children()));
        do {
            if (func->isTemplateInstantiation()) {
                return;                                               // RETURN
            }
        } while (0 != (func = d_analyser.get_parent<FunctionDecl>(func)));
        d_data.d_all_returns.insert(ret);
    }

    void operator()()
    {
        MatchFinder mf;
        OnMatch<report, &report::match_return> m1(this);
        mf.addDynamicMatcher(return_matcher(), &m1);
        mf.match(*d_analyser.context()->getTranslationUnitDecl(),
                 *d_analyser.context());

        process_all_returns(
            d_data.d_all_returns.begin(), d_data.d_all_returns.end());
    }

    void process_all_returns(std::set<const Stmt*>::iterator begin,
                             std::set<const Stmt*>::iterator end)
    {
        const data& d = d_analyser.attachment<data>();
        for (std::set<const Stmt*>::iterator it = begin; it != end; ++it) {
            // Ignore final top-level return statements.
            if (!d.d_last_returns.count(*it) &&
                !is_commented(*it, d.d_rcs.begin(), d.d_rcs.end())) {
                d_analyser.report(*it, check_name, "MR01",
                        "Mid-function 'return' requires '// RETURN' comment");
                SourceRange line_range =
                    d_analyser.get_line_range((*it)->getLocEnd());
                if (line_range.isValid()) {
                    llvm::StringRef line = d_analyser.get_source(line_range);
                    std::string tag = (line.size() < 70
                                       ? std::string(70 - line.size(), ' ')
                                       : "\n" + std::string(70, ' ')
                                      ) + "// RETURN";
                    d_analyser.report(*it, check_name, "MR01",
                                      "Correct text is\n%0",
                                      false, clang::DiagnosticsEngine::Note)
                        << line.str() + tag;
                    d_analyser.rewriter().InsertTextAfter(
                        line_range.getEnd(), tag);
                }
            }
        }
    }

    // Determine if a statement has a proper '// RETURN' comment.
    bool is_commented(const Stmt* stmt,
                      std::set<SourceLocation>::iterator comments_begin,
                      std::set<SourceLocation>::iterator comments_end)
    {
        SourceManager& m = d_analyser.manager();
        SourceLocation loc = m.getFileLoc(stmt->getLocEnd());
        unsigned       sline = m.getPresumedLineNumber(loc);
        unsigned       scolm = m.getPresumedColumnNumber(loc);
        FileID  sfile = m.getFileID(loc);

        for (std::set<SourceLocation>::iterator it = comments_begin;
             it != comments_end;
             ++it) {
            unsigned      cline = m.getPresumedLineNumber(*it);
            unsigned      ccolm = m.getPresumedColumnNumber(*it);
            FileID cfile = m.getFileID(*it);

            if (   (cline == sline || (scolm >= 69 && cline == sline + 1))
                && cfile == sfile) {
                if (ccolm != 71) {
                    std::ostringstream ss;
                    ss << "'// RETURN' comment must end in column 79, "
                       << "not " << (ccolm + 8);
                    if (scolm >= 69 && ccolm > 71) {
                        ss << " (place it alone on the next line)";
                    }
                    d_analyser.report(*it, check_name, "MR02", ss.str());
                    SourceRange line_range = d_analyser.get_line_range(*it);
                    llvm::StringRef line = d_analyser.get_source(line_range)
                                               .slice(0, ccolm - 1)
                                               .rtrim();
                    std::string tag = (line.size() < 70
                                       ? std::string(70 - line.size(), ' ')
                                       : "\n" + std::string(70, ' ')
                                      ) + "// RETURN";
                    d_analyser.report(*it, check_name, "MR01",
                            "Correct text is\n%0",
                            false, clang::DiagnosticsEngine::Note)
                        << line.str() + tag;
                    line_range.setBegin(
                        line_range.getBegin().getLocWithOffset(line.size()));
                    d_analyser.rewriter().ReplaceText(line_range, tag);
                }
                return true;
            }
        }
        return false;
    }
};

void subscribe(Analyser& analyser, Visitor&, PPObserver& observer)
{
    analyser.onTranslationUnitDone += report(analyser);
    observer.onComment += comments(analyser);
}

}  // close anonymous namespace

// ----------------------------------------------------------------------------

static RegisterCheck c3(check_name, &subscribe);
