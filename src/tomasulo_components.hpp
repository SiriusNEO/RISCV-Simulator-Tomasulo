//
// Created by SiriusNEO on 2021/7/6.
//

#ifndef RISC_V_SIMULATOR_TOMASULO_COMPONENTS_HPP
#define RISC_V_SIMULATOR_TOMASULO_COMPONENTS_HPP

#include "include.hpp"

namespace RISC_V {
    struct RFNode {
        int Q;
        uint32_t V;
        void clear() {Q = -1, V = 0;}
    };

    template<size_t SIZ = 32>
    struct RegFile{
        RFNode regs[SIZ];

        RegFile() {for (int i = 0; i < SIZ; ++i) regs[i].clear();}

        void clearQ() {
            for (int i = 0; i < SIZ; ++i) regs[i].Q = -1;
        }

        void display() {
            std::cout << "RegFile\n";
            for (int i = 0; i < SIZ; ++i) std::cout << regName[i] << ": " <<
            "Q:" << regs[i].Q << " V:" << regs[i].V << ' ' << '\n';
        }
    };

    struct IQNode {
        uint32_t insCode, pc;
    };

    struct RSNode {
        bool busy;
        Instruction IR;
        uint32_t V1, V2;
        int Q1, Q2, ROB_id;
        uint32_t pc; //0 based
        void clear() {
            busy = false;
            IR.init();
            V1 = V2 = pc = 0;
            ROB_id = -1;
            Q1 = Q2 = -1;
        }
        bool isReady() {
            return busy && Q1 == -1 && Q2 == -1;
        }
    };

    template<size_t SIZ>
    class ReservationStation {
    private:
            size_t siz;
    public:
            RSNode node[SIZ];


        ReservationStation(): siz(0) {
            for (int i = 0; i < SIZ; ++i) node[i].clear();
        }

        bool full() {return siz >= SIZ;}

        void insert(const RSNode& newNode) {
            for (int i = 0; i < SIZ; ++i) {
                if (!node[i].busy) {
                    node[i] = newNode;
                    node[i].busy = true;
                    ++siz;
                    return;
                }
            }
        }

        void muxWrite(size_t ROB_id, uint32_t V) {
            for (int i = 0; i < SIZ; ++i)
                if (node[i].busy) {
                    if (node[i].Q1 == ROB_id) node[i].V1 = V, node[i].Q1 = -1;
                    else if (node[i].Q2 == ROB_id) node[i].V2 = V, node[i].Q2 = -1;
                }
        }

        int exFind() {
            for (int i = 0; i < SIZ; ++i)
                if (node[i].isReady()) {
                    return i;
                }
            return -1;
        }

        void del(size_t pos) {
            node[pos].clear();
            --siz;
        }

        void clear() {
            siz = 0;
            for (int i = 0; i < SIZ; ++i)
                node[i].clear();
        }

    };

    struct ROBNode {
        bool isCommit, ready, hit;
        Instruction IR;
        uint32_t dat, pc;
        int ROB_id;

        void clear() {
            isCommit = ready = hit = false;
            IR.init();
            dat = pc = 0;
            ROB_id = -1;
        }
    };

    struct SLBNode {
        bool busy;
        Instruction IR;
        uint32_t V1, V2;
        int Q1, Q2, tim, ROB_id;
        uint32_t pc;

        void clear() {
            busy = false;
            IR.init();
            V1 = V2 = 0;
            tim = Q1 = Q2 = -1;
            ROB_id = -1;
            pc = 0;
        }

        bool isReady() {
            return busy && Q1 == -1 && Q2 == -1;
        }
    };

    template<size_t SIZ>
    class StoreLoadBuffer {
    private:
        size_t siz;

    public:
        Queue<SLBNode, SIZ> q;

        StoreLoadBuffer(): siz(0), q() {}

        void muxWrite(size_t ROB_id, uint32_t V) {
            for (int i = 0; i < SIZ; ++i)
                if (q.que[i].busy) {
                    if (q.que[i].Q1 == ROB_id) q.que[i].V1 = V, q.que[i].Q1 = -1;
                    else if (q.que[i].Q2 == ROB_id) q.que[i].V2 = V, q.que[i].Q2 = -1;
                }
        }

        bool full() {return siz >= SIZ;}

        void insert(const SLBNode& newNode) {
            q.enque(newNode);
            ++siz;
        }

        void deque() {
            q.deque();
            --siz;
        }

        void clear() {
            siz = 0;
            q.clear();
        }
    };

    enum CDBType {IS_RF_Type, IS_RS_Type, IS_SLB_Type, IS_ROB_Type, RS_EX_Type,
        ROB_COM_Type, EX_PUB_Type, SLB_PUB_Type, COM_RF_Type, COM_PUB_Type};

    struct CDBNode { //for all cdb
        //Not empty
        bool valid;
        //to RF
        RFNode toRF; uint32_t toRF_rd;
        //to RS
        RSNode toRS;
        //to ROB
        ROBNode toROB;
        //ROB to COM
        ROBNode toCOM;
        //to SLB
        SLBNode toSLB;
        //to EX
        Instruction toEX_inst;
        uint32_t toEX_A, toEX_B, toEX_pc;
        int toEX_ROB_id;
        //EX/SLB to PUB
        Instruction toPUB_inst;
        uint32_t toPUB_rd, toPUB_ALUOut, toPUB_loadOut, toPUB_tarpc;
        int toPUB_ROB_id;
        bool toPUB_hit, toPUB_jumpFlag;

        void clear() {
            valid = false;
            toRF.clear(); toRF_rd = 0;
            toRS.clear(); toROB.clear();
            toCOM.clear();
            toSLB.clear();
            toEX_inst.init(); toEX_A = toEX_B = toEX_pc = 0; toEX_ROB_id = -1;
            toPUB_inst.init();
            toPUB_rd = toPUB_ALUOut = toPUB_loadOut = toPUB_tarpc = 0; toPUB_ROB_id = -1;
            toPUB_hit = false;
            toPUB_jumpFlag = false; //remember clear
        }

        void debug(CDBType type) {
            std::cout << "CDB: ";
            if (!valid) {
                std::cout << "empty\n";
                return;
            }
            switch (type) {
                case IS_RF_Type:
                    std::cout << "IS to RF" << '\n'
                    << "Q:" << toRF.Q << " V:" << toRF.V << " rd:" << toRF_rd << '\n';
                break;
                case IS_RS_Type:
                    std::cout << "IS to RS" << '\n'
                    << insName[toRS.IR.ins] << " Q1:" << toRS.Q1 << " Q2:" << toRS.Q2
                    << " V1:" << toRS.V1 << " V2:" << toRS.V2
                    << " ROB_id:" << toRS.ROB_id << '\n';
                break;
                case IS_SLB_Type:
                    std::cout << "IS to SLB" << '\n'
                    << insName[toSLB.IR.ins] << " Q1:" << toSLB.Q1 << " Q2:" << toSLB.Q2
                    << " V1:" << toSLB.V1 << " V2:" << toSLB.V2
                    << " ROB_id:" << toSLB.ROB_id << '\n';
                    break;
                case IS_ROB_Type:
                    std::cout << "IS to ROB" << '\n'
                    << insName[toROB.IR.ins] << " pc:" << toROB.pc << '\n';
                    break;
                case RS_EX_Type:
                    std::cout << "RS to EX" << '\n'
                    << insName[toEX_inst.ins] <<
                    " rd:" << toEX_inst.rd <<
                    " A:" << toEX_A <<
                    " B:" << toEX_B <<
                    " imm:" << toEX_inst.imm <<
                    " pc:" << toEX_pc << '\n';
                    break;
                case ROB_COM_Type:
                    std::cout << "ROB to COM" << '\n'
                    << insName[toCOM.IR.ins] <<
                    " val:" << toCOM.dat << '\n';
                break;
                case EX_PUB_Type:
                    std::cout << "EX Result" << '\n'
                    << insName[toPUB_inst.ins] <<
                    " aluout:" << toPUB_ALUOut <<
                    " rd:" << toPUB_rd << " hit:" << toPUB_hit << " tarpc:" << toPUB_tarpc << '\n';
                break;
                case SLB_PUB_Type:
                    std::cout << "SLB to PUB" << '\n'
                    << insName[toPUB_inst.ins] <<
                    " loadOut:" << toPUB_loadOut <<
                    " rd:" << toPUB_rd << '\n';
                break;
                case COM_PUB_Type:
                    std::cout << "COM to PUB\n"
                    << insName[toPUB_inst.ins] << " jump:" << toPUB_jumpFlag << " jumptar:" << toPUB_tarpc << '\n';
                break;
            }
        }
    };
}

#endif //RISC_V_SIMULATOR_TOMASULO_COMPONENTS_HPP
