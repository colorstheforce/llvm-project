#include "ClangTidy.h"
#include "ClangTidyTest.h"
#include "gtest/gtest.h"

namespace clang {
namespace tidy {
namespace test {

namespace {
class TestCheck : public ClangTidyCheck {
public:
  TestCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {
    diag("DiagWithNoLoc");
  }
  void registerMatchers(ast_matchers::MatchFinder *Finder) override {
    Finder->addMatcher(ast_matchers::varDecl().bind("var"), this);
  }
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override {
    const auto *Var = Result.Nodes.getNodeAs<VarDecl>("var");
    // Add diagnostics in the wrong order.
    diag(Var->getLocation(), "variable");
    diag(Var->getTypeSpecStartLoc(), "type specifier");
  }
};

class HighlightTestCheck : public ClangTidyCheck {
public:
  HighlightTestCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override {
    Finder->addMatcher(ast_matchers::varDecl().bind("var"), this);
  }
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override {
    const auto *Var = Result.Nodes.getNodeAs<VarDecl>("var");
    diag(Var->getLocation(), "highlight range") << Var->getSourceRange();
  }
};

} // namespace

TEST(ClangTidyDiagnosticConsumer, SortsErrors) {
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<TestCheck>("int a;", &Errors);
  EXPECT_EQ(3ul, Errors.size());
  EXPECT_EQ("DiagWithNoLoc", Errors[0].Message.Message);
  EXPECT_EQ("type specifier", Errors[1].Message.Message);
  EXPECT_EQ("variable", Errors[2].Message.Message);
}

TEST(ClangTidyDiagnosticConsumer, HandlesSourceRangeHighlight) {
  std::vector<ClangTidyError> Errors;
  runCheckOnCode<HighlightTestCheck>("int abc;", &Errors);
  EXPECT_EQ(1ul, Errors.size());
  EXPECT_EQ("highlight range", Errors[0].Message.Message);

  // int abc;
  // ____^
  // 01234
  EXPECT_EQ(4, Errors[0].Message.FileOffset);

  // int abc
  // ~~~~~~~   -> Length 7. (0-length highlights are nonsensical.)
  EXPECT_EQ(1, Errors[0].Message.Ranges.size());
  EXPECT_EQ(0, Errors[0].Message.Ranges[0].FileOffset);
  EXPECT_EQ(7, Errors[0].Message.Ranges[0].Length);
}

} // namespace test
} // namespace tidy
} // namespace clang
