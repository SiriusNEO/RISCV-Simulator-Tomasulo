//
// Created by SiriusNEO on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_INCLUDE_HPP
#define RISC_V_SIMULATOR_INCLUDE_HPP

#include <cstdio>
#include <iostream>
#include <cstring>
#include <sstream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <assert.h>
#include <queue>

#define BOMB std::cout<<"bomb\n";
#define MINE(_x) std::cout<<_x<<'\n';

//#define DEBUG
//#define LOCAL
//#define MINE(_x) std::cout << "Important Out: " << _x << '\n';

namespace RISC_V {
    const size_t REG_N = 32, MEM_SIZE = 500000, FUNCTION_RETURN = 10, RETURN_ADDRESS = 1;
    const size_t RS_N = 64, ROB_N = 64, IQ_N = 1024, SLB_N = 64;

    enum InsType {NOP, HALT, LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU,
        LHU, SB, SH, SW, ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD,
        SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND};
    enum InsFormatType {RType, IType, SType, BType, UType, JType};
    enum StageType {IF_Stage, ID_Stage, EX_Stage, MEM_Stage, WB_Stage};

    const std::string insName[] = {"NOP", "END", "LUI", "AUIPC", "JAL", "JALR", "BEQ", "BNE", "BLT", "BGE", "BLTU", "BGEU", "LB", "LH", "LW", "LBU",
                                   "LHU", "SB", "SH", "SW", "ADDI", "SLTI", "SLTIU", "XORI", "ORI", "ANDI", "SLLI", "SRLI", "SRAI", "ADD",
                                   "SUB", "SLL", "SLT", "SLTU", "XOR", "SRL", "SRA", "OR", "AND"};

    const std::string regName[] = {"zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "fp", "s1",
                                   "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5"
                                    , "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

    struct Instruction { //指令结构体
        InsType ins;
        uint32_t opcode, rd, rs1, rs2, imm, funct3, funct7, shamt;
        Instruction():ins(NOP),opcode(0),rd(0),rs1(0),rs2(0),imm(0),funct3(0),funct7(0),shamt(0){}
        void init() {
            ins = NOP, opcode = rd = rs1 = rs2 = imm = funct3 = funct7 = shamt = 0;
        }
        bool operator == (const Instruction& obj) const {
            return ins == obj.ins && opcode == obj.opcode && rs1 == obj.rs1 && rs2 == obj.rs2 && imm == obj.imm &&
            funct3 == obj.funct3 && funct7 == obj.funct7 && shamt == obj.shamt;
        }
    };

    struct Triple { //一个普通的三元组类
        uint32_t first, second, third;
        bool operator < (const Triple& obj) const {
            if (first != obj.first) return first < obj.first;
            return (second == obj.second) ? third < obj.third : second < obj.second;
        }
        bool operator == (const Triple& obj) const {
            return first == obj.first && second == obj.second && third == obj.third;
        }
    };

    template<class T, size_t SIZ = 32>
    struct Queue {
        T que[SIZ+1];
        size_t head, tail;

        Queue() {
            head = tail = 0;
        }

        void clear() {head = tail = 0;}
        bool empty() {return head == tail;}
        bool full() {return (tail+1)%(SIZ+1) == head;}
        void enque(const T& val) {
            que[tail] = val, tail = (tail + 1) % (SIZ+1);
        }
        T deque() {
            auto ret = que[head];
            head = (head + 1) % (SIZ+1);
            return ret;
        }
        T& getHead() {
            return que[head];
        }
    };

    static uint32_t slice(uint32_t num, size_t l, size_t r) { //二进制切片，[l, r]
        if (r == 31) return num >> l;
        return (num & ((1u << (r+1)) - 1)) >> l;
    }

    static uint32_t sext(uint32_t num, size_t r) { //符号位扩展，最高位位置 r
        uint32_t mask = 0;
        if (num & (1 << r)) mask = -(1u << r);
        return num | mask;
    }

    static bool isLoad(InsType ins) {
        return ins == LB || ins == LH || ins == LW || ins == LBU || ins == LHU;
    }

    static bool isMemoryAccess(InsType ins) {
        return isLoad(ins) || ins == SB || ins == SH || ins == SW;
    }
}

#endif //RISC_V_SIMULATOR_INCLUDE_HPP
