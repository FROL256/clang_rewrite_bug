//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <cstdio>
#include <memory>
#include <sstream>
#include <string>
#include <iostream>
#include <unordered_set>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

const clang::SourceManager* g_pSM = nullptr;

std::string GetOriginalText(const clang::SourceRange a_range)
{
  const clang::SourceManager& sm = *g_pSM;
  clang::LangOptions lopt;
  clang::SourceLocation b(a_range.getBegin()), _e(a_range.getEnd());
  clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, lopt));
  return std::string(sm.getCharacterData(b), sm.getCharacterData(e));
}

uint64_t GetHashOfSourceRange(const clang::SourceRange& a_range)
{
  const uint32_t hash1 = a_range.getBegin().getRawEncoding();
  const uint32_t hash2 = a_range.getEnd().getRawEncoding();
  return (uint64_t(hash1) << 32) | uint64_t(hash2);
}

const clang::Expr* RemoveImplicitCast(const clang::Expr* nextNode)
{
  if(nextNode == nullptr)
    return nullptr;
  while(clang::isa<clang::ImplicitCastExpr>(nextNode))
  {
    auto cast = dyn_cast<clang::ImplicitCastExpr>(nextNode);
    nextNode  = cast->getSubExpr();
  }
  return nextNode;
}


class NodesMarker : public RecursiveASTVisitor<NodesMarker> // mark all subsequent nodes to be rewritten, put their ash codes in 'rewrittenNodes'
{
public:
  NodesMarker(std::unordered_set<uint64_t>& a_rewrittenNodes) : m_rewrittenNodes(a_rewrittenNodes){}
  bool VisitStmt(Stmt* expr)
  {
    auto hash = GetHashOfSourceRange(expr->getSourceRange());
    m_rewrittenNodes.insert(hash);
    return true;
  }
private:
  std::unordered_set<uint64_t>& m_rewrittenNodes;
};

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : m_rewriter(R) {}
  
  std::string RecursiveRewrite(const clang::Stmt* expr)
  {
    MyASTVisitor rvCopy = *this;
    rvCopy.TraverseStmt(const_cast<clang::Stmt*>(expr));
    return m_rewriter.getRewrittenText(expr->getSourceRange());
  }

  std::string FunctionCallRewriteNoName(const clang::CXXConstructExpr* call)
  {
    std::string textRes = "(";
    for(unsigned i=0;i<call->getNumArgs();i++)
    {
      textRes += RecursiveRewrite(RemoveImplicitCast(call->getArg(i)));
      if(i < call->getNumArgs()-1)
        textRes += ",";
    }
    return textRes + ")";
  }

  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr* node)
  {
    static std::unordered_map<std::string, std::string> remapOp = {{"+","add"}, {"-","sub"}, {"*","mul"}, {"/","div"}};
  
    auto opKind = node->getOperator();
    const std::string op = clang::getOperatorSpelling(opKind);
    if((op == "+" || op == "-" || op == "*" || op == "/") && WasNotRewrittenYet(node->getSourceRange()))
    {
      const clang::Expr* left  = RemoveImplicitCast(node->getArg(0));
      const clang::Expr* right = RemoveImplicitCast(node->getArg(1));
  
      const std::string leftType  = left->getType().getAsString();
      const std::string rightType = right->getType().getAsString();
      const std::string keyType   = "complex"; 
      
      const std::string leftText  = RecursiveRewrite(left);
      const std::string rightText = RecursiveRewrite(right);
  
      std::string rewrittenOp;
      {
        if(leftType == keyType && rightType == keyType)
          rewrittenOp = keyType + "_" + remapOp[op] + "(" + leftText + "," + rightText + ")";
        else if (leftType == keyType)
          rewrittenOp = keyType + "_" + remapOp[op] + "_real(" + leftText + "," + rightText + ")";
        else if(rightType == keyType)
          rewrittenOp =  "real_" + remapOp[op] + "_" + keyType + "(" + leftText + "," + rightText + ")";
      }
  
      m_rewriter.ReplaceText(node->getSourceRange(), rewrittenOp);
      MarkRewritten(node);
    }
  
    return true;
  }

  bool VisitCXXConstructExpr(CXXConstructExpr* call)
  {
    //call->dump();
    const clang::CXXConstructorDecl* ctorDecl = call->getConstructor();
    const std::string fname = ctorDecl->getNameInfo().getName().getAsString();
    
    if(WasNotRewrittenYet(call->getSourceRange()))
    {
      std::string textRes = "to_complex" + FunctionCallRewriteNoName(call); 
      m_rewriter.ReplaceText(call->getSourceRange(), textRes);
      MarkRewritten(call);
    }
    return true;
  }

private:
  Rewriter& m_rewriter;
  std::unordered_set<uint64_t> m_rewrittenNodes;

  bool WasNotRewrittenYet(const clang::SourceRange a_range) 
  { 
    return (m_rewrittenNodes.find(GetHashOfSourceRange(a_range)) == m_rewrittenNodes.end()); 
  }

  void MarkRewritten(const clang::Stmt* node)
  {
    NodesMarker rv(m_rewrittenNodes); 
    rv.TraverseStmt(const_cast<clang::Stmt*>(node));
  }
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : Visitor(R) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  virtual bool HandleTopLevelDecl(DeclGroupRef DR) override {
    for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
      // Traverse the declaration using our AST visitor.
      Visitor.TraverseDecl(*b);
    return true;
  }

private:
  MyASTVisitor Visitor;
};

int main(int argc, char *argv[]) 
{
  // CompilerInstance will hold the instance of the Clang compiler for us,
  // managing the various objects needed to run the compiler.
  CompilerInstance compiler;
  compiler.createDiagnostics();

  LangOptions &lo = compiler.getLangOpts();
  lo.CPlusPlus = 1;

  // Initialize target info with the default triple for our platform.
  auto TO = std::make_shared<TargetOptions>();
  TO->Triple = llvm::sys::getDefaultTargetTriple();
  TargetInfo *TI = TargetInfo::CreateTargetInfo(compiler.getDiagnostics(), TO);
  compiler.setTarget(TI);
  
  {
    compiler.getLangOpts().GNUMode = 1;
    compiler.getLangOpts().CXXExceptions = 1;
    compiler.getLangOpts().RTTI        = 1;
    compiler.getLangOpts().Bool        = 1;
    compiler.getLangOpts().CPlusPlus   = 1;
    compiler.getLangOpts().CPlusPlus11 = 1;
    compiler.getLangOpts().CPlusPlus14 = 1;
    compiler.getLangOpts().CPlusPlus17 = 1;
  }

  compiler.createFileManager();
  FileManager& FileMgr = compiler.getFileManager();
  compiler.createSourceManager(FileMgr);
  SourceManager& SourceMgr = compiler.getSourceManager();
  g_pSM = &SourceMgr;

  compiler.createPreprocessor(TU_Complete); //TU_Module
  compiler.createASTContext();

  // A Rewriter helps us manage the code rewriting task.
  Rewriter TheRewriter;
  TheRewriter.setSourceMgr(SourceMgr, compiler.getLangOpts());

  // Set the main file handled by the source manager to the input file.
  auto fileOrErr = FileMgr.getFileRef(argv[1], "-std=c++11");

  if (auto ec = fileOrErr.takeError()) 
  {
    std::cout << "[main.cpp]: can't open file: " << argv[1] << std::endl;
    return 0;
  }

  //const FileEntry* FileIn = *fileOrErr;
  const FileEntryRef FileIn = fileOrErr.get();
  SourceMgr.setMainFileID(SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
  compiler.getDiagnosticClient().BeginSourceFile(compiler.getLangOpts(), &compiler.getPreprocessor());

  // Create an AST consumer instance which is going to get called by
  // ParseAST.
  MyASTConsumer TheConsumer(TheRewriter);

  // Parse the file to AST, registering our consumer as the AST consumer.
  ParseAST(compiler.getPreprocessor(), &TheConsumer,
           compiler.getASTContext());

  // At this point the rewriter's buffer should be full with the rewritten
  // file contents.
  const RewriteBuffer *RewriteBuf = TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
  llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());

  return 0;
}
