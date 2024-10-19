set(LLVM_LINK_COMPONENTS
  FrontendOpenMP
  Support
)
             
add_clang_executable(rtest 
                     main.cpp)
             
clang_target_link_libraries(rtest
  PRIVATE
  clangAST
  clangASTMatchers
  clangBasic
  clangFrontend
  clangTooling
  clangToolingRefactoring
  )