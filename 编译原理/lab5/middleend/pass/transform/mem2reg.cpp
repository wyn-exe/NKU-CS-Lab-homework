#include <middleend/pass/transform/mem2reg.h>
#include <middleend/pass/analysis/analysis_manager.h>
#include <middleend/pass/analysis/cfg.h>
#include <middleend/module/ir_block.h>
#include <middleend/module/ir_instruction.h>
#include <middleend/module/ir_operand.h>
#include <algorithm>
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <functional>

namespace ME
{
    void Mem2RegPass::runOnFunction(Function& function)
    {
        domInfo = nullptr;

        // 1. 简单 Mem2Reg (消除未使用的 allocas)
        allocaInfos.clear();
        analyzeAllocas(function);
        runSimpleMem2Reg(function);

        // 简单消除后 IR 已经被修改，旧的分析结果可能失效
        Analysis::AM.invalidate(function);

        // 2. 重新分析，若无可提升的 alloca 直接返回
        allocaInfos.clear();
        analyzeAllocas(function);
        if (allocaInfos.empty())
        {
            Analysis::AM.invalidate(function);
            return;  // 经过简单删除后没有可提升的变量，提前结束
        }

        // 3. 获取支配信息，再做完整 Mem2Reg (插入 Phi 指令 + 变量重命名)
        domInfo = Analysis::AM.get<Analysis::DomInfo>(function);
        runCompleteMem2Reg(function);
        Analysis::AM.invalidate(function);
    }

    void Mem2RegPass::analyzeAllocas(Function& function)
    {
        for (auto& [id, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                if (inst->opcode == Operator::ALLOCA)
                {
                    AllocaInst* alloca = static_cast<AllocaInst*>(inst);
                    if (isPromotable(alloca, function))
                    {
                        AllocaInfo info;
                        info.allocaInst = alloca;
                        info.is_promotable = true;
                        info.definingBlocks.clear();
                        info.usingBlocks.clear();
                        allocaInfos[alloca] = info;
                    }
                }
            }
        }
        
        for (auto& [id, block] : function.blocks)
        {
            for (auto* inst : block->insts)
            {
                if (inst->opcode == Operator::LOAD)
                {
                    LoadInst* load = static_cast<LoadInst*>(inst);
                    if (load->ptr->getType() == OperandType::REG) {
                        size_t ptrReg = load->ptr->getRegNum();
                        for (auto& [alloca, info] : allocaInfos) {
                            if (alloca->res->getRegNum() == ptrReg) {
                                info.usingBlocks.push_back(load);
                                break;
                            }
                        }
                    }
                }
                else if (inst->opcode == Operator::STORE)
                {
                    StoreInst* store = static_cast<StoreInst*>(inst);
                    if (store->ptr->getType() == OperandType::REG) {
                        size_t ptrReg = store->ptr->getRegNum();
                        for (auto& [alloca, info] : allocaInfos) {
                            if (alloca->res->getRegNum() == ptrReg) {
                                info.definingBlocks.push_back(store);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    bool Mem2RegPass::isPromotable(AllocaInst* allocaInst, Function& function)
    {
        if (!allocaInst->dims.empty()) return false;
        
        size_t regNum = allocaInst->res->getRegNum();
        
        for (auto& [id, block] : function.blocks) {
            for (auto* inst : block->insts) {
                if (inst->opcode == Operator::LOAD) {
                    LoadInst* load = static_cast<LoadInst*>(inst);
                    if (load->ptr->getType() == OperandType::REG && load->ptr->getRegNum() == regNum) continue;
                }
                if (inst->opcode == Operator::STORE) {
                    StoreInst* store = static_cast<StoreInst*>(inst);
                    if (store->ptr->getType() == OperandType::REG && store->ptr->getRegNum() == regNum) continue;
                    if (store->val->getType() == OperandType::REG && store->val->getRegNum() == regNum) return false;
                }
                
                auto checkOp = [&](Operand* op) {
                    if (op && op->getType() == OperandType::REG && op->getRegNum() == regNum) return true;
                    return false;
                };
                
                bool bad = false;
                if (auto* i = dynamic_cast<ArithmeticInst*>(inst)) {
                    if (checkOp(i->lhs) || checkOp(i->rhs)) bad = true;
                } else if (auto* i = dynamic_cast<IcmpInst*>(inst)) {
                    if (checkOp(i->lhs) || checkOp(i->rhs)) bad = true;
                } else if (auto* i = dynamic_cast<FcmpInst*>(inst)) {
                    if (checkOp(i->lhs) || checkOp(i->rhs)) bad = true;
                } else if (auto* i = dynamic_cast<BrCondInst*>(inst)) {
                    if (checkOp(i->cond)) bad = true;
                } else if (auto* i = dynamic_cast<CallInst*>(inst)) {
                    for (auto& arg : i->args) if (checkOp(arg.second)) bad = true;
                } else if (auto* i = dynamic_cast<GEPInst*>(inst)) {
                    if (checkOp(i->basePtr)) bad = true;
                    for(auto* idx : i->idxs) if(checkOp(idx)) bad = true;
                } else if (auto* i = dynamic_cast<SI2FPInst*>(inst)) {
                    if (checkOp(i->src)) bad = true;
                } else if (auto* i = dynamic_cast<FP2SIInst*>(inst)) {
                    if (checkOp(i->src)) bad = true;
                } else if (auto* i = dynamic_cast<ZextInst*>(inst)) {
                    if (checkOp(i->src)) bad = true;
                } else if (auto* i = dynamic_cast<RetInst*>(inst)) {
                    if (checkOp(i->res)) bad = true;
                } else if (dynamic_cast<BrUncondInst*>(inst)) {
                    // safe
                } else if (inst->opcode == Operator::ALLOCA) {
                    // safe
                } else if (inst->opcode == Operator::PHI) {
                    PhiInst* phi = static_cast<PhiInst*>(inst);
                    for (auto& pair : phi->incomingVals) if (checkOp(pair.second)) bad = true;
                } else {
                     // bad if any operand uses the regNum?
                }
                
                if (bad) return false;
            }
        }
        return true;
    }

    void Mem2RegPass::runSimpleMem2Reg(Function& function)
    {
        std::set<Instruction*> instsToDelete;
        
        for (auto& [alloca, info] : allocaInfos)
        {
            if (info.usingBlocks.empty())
            {
                instsToDelete.insert(alloca);
                for (auto* store : info.definingBlocks) instsToDelete.insert(store);
            }
        }
        
        for (auto& [id, block] : function.blocks) {
            for (auto it = block->insts.begin(); it != block->insts.end(); ) {
                if (instsToDelete.count(*it)) {
                    it = block->insts.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void Mem2RegPass::runCompleteMem2Reg(Function& function)
    {
        if (domInfo == nullptr) return;
        if (allocaInfos.empty()) return;
        std::map<AllocaInst*, std::set<size_t>> defBlocks;
        std::map<Instruction*, size_t> instBlockMap;
        for(auto& [id, block] : function.blocks) {
            for(auto* inst : block->insts) instBlockMap[inst] = id;
        }

        for (auto& [alloca, info] : allocaInfos) {
            for(auto* store : info.definingBlocks) {
                if(instBlockMap.count(store))
                    defBlocks[alloca].insert(instBlockMap[store]);
            }
        }
        
        std::map<AllocaInst*, std::set<size_t>> phiBlocks;
        
        for (auto& [alloca, defs] : defBlocks) {
            std::vector<size_t> worklist(defs.begin(), defs.end());
            std::set<size_t> inWorklist(defs.begin(), defs.end());
            
            size_t i = 0;
            while(i < worklist.size()) {
                size_t b_id = worklist[i++];
                if (b_id >= domInfo->getDomFrontier().size()) continue;
                
                for(int d_idx : domInfo->getDomFrontier()[b_id]) {
                    size_t d = (size_t)d_idx;
                    if (phiBlocks[alloca].find(d) == phiBlocks[alloca].end()) {
                        phiBlocks[alloca].insert(d);
                        if (inWorklist.find(d) == inWorklist.end()) {
                            worklist.push_back(d);
                            inWorklist.insert(d);
                        }
                    }
                }
            }
        }
        
        // Calculate maxReg properly
        size_t maxReg = 0;
        auto scanReg = [&](Operand* op) {
            if (op && op->getType() == OperandType::REG) {
                maxReg = std::max(maxReg, op->getRegNum());
            }
        };
        for(auto& [id, block] : function.blocks) {
            for(auto* inst : block->insts) {
                if (auto* i = dynamic_cast<ArithmeticInst*>(inst)) {
                    scanReg(i->lhs); scanReg(i->rhs); scanReg(i->res);
                } else if (auto* i = dynamic_cast<LoadInst*>(inst)) {
                    scanReg(i->ptr); scanReg(i->res);
                } else if (auto* i = dynamic_cast<StoreInst*>(inst)) {
                    scanReg(i->val); scanReg(i->ptr);
                } else if (auto* i = dynamic_cast<IcmpInst*>(inst)) {
                    scanReg(i->lhs); scanReg(i->rhs); scanReg(i->res);
                } else if (auto* i = dynamic_cast<FcmpInst*>(inst)) {
                    scanReg(i->lhs); scanReg(i->rhs); scanReg(i->res);
                } else if (auto* i = dynamic_cast<BrCondInst*>(inst)) {
                    scanReg(i->cond);
                } else if (auto* i = dynamic_cast<CallInst*>(inst)) {
                    for(auto& arg : i->args) scanReg(arg.second);
                    scanReg(i->res);
                } else if (auto* i = dynamic_cast<RetInst*>(inst)) {
                    scanReg(i->res);
                } else if (auto* i = dynamic_cast<GEPInst*>(inst)) {
                    scanReg(i->basePtr); scanReg(i->res);
                    for(auto* idx : i->idxs) scanReg(idx);
                } else if (auto* i = dynamic_cast<SI2FPInst*>(inst)) {
                    scanReg(i->src); scanReg(i->dest);
                } else if (auto* i = dynamic_cast<FP2SIInst*>(inst)) {
                    scanReg(i->src); scanReg(i->dest);
                } else if (auto* i = dynamic_cast<ZextInst*>(inst)) {
                    scanReg(i->src); scanReg(i->dest);
                } else if (auto* i = dynamic_cast<PhiInst*>(inst)) {
                    scanReg(i->res);
                    for(auto& pair : i->incomingVals) scanReg(pair.second);
                } else if (auto* i = dynamic_cast<AllocaInst*>(inst)) {
                    scanReg(i->res);
                }
            }
        }
         
        std::map<AllocaInst*, std::map<size_t, PhiInst*>> insertedPhis;
         
        for(auto& [alloca, blocks] : phiBlocks) {
            for(size_t bid : blocks) {
                Operand* newReg = OperandFactory::getInstance().getRegOperand(++maxReg);
                PhiInst* phi = new PhiInst(alloca->dt, newReg);
                if (function.blocks.count(bid)) {
                    function.blocks[bid]->insertFront(phi);
                    insertedPhis[alloca][bid] = phi;
                }
            }
        }
         
        std::map<AllocaInst*, std::vector<Operand*>> stacks;
        std::map<AllocaInst*, Operand*>              defaultVals;
        for(auto& [alloca, info] : allocaInfos) {
            Operand* undef;
            if (alloca->dt == DataType::F32)
                undef = OperandFactory::getInstance().getImmeF32Operand(0.0f);
            else if (alloca->dt == DataType::PTR) {
                 // Undefined pointer. Since we don't have null, use 0 but warn?
                 // SysY arrays are global or on stack. 0 might be okay as placeholder.
                 undef = OperandFactory::getInstance().getImmeI32Operand(0);
            }
            else
                undef = OperandFactory::getInstance().getImmeI32Operand(0);
            stacks[alloca].push_back(undef);
            defaultVals[alloca] = undef;
        }
         
        valueReplacement.clear();
        std::set<size_t> visited;
         
        std::function<void(size_t)> rename = [&](size_t blockId) {
            visited.insert(blockId);
            auto blockIt = function.blocks.find(blockId);
            if (blockIt == function.blocks.end() || blockIt->second == nullptr) return;
            Block* block = blockIt->second;
             
            std::map<AllocaInst*, size_t> stackSizes;
            for(auto& [alloca, stack] : stacks) stackSizes[alloca] = stack.size();
             
            for(auto* inst : block->insts) {
                replaceUsesInInst(inst);

                if (inst->opcode == Operator::PHI) {
                    PhiInst* phi = static_cast<PhiInst*>(inst);
                    for(auto& [alloca, blockMap] : insertedPhis) {
                        if (blockMap.count(blockId) && blockMap[blockId] == phi) {
                            if (stacks[alloca].empty()) stacks[alloca].push_back(defaultVals[alloca]);
                            stacks[alloca].push_back(phi->res);
                            break;
                        }
                    }
                }
                else if (inst->opcode == Operator::LOAD) {
                    LoadInst* load = static_cast<LoadInst*>(inst);
                    if (load->ptr->getType() == OperandType::REG) {
                        size_t ptrReg = load->ptr->getRegNum();
                        for(auto& [alloca, info] : allocaInfos) {
                            if (alloca->res->getRegNum() == ptrReg) {
                                if (stacks[alloca].empty()) stacks[alloca].push_back(defaultVals[alloca]);
                                valueReplacement[load->res] = stacks[alloca].back();
                                break;
                            }
                        }
                    }
                }
                else if (inst->opcode == Operator::STORE) {
                    StoreInst* store = static_cast<StoreInst*>(inst);
                    if (store->ptr->getType() == OperandType::REG) {
                        size_t ptrReg = store->ptr->getRegNum();
                        for(auto& [alloca, info] : allocaInfos) {
                            if (alloca->res->getRegNum() == ptrReg) {
                                if (stacks[alloca].empty()) stacks[alloca].push_back(defaultVals[alloca]);
                                stacks[alloca].push_back(store->val);
                                break;
                            }
                        }
                    }
                }
            }
             
            auto* cfg = Analysis::AM.get<Analysis::CFG>(function);
            if (blockId < cfg->G_id.size()) {
                for (size_t succId : cfg->G_id[blockId]) {
                    for(auto& [alloca, blockMap] : insertedPhis) {
                        if (blockMap.count(succId)) {
                            PhiInst* phi = blockMap[succId];
                            if (stacks[alloca].empty()) stacks[alloca].push_back(defaultVals[alloca]);
                            Operand* val = stacks[alloca].back();
                            phi->addIncoming(val, OperandFactory::getInstance().getLabelOperand(blockId));
                        }
                    }
                }
            }

            if (blockId < domInfo->getDomTree().size()) {
                for (int child : domInfo->getDomTree()[blockId]) {
                    if (visited.find(child) == visited.end())
                    rename((size_t)child);
                }
            }
             
            for(auto& [alloca, size] : stackSizes) {
                while(stacks[alloca].size() > size) stacks[alloca].pop_back();
            }
        };
         
        if (function.blocks.count(0))
            rename(0);
         
        std::set<Instruction*> instsToDelete;
        for(auto& [alloca, info] : allocaInfos) {
            instsToDelete.insert(alloca);
            for(auto* store : info.definingBlocks) instsToDelete.insert(store);
            for(auto* load : info.usingBlocks) instsToDelete.insert(load);
        }
         
        for (auto& [id, block] : function.blocks) {
            for (auto it = block->insts.begin(); it != block->insts.end(); ) {
                if (instsToDelete.count(*it)) {
                    it = block->insts.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    void Mem2RegPass::replaceUsesInInst(Instruction* inst) {
        auto replace = [&](Operand*& op) {
            while (op && valueReplacement.count(op)) {
                op = valueReplacement[op];
            }
        };
    
        if (auto* i = dynamic_cast<ArithmeticInst*>(inst)) {
            replace(i->lhs);
            replace(i->rhs);
        } else if (auto* i = dynamic_cast<LoadInst*>(inst)) {
            replace(i->ptr);
        } else if (auto* i = dynamic_cast<StoreInst*>(inst)) {
            replace(i->val);
            replace(i->ptr);
        } else if (auto* i = dynamic_cast<IcmpInst*>(inst)) {
            replace(i->lhs);
            replace(i->rhs);
        } else if (auto* i = dynamic_cast<FcmpInst*>(inst)) {
            replace(i->lhs);
            replace(i->rhs);
        } else if (auto* i = dynamic_cast<BrCondInst*>(inst)) {
            replace(i->cond);
        } else if (auto* i = dynamic_cast<CallInst*>(inst)) {
            for (auto& arg : i->args) replace(arg.second);
        } else if (auto* i = dynamic_cast<RetInst*>(inst)) {
            if(i->res) replace(i->res);
        } else if (auto* i = dynamic_cast<GEPInst*>(inst)) {
            replace(i->basePtr);
            for (auto& idx : i->idxs) replace(idx);
        } else if (auto* i = dynamic_cast<SI2FPInst*>(inst)) {
            replace(i->src);
        } else if (auto* i = dynamic_cast<FP2SIInst*>(inst)) {
            replace(i->src);
        } else if (auto* i = dynamic_cast<ZextInst*>(inst)) {
            replace(i->src);
        } else if (auto* i = dynamic_cast<PhiInst*>(inst)) {
             for(auto& pair : i->incomingVals) {
                 replace(pair.second);
             }
        }
    }
}
