/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write

enum {
  TYPE_I, TYPE_U, TYPE_S,TYPE_J,TYPE_R,TYPE_B,
  TYPE_N, // none
};

#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = (SEXT(BITS(i, 31, 12), 20) << 12); } while(0)//modified
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)


#define immB() do { *imm = SEXT(BITS(i, 31, 31), 1) << 11 | ((SEXT(BITS(i, 7, 7), 1) << 63) >> 63) << 10 | ((SEXT(BITS(i, 30, 25), 6) << 58) >> 58) << 4 | ((SEXT(BITS(i, 11, 8), 4) << 60) >> 60); *imm = *imm << 1; } while (0)
#define immJ() do { *imm = (SEXT(BITS(i, 31, 31), 1) << 20) | BITS(i, 30, 21) << 1 | BITS(i, 20, 20) << 11 | BITS(i, 19, 12) << 12 ; } while(0)

static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst.val;
  int rs1 = BITS(i, 19, 15);
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ();break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_R: src1R(); src2R();         break;

  }
}

static int decode_exec(Decode *s) {
  int rd = 0;
  word_t src1 = 0, src2 = 0, imm = 0;
  s->dnpc = s->snpc;

#define INSTPAT_INST(s) ((s)->isa.inst.val)
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
//INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
  INSTPAT_START();
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2););
//modified by syt
//to solve dummy
  INSTPAT("??????? ????? 00000 000 ????? 00100 11", li      ,I,R(rd)=imm);//addi的语法糖，将rs1设置为0，使得永远取R[0]中的0，达到0+imm -> R[dest]的效果
  INSTPAT("0000000 00000 ????? 000 ????? 00100 11", mv   , I, R(rd) = src1 + imm);//imm为0的addi
  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);
  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s -> pc + 4; s -> dnpc += imm -4;); 
  INSTPAT("??????? ????? ????? 000 00000 11001 11", ret    , I, R(rd) = s -> pc + 4; s -> dnpc = (src1 + imm) & ~1); //ret指令即为 rd设为 x0（恒为0）的那个寄存器（不需要保存下一条指令的地址）的jalr
  INSTPAT("??????? ????? ????? 000 ????? 11001 11",jalr    , I,R(rd)=s->pc+4;s -> dnpc = (src1 + imm) & ~1);//riscV地址要求为偶数  "& ~1"    
  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));//sw=store word
//to solve add
  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , R, R(rd) = src1 + src2);
  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, s -> dnpc += src1 == src2 ? imm - 4: 0;);
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , R, R(rd) = src1 - src2);

  INSTPAT("0000000 00001 ????? 011 ????? 00100 11", seqz  , I, R(rd) = (uint32_t)src1 < 1 ? 1: 0);//天才，实际上为sltui rd  rc1 1
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , R, R(rd) = (uint32_t)src1 < (uint32_t)src2 ? 1: 0;);
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = (uint32_t)src1 < (uint32_t)imm ? 1: 0);
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = (int32_t)src1 < (int32_t)imm ? 1: 0);
  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , R, R(rd) = (int)src1 < (int)src2 ? 1: 0);

  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, s -> dnpc += src1 != src2 ? imm - 4: 0;);
//to solve fact
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , R, R(rd) = src1 * src2);
//to solve string
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) += imm;printf("imm:%u\n",imm);); 


//end modification by syt
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak , N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst.val = inst_fetch(&s->snpc, 4);
  return decode_exec(s);
}
