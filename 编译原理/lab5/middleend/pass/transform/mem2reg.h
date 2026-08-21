#ifndef __MIDDLEEND_PASS_TRANSFORM_MEM2REG_H__
#define __MIDDLEEND_PASS_TRANSFORM_MEM2REG_H__

#include <middleend/pass.h>
#include <middleend/module/ir_function.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/pass/analysis/dominfo.h>
#include <map>
#include <stack>
#include <set>

namespace ME
{
    /**
     * @brief Mem2Reg 优化 Pass
     * 
     * 将内存操作（alloca/load/store）提升为寄存器操作，构建 SSA 形式。
     * 包括：
     * 1. 简单情况：无用的 alloca 消除。
     * 2. 完整情况：基于支配树和支配边界插入 Phi 指令并重命名变量。
     */
    class Mem2RegPass : public FunctionPass
    {
      public:
        virtual void runOnFunction(Function& function) override;

      private:
        Analysis::DomInfo* domInfo;
        
        // 辅助结构，用于存储 alloca 的相关信息
        struct AllocaInfo {
            AllocaInst* allocaInst;
            std::vector<StoreInst*> definingBlocks; // 对该 alloca 进行 store 的指令
            std::vector<LoadInst*> usingBlocks;     // 从该 alloca 进行 load 的指令
            bool is_promotable;
        };

        std::map<AllocaInst*, AllocaInfo> allocaInfos;
        std::map<Operand*, Operand*> valueReplacement; // 旧的操作数（load结果）-> 新的操作数（寄存器值）

        // 分析函数中的 alloca 指令
        void analyzeAllocas(Function& function);
        // 检查 alloca 是否可以被提升（例如非数组，地址未逃逸）
        bool isPromotable(AllocaInst* allocaInst, Function& function);
        
        // 运行简单 Mem2Reg：消除无用 alloca
        void runSimpleMem2Reg(Function& function);
        // 运行完整 Mem2Reg：Phi 插入与变量重命名
        void runCompleteMem2Reg(Function& function);
        
        // 辅助函数：替换指令中的操作数使用
        void replaceUsesInInst(Instruction* inst);
    };
}  // namespace ME

#endif // __MIDDLEEND_PASS_TRANSFORM_MEM2REG_H__