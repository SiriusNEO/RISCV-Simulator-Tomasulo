//
// Created by SiriusNEO on 2021/7/6.
//

#include "cpu_core.hpp"

namespace RISC_V {

    void CPU::instructionQueue() {
        if (COM_PUB.valid && COM_PUB.toPUB_jumpFlag) IQ.clear();
        IQNode newIQNode;
        if (COM_PUB.valid && COM_PUB.toPUB_jumpFlag && COM_PUB.toPUB_tarpc)
            pc = COM_PUB.toPUB_tarpc;
        newIQNode.pc = pc;
        newIQNode.insCode = mem.read(pc, 4);
        assert(!IQ.full()); //will it full?
        IQ.enque(newIQNode);
        pc += 4;

#ifdef DEBUG
        std::cout << '\n' << "* IQ *" << '\n';
        std::cout << "[Input] " << pc-4 << '\n';
        std::cout << "[Output] " << std::hex << newIQNode.insCode << std::dec << '\n';
#endif
    }

    void CPU::issue() { //to ROB, to RS, to SLB
        IS_RF.clear(); IS_RS.clear(); IS_SLB.clear(); IS_ROB.clear();
        if (ROB_prev.full() || RS_prev.full() || SLB_prev.full()) return; //full, wait
        auto newIQ = IQ.deque();
        Instruction inst;
        id.decode(newIQ.insCode, inst);

        //ROB
        IS_ROB.valid = true;
        IS_ROB.toROB = (ROBNode){false, false, false, inst, 0, newIQ.pc, ROB_prev.tail};

        //SLBuffer
        if (isMemoryAccess(inst.ins)) {
            IS_SLB.valid = true;
            IS_SLB.toSLB = (SLBNode){true, inst, 0, 0, -1, -1, -1, ROB_prev.tail, newIQ.pc};
            if (RF_prev.regs[inst.rs1].Q >= 0) {
                if (ROB_prev.que[RF_prev.regs[inst.rs1].Q].ready)
                    IS_SLB.toSLB.V1 = ROB_prev.que[RF_prev.regs[inst.rs1].Q].dat;
                else IS_SLB.toSLB.Q1 = RF_prev.regs[inst.rs1].Q;
            }
            else IS_SLB.toSLB.V1 = RF_prev.regs[inst.rs1].V;

            if (RF_prev.regs[inst.rs2].Q >= 0) {
                if (ROB_prev.que[RF_prev.regs[inst.rs2].Q].ready)
                    IS_SLB.toSLB.V2 = ROB_prev.que[RF_prev.regs[inst.rs2].Q].dat;
                else IS_SLB.toSLB.Q2 = RF_prev.regs[inst.rs2].Q;
            }
            else IS_SLB.toSLB.V2 = RF_prev.regs[inst.rs2].V; //upd

            IS_RF.valid = true;
            IS_RF.toRF = (RFNode){IS_SLB.toSLB.ROB_id, 0}; //V is useless in IS_RF
            IS_RF.toRF_rd = inst.rd;
        }
        else {//RS
            IS_RS.valid = true;
            IS_RS.toRS = (RSNode){true, inst, 0, 0, -1, -1, ROB_prev.tail, newIQ.pc};
            if (RF_prev.regs[inst.rs1].Q >= 0) {
                if (ROB_prev.que[RF_prev.regs[inst.rs1].Q].ready)
                    IS_RS.toRS.V1 = ROB_prev.que[RF_prev.regs[inst.rs1].Q].dat;
                else IS_RS.toRS.Q1 = RF_prev.regs[inst.rs1].Q;
            }
            else IS_RS.toRS.V1 = RF_prev.regs[inst.rs1].V;

            if (RF_prev.regs[inst.rs2].Q >= 0) {
                if (ROB_prev.que[RF_prev.regs[inst.rs2].Q].ready)
                    IS_RS.toRS.V2 = ROB_prev.que[RF_prev.regs[inst.rs2].Q].dat;
                else IS_RS.toRS.Q2 = RF_prev.regs[inst.rs2].Q;
            }
            else IS_RS.toRS.V2 = RF_prev.regs[inst.rs2].V;

            IS_RF.valid = true;
            IS_RF.toRF = (RFNode){IS_RS.toRS.ROB_id, 0}; //V is useless in IS_RF
            IS_RF.toRF_rd = inst.rd;
        }
#ifdef DEBUG
        std::cout << '\n' << "* IS *" << '\n';
        std::cout << "[Input] " << std::hex << newIQ.insCode << std::dec << '\n';
        std::cout << "[Output] " << insName[inst.ins] << " rd:" << inst.rd << " rs1:" << inst.rs1 << " rs2:"
        << inst.rs2 << " imm:" << inst.imm << '\n';
        IS_RF.debug(IS_RF_Type);
        IS_RS.debug(IS_RS_Type);
        IS_ROB.debug(IS_ROB_Type);
        IS_SLB.debug(IS_SLB_Type);
#endif
    }

    void CPU::reservation() {
        RS_nxt = RS_prev;
        if (COM_PUB.valid && COM_PUB.toPUB_jumpFlag) {
            RS_prev.clear(), RS_nxt.clear();
            IS_RS.clear(); //last cycle fetched
        }
        if (IS_RS.valid) {
            RS_nxt.insert(IS_RS.toRS);
        }
        if (EX_PUB.valid)
            RS_nxt.muxWrite(EX_PUB.toPUB_ROB_id, EX_PUB.toPUB_ALUOut);
        if (SLB_PUB_prev.valid)
            RS_nxt.muxWrite(SLB_PUB_prev.toPUB_ROB_id, SLB_PUB_prev.toPUB_loadOut);

        RS_EX.clear(); //not founded should clear as well
        auto exPos = RS_prev.exFind();
        if (exPos >= 0) {
            RS_EX.valid = true;
            RS_EX.toEX_inst = RS_prev.node[exPos].IR;
            RS_EX.toEX_A = RS_prev.node[exPos].V1;
            RS_EX.toEX_B = RS_prev.node[exPos].V2;
            RS_EX.toEX_pc = RS_prev.node[exPos].pc;
            RS_EX.toEX_ROB_id = RS_prev.node[exPos].ROB_id;
            RS_nxt.del(exPos);
        }
#ifdef DEBUG
        std::cout << '\n' << "* RS *" << '\n';
        std::cout << "[Input] ";
        IS_RS.debug(IS_RS_Type);
        std::cout << "[Output] ";
        RS_EX.debug(RS_EX_Type);
#endif
    }

    void CPU::storeLoadBuffer() {
        SLB_nxt = SLB_prev;
        SLB_PUB_nxt = SLB_PUB_prev;

        assert(!SLB_nxt.q.full()); //will it full?
        if (COM_PUB.toPUB_jumpFlag) {
            SLB_prev.clear(), SLB_nxt.clear();
            IS_SLB.clear();
        }
        if (IS_SLB.valid) SLB_nxt.insert(IS_SLB.toSLB);
        if (EX_PUB.valid)
            SLB_nxt.muxWrite(EX_PUB.toPUB_ROB_id, EX_PUB.toPUB_ALUOut);
        if (SLB_PUB_prev.valid)
            SLB_nxt.muxWrite(SLB_PUB_prev.toPUB_ROB_id, SLB_PUB_prev.toPUB_loadOut);

        SLB_PUB_nxt.clear();
        if (!SLB_prev.q.empty()) {
            auto& qf = SLB_prev.q.getHead();
            if (memClock) memClock--;
            if (!memClock && qf.isReady()) {
                if (qf.IR.ins > LHU && qf.tim == -1) {
                    SLB_PUB_nxt.valid = true; //SLB->ROB, make it ready
                    SLB_PUB_nxt.toPUB_inst = qf.IR;
                    SLB_PUB_nxt.toPUB_rd = qf.IR.rd;
                    SLB_PUB_nxt.toPUB_ROB_id = qf.ROB_id; //S loadOut does not matter
                    SLB_nxt.q.getHead().tim = 0;
                }
                else {
                    auto memTar = qf.V1 + qf.IR.imm;
                    SLB_PUB_nxt.valid = true;
                    SLB_PUB_nxt.toPUB_inst = qf.IR;
                    SLB_PUB_nxt.toPUB_rd = qf.IR.rd;
                    SLB_PUB_nxt.toPUB_ROB_id = qf.ROB_id;
                    if (qf.IR.ins <= LHU) {
                        SLB_nxt.deque();
                        memClock = 3;
                        switch (qf.IR.ins) {
                            case LB: {
                                SLB_PUB_nxt.toPUB_loadOut = mem.reads(memTar, 1);
                            }
                                break;
                            case LH: {
                                SLB_PUB_nxt.toPUB_loadOut = mem.reads(memTar, 2);
                            }
                                break;
                            case LW: {
                                SLB_PUB_nxt.toPUB_loadOut = mem.reads(memTar, 4);
                            }
                                break;
                            case LBU: {
                                SLB_PUB_nxt.toPUB_loadOut = mem.read(memTar, 1);
                            }
                                break;
                            case LHU: {
                                SLB_PUB_nxt.toPUB_loadOut = mem.read(memTar, 2);
                            }
                                break;
                        }
                    } else {
                        if (COM_PUB.valid && COM_PUB.toPUB_inst == qf.IR) {
                            SLB_nxt.deque();
                            memClock = 3;
                            switch (qf.IR.ins) {
                                case SB: {
                                    mem.write(memTar, qf.V2, 1);
                                }
                                    break;
                                case SH: {
                                    mem.write(memTar, qf.V2, 2);
                                }
                                    break;
                                case SW: {
                                    mem.write(memTar, qf.V2, 4);
                                }
                                    break;
                            }
                        }
                    }
                }
            }
        }
#ifdef DEBUG
        std::cout << '\n' << "* SLB *" << '\n';
        std::cout << "[Input] ";
        IS_SLB.debug(IS_SLB_Type);
        std::cout << "[Output] ";
        SLB_PUB_nxt.debug(SLB_PUB_Type);
#endif
    }

    void CPU::regFile() {
        RF_nxt = RF_prev;

        //COM to RF
        if (COM_RF.valid) {
            //RF_nxt.writeQ(COM_RF.toRF_rd, COM_RF.toRF.Q);
            //RF_nxt.writeV(COM_RF.toRF_rd, COM_RF.toRF.V);
            if (COM_RF.toRF_rd > 32) std::cout << COM_RF.toRF_rd << '\n';
            if (COM_RF.toRF_rd) {
                if (RF_nxt.regs[COM_RF.toRF_rd].Q == COM_RF.toRF.Q)
                    RF_nxt.regs[COM_RF.toRF_rd].Q = -1;
                RF_nxt.regs[COM_RF.toRF_rd].V = COM_RF.toRF.V;
            }
        }

        //clear RF
        if (COM_PUB.toPUB_jumpFlag) {
            RF_prev.clearQ(), RF_nxt.clearQ();
            IS_RF.clear();
        }

        //IS to RF
        if (IS_RF.valid) {
            //RF_nxt.writeQ(IS_RF.toRF_rd, IS_RF.toRF.Q);
            if (IS_RF.toRF_rd) RF_nxt.regs[IS_RF.toRF_rd].Q = IS_RF.toRF.Q;
        }
#ifdef DEBUG
        std::cout << '\n' << "* RF *" << '\n';
        std::cout << "[Input] ";
        COM_RF.debug(COM_RF_Type);
        IS_RF.debug(IS_RF_Type);
#endif
    }

    void CPU::reorderBuffer() {
        ROB_nxt = ROB_prev;
        if (IS_ROB.valid) {
            ROB_nxt.enque(IS_ROB.toROB);
        }
        if (COM_PUB.toPUB_jumpFlag) {
            ROB_prev.clear(), ROB_nxt.clear();
        }
        if (EX_PUB.valid) {
            ROB_nxt.que[EX_PUB.toPUB_ROB_id] = (ROBNode) {false, true, EX_PUB.toPUB_hit,
                                                          EX_PUB.toPUB_inst, EX_PUB.toPUB_ALUOut, EX_PUB.toPUB_tarpc, EX_PUB.toPUB_ROB_id};
        }
        if (SLB_PUB_prev.valid) {
            ROB_nxt.que[SLB_PUB_prev.toPUB_ROB_id] =
                    (ROBNode) {false, true, false, SLB_PUB_prev.toPUB_inst, SLB_PUB_prev.toPUB_loadOut,
                               SLB_PUB_prev.toPUB_tarpc, EX_PUB.toPUB_ROB_id};
        }
        ROB_COM.clear();
        if (!ROB_prev.empty()) {
            auto& qf = ROB_prev.getHead();
            if (qf.ready) {
                ROB_COM.valid = true;
                ROB_COM.toCOM = qf;
                qf.isCommit = true;
                ROB_nxt.deque();
            }
        }
#ifdef DEBUG
        std::cout << '\n' << "* ROB *" << '\n';
        std::cout << "[Input] ";
        IS_ROB.debug(IS_ROB_Type);
        EX_PUB.debug(EX_PUB_Type);
        std::cout << "[Output] ";
        ROB_COM.debug(ROB_COM_Type);
#endif
    }

    void CPU::commit() {
        COM_RF.clear();
        COM_PUB.clear();

        if (!ROB_COM.valid) return;

        //to RF
        COM_RF.toRF_rd = ROB_COM.toCOM.IR.rd;
        COM_RF.toRF = (RFNode) {ROB_COM.toCOM.ROB_id, ROB_COM.toCOM.dat};
        COM_RF.valid = true;

        //commit info to public
        COM_PUB.valid = true;
        COM_PUB.toPUB_inst = ROB_COM.toCOM.IR;

        //jump info to public (make sure the clear)
        if (ROB_COM.toCOM.hit) {
            COM_PUB.toPUB_jumpFlag = true;
            COM_PUB.toPUB_tarpc = ROB_COM.toCOM.pc;
        }
#ifdef DEBUG
        std::cout << '\n' << "* COM *" << '\n';
        std::cout << "[Input] ";
        ROB_COM.debug(ROB_COM_Type);
        std::cout << "[Output] ";
        COM_PUB.debug(COM_PUB_Type);
#endif
    }

    void CPU::execute() {
        EX_PUB.clear();
        if (!RS_EX.valid) return;
        //EX result to public
        EX_PUB.valid = true;
        EX_PUB.toPUB_rd = RS_EX.toEX_inst.rd;
        EX_PUB.toPUB_inst = RS_EX.toEX_inst;
        EX_PUB.toPUB_ROB_id = RS_EX.toEX_ROB_id;
        switch (RS_EX.toEX_inst.ins) {
            case JAL:{
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
                EX_PUB.toPUB_hit = true, EX_PUB.toPUB_ALUOut = RS_EX.toEX_pc + 4;
            }break;
            case JALR:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut & ~1;
                EX_PUB.toPUB_hit = true, EX_PUB.toPUB_ALUOut = RS_EX.toEX_pc + 4;
            }break;
            case LUI:{
                EX_PUB.toPUB_ALUOut = RS_EX.toEX_inst.imm;
            }break;
            case AUIPC:{
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case BEQ:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-');
                EX_PUB.toPUB_hit = alu.zero;
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
            }break;
            case BNE:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-');
                EX_PUB.toPUB_hit = !alu.zero;
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
            }break;
            case BLT:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-');
                EX_PUB.toPUB_hit = alu.neg;
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
            }break;
            case BGE:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-');
                EX_PUB.toPUB_hit = !alu.neg;
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
            }break;
            case BLTU:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-', false);
                EX_PUB.toPUB_hit = alu.neg;
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
            }break;
            case BGEU:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-', false);
                EX_PUB.toPUB_hit = !alu.neg;
                alu.input(RS_EX.toEX_pc, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_tarpc = alu.ALUOut;
            }break;
            case ADDI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '+');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SLTI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '-');
                EX_PUB.toPUB_ALUOut = alu.neg;
            }break;
            case SLTIU:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '-', false);
                EX_PUB.toPUB_ALUOut = alu.neg;
            }break;
            case XORI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '^');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case ORI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '|');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case ANDI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.imm, '&');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SLLI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.shamt, '<');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SRLI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.shamt, '>', false);
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SRAI:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_inst.shamt, '>', true);
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case ADD:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '+');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SUB:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SLL:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '<');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SLT:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-');
                EX_PUB.toPUB_ALUOut = alu.neg;
            }break;
            case SLTU:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '-', false);
                EX_PUB.toPUB_ALUOut = alu.neg;
            }break;
            case XOR:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '^');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SRL:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '>', false);
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case SRA:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '>', true);
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case OR:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '|');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
            case AND:{
                alu.input(RS_EX.toEX_A, RS_EX.toEX_B, '&');
                EX_PUB.toPUB_ALUOut = alu.ALUOut;
            }break;
        }
#ifdef DEBUG
        std::cout << '\n' << "* EX *" << '\n';
        std::cout << "[Input] ";
        RS_EX.debug(RS_EX_Type);
        std::cout << "[Output] ";
        EX_PUB.debug(EX_PUB_Type);
#endif
    }
}
