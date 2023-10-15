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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  if(result>=65535){reutrn 0;}"
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// 随机生成数字
int MAX_NUM=10000;
void gen_num() {


  
  int num = rand() % MAX_NUM;
  sprintf(buf + strlen(buf), "%d", num);
}

// 随机生成运算符
void gen_rand_op() {
  switch (rand() % 4) {
    case 0: strcat(buf, "+"); break;
    case 1: strcat(buf, "-"); break;
    case 2: strcat(buf, "*"); break;
    default: strcat(buf, "/"); break;
  }
}

// 随机生成'('或者')'
void gen(char ch) {
  sprintf(buf + strlen(buf), "%c", ch);
}

// 随机选择[0, n)内的整数
int choose(int n) {
  return rand() % n;
}
#define MAX_DEPTH 10
static void gen_rand_expr(int depth) {
  if(depth>=MAX_DEPTH){gen_num();}
  switch (choose(3)) {
    case 0: 
      gen_num();
      break;
    case 1:
      gen('(');
      gen_rand_expr(depth+1);
      gen(')');
      break;
    default:
      gen_rand_expr(depth+1);
      gen_rand_op();
      gen_rand_expr(depth+1);
      break;
  }
}


int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr(0);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
