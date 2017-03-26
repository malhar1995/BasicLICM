References
1. $LLVM_SRC/lib/Transforms/Scalar/LICM.cpp
2. http://llvm.org/docs/doxygen/html/classllvm_1_1Instruction.html
3. http://llvm.org/docs/doxygen/html/classllvm_1_1Loop.html

Algorithm:

LICM(Loop L){
  for(each basic block BB dominated by loop header in preorder on dominator tree){
    if(BB is immediately within L){
      for(each instruction I in BB){
        if(isLoopInvariant(I) && safeToHoist(I))
          move I to preheader basic block;
      }
    }
  }
}
Detailed algorithm can be found in the attached PDF.
