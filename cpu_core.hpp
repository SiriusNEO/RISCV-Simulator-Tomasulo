//
// Created by SiriusNEO on 2021/6/28.
//

#ifndef RISC_V_SIMULATOR_CPU_CORE_HPP
#define RISC_V_SIMULATOR_CPU_CORE_HPP

#include "decoder.hpp"
#include "components.hpp"
#include "tomasulo_components.hpp"

using std::queue;

namespace RISC_V {

    class CPU {
    public:
        explicit CPU():pc(0), mem() {
            IS_RF.clear(), COM_RF.clear(), IS_RS.clear(), RS_EX.clear(),
            IS_ROB.clear(), ROB_COM.clear(), IS_SLB.clear(), EX_PUB.clear(),
            COM_PUB.clear(), SLB_PUB_prev.clear(), SLB_PUB_nxt.clear();
        }

        void run() {
            while (true) {
                reorderBuffer();
                storeLoadBuffer();
                reservation();
                regFile();
                instructionQueue();
                update();
                if (ROB_COM.toCOM.IR.ins == HALT) {
                    std::cout << (RF_prev.regs[FUNCTION_RETURN].V & 255u) << '\n';
                    break;
                }
                issue();
                execute();
                commit();
#ifdef DEBUG
                RF_nxt.display();
#endif
            }
        }
        private:

            uint32_t pc;
            Decoder id;
            Memory mem;
            ALU alu;

            Queue<IQNode, BUFF_N> IQ;
            ReservationStation<BUFF_N> RS_prev, RS_nxt;
            RegFile<REG_N> RF_prev, RF_nxt;
            StoreLoadBuffer<BUFF_N> SLB_prev, SLB_nxt;
            Queue<ROBNode, BUFF_N> ROB_prev, ROB_nxt;

            //INPUT_OUTPUT
            CDBNode IS_RF, COM_RF, IS_RS, RS_EX, IS_ROB, ROB_COM, IS_SLB, EX_PUB, COM_PUB, SLB_PUB_prev, SLB_PUB_nxt;

            void issue();
            void execute();
            void commit();

            void instructionQueue();
            void regFile();
            void reservation();
            void storeLoadBuffer();
            void reorderBuffer();
            void update() {
                RF_prev = RF_nxt;
                RS_prev = RS_nxt;
                SLB_prev = SLB_nxt;
                SLB_PUB_prev = SLB_PUB_nxt;
                ROB_prev = ROB_nxt;
            }
    };
}

#endif //RISC_V_SIMULATOR_CPU_CORE_HPP
