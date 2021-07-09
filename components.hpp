//
// Created by SiriusNEO on 2021/7/5.
//

#ifndef RISC_V_SIMULATOR_COMPONENTS_HPP
#define RISC_V_SIMULATOR_COMPONENTS_HPP

#include "include.hpp"

namespace RISC_V {
    class Memory {
    private:
        uint32_t memPool[MEM_SIZE];
        size_t siz;
    public:
        Memory() : siz(0) {
            memset(memPool, 0, sizeof(memPool));
            std::string buffer;
            while (std::cin >> buffer) {
                std::stringstream trans;
                if (buffer[0] == '@') {
                    trans << std::hex << buffer.substr(1);
                    trans >> siz;
                } else {
                    trans << std::hex << buffer;
                    trans >> memPool[siz++];
                    //std::cout << memPool[siz-1] << '\n';
                }
            }
#ifdef DEBUG
            MINE("build finish. size: " << siz)
#endif
        }

        uint32_t * getPool() {return memPool;}

        size_t size() const { return siz; }

        uint32_t read(size_t pos, size_t bytes = 1) const {
            uint32_t ret = 0;
            for (int i = pos + bytes - 1; i >= signed(pos); --i)
                ret = (ret << 8) + memPool[i];
            return ret;
        }

        int32_t reads(size_t pos, size_t bytes) const {
            int32_t ret = 0;
            for (int i = pos + bytes - 1; i >= signed(pos); --i)
                ret = (ret << 8) + memPool[i];
            return ret;
        }

        void write(size_t pos, uint32_t val, size_t bytes = 1) {
            for (int i = pos; i < pos + bytes; ++i)
                memPool[i] = slice(val, 0, 7), val >>= 8;
        }
    };

    class ALU {
    public:
        uint32_t ALUOut;
        bool neg, overflow, zero;

        ALU() = default;

        void input(uint32_t input1, uint32_t input2, char op, bool sign = true) {
            switch (op) {
                case '+':
                    ALUOut = input1 + input2;
                    break;
                case '-':
                    ALUOut = input1 - input2;
                    break;
                case '<':
                    ALUOut = input1 << input2;
                    break;
                case '>':
                    ALUOut = (sign) ? sext(input1 >> input2, 31 - input2) : (input1 >> input2);
                    break;
                case '&':
                    ALUOut = input1 & input2;
                    break;
                case '|':
                    ALUOut = input1 | input2;
                    break;
                case '^':
                    ALUOut = input1 ^ input2;
                    break;
            }
            zero = (ALUOut == 0);
            if (!sign) neg = (input1 < input2); //unsigned, fix sign-bit
            else neg = (signed(ALUOut) < 0);
        }
    };
}

#endif //RISC_V_SIMULATOR_COMPONENTS_HPP
