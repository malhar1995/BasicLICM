#include "llvm/Transforms/Scalar.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/PredIteratorCache.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include <algorithm>

using namespace llvm;

namespace{
	struct BasicLICM : public LoopPass {
		static char ID;
		bool changed=false;
	 	BasicLICM() : LoopPass(ID) {}

	 	void getAnalysisUsage(AnalysisUsage &AU) const {
	 		AU.setPreservesCFG();
	 		AU.addRequired<LoopInfoWrapperPass>();
    		AU.addRequired<DominatorTreeWrapperPass>();
    		AU.addRequiredID(LoopSimplifyID);
		}

		// Finds all the basic blocks in preorder which are dominated by the header of the loop "L".
		void preOrder(DomTreeNode *N, std::vector<BasicBlock*> *loopHeaderDominatedBasicBlocks, Loop *L, DominatorTree *DT){
			BasicBlock *BB = N->getBlock();
			if(DT->dominates(L->getHeader(), BB))
				loopHeaderDominatedBasicBlocks->push_back(BB);
			for (DomTreeNode *Child : N->getChildren())
				preOrder(Child, loopHeaderDominatedBasicBlocks, L, DT);
		}

		// Checks if the instruction "I" is loop invariant.
		bool isLoopInvariant(Loop *L, Instruction *I){
			if ((!isa<BinaryOperator>(*I) && !isa<CastInst>(*I) && !isa<SelectInst>(*I) && !isa<GetElementPtrInst>(*I) && !I->isShift()) || !L->hasLoopInvariantOperands(I))
				return false;
			return true;
		}

		// Checks if the instruction "I" is safe to hoist.
		bool safeToHoist(Loop *L,Instruction *I, DominatorTree *DT){
			bool dominatesAllExitBlocks=true;
			SmallVector<BasicBlock*, 8> ExitBlocks;
  			L->getExitBlocks(ExitBlocks);

  			// Checking if the basic block containing the instruction dominates all the exit blocks.
  			for (BasicBlock *ExitBlock : ExitBlocks){
  				if (!DT->dominates(I->getParent(), ExitBlock)){
    				dominatesAllExitBlocks=false;
    				break;
    			}
  			}

  			// isSafeToSpeculativelyExecute(I) returns true if the instruction "I" doesn't have any side effects.
			if(isSafeToSpeculativelyExecute(I) || dominatesAllExitBlocks){
				return true;
			}
			else
				return false;
		}

		// Performs Basic LICM
		void LICM(Loop *L, LoopInfo *LI, DominatorTree *DT, std::vector<BasicBlock*> *loopHeaderDominatedBasicBlocks){
			BasicBlock *PreHeader = L->getLoopPreheader();
			for(BasicBlock *BB:*loopHeaderDominatedBasicBlocks){
				if(LI->getLoopFor(BB) == L){
					for(Instruction &I:*BB){
						if(isLoopInvariant(L, &I) && safeToHoist(L, &I, DT)){
							changed=true;
							I.moveBefore(PreHeader->getTerminator());
						}
					}
				}
			}
		}

	 	bool runOnLoop(Loop *L, LPPassManager &LPM) override {
	 		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
	 		DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
	 		std::vector<BasicBlock*> loopHeaderDominatedBasicBlocks;
	 		preOrder(DT->getRootNode(), &loopHeaderDominatedBasicBlocks, L, DT);
	 		LICM(L, &LI, DT, &loopHeaderDominatedBasicBlocks);
 		return changed;
 		}
 	}; 
} 
char BasicLICM::ID = 'a';
static RegisterPass<BasicLICM> X("basic-licm", "Basic LICM Pass");