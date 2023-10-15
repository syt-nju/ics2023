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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
extern word_t vaddr_read(vaddr_t addr, int len) ;

enum {
    TK_NOTYPE = 256, TK_EQ,  TK_PLUS='+',

  /* TODO: Add more token types */
  TK_MINUS='-',TK_MUL='*',TK_DIV='/',TK_LPAREN='(',TK_RPAREN=')', TK_NUM='n',TK_HEX='h',TK_pointer='s',TK_REG='$',TK_REGARG='A',

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", TK_MINUS},         // minus
  {"\\*", TK_MUL},         //multiply
  {"\\/", TK_DIV}, 
  {"\\(",TK_LPAREN},
  {"\\)",TK_RPAREN},
  {"0[xX][0-9a-fA-F]+",TK_HEX},
  {"[0-9]+",TK_NUM},          //'n' stands for number
  {"\\$",TK_REG},
  {"\\$0|ra|sp|gp|tp|t[0-6]|s[0-9]|s1[0-1]",TK_REGARG}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;
  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;
        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */


        switch (rules[i].token_type) {
          case TK_NOTYPE:break;
          case '+': 
          case '-':
          case '*':
          case '/':
          case '(':
          case ')':
          case 'n':
          case 'h':
          strncpy(tokens[nr_token].str, substr_start, substr_len);
          tokens[nr_token].str[substr_len]='\0';
          tokens[nr_token].type = rules[i].token_type;
          nr_token++;
          break;
          default: assert(0);
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p,int q)
{ if (tokens[p].type != '(' || tokens[q].type != ')')
    return false;
  int count=0;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
    {count++;}
    if (tokens[i].type == ')')
    {count--;}
    if (count == 0&& i!=q)
    {
      return false;
    }
    if (count < 0)
    {printf("parentheses dismatch");
    assert(0);
      return false;}
  }
  return true;
}
/*function to check if the token is operand or operator*/
bool is_operand(Token token)
{
  if (token.type == 'n'||token.type=='h')
    return true;
  else return false;
}
/*function to get the 主运算符 of given cut of expression*/
int get_op(int p,int q)
{
  int result=-1;
  int parenthesis_count=0;//用来确定括号作用域
  for (int i = p; i <= q; i++)
  {/*先排除操作数*/
    if (is_operand(tokens[i]))
    {continue;}
    
    /*排除括号*/
    if (tokens[i].type=='(')
    {
      parenthesis_count++;continue;}
    if (tokens[i].type==')')
    {parenthesis_count--;continue;}
    if (parenthesis_count>0)
    {continue;}
    /*丑陋的初始化问题的答辩*/
    if (result==-1&&tokens[i].type!=TK_pointer&&tokens[i].type!=TK_REG&&tokens[i].type!=TK_REGARG){result=i;continue;}

    /*比较优先级*/
    switch(tokens[i].type)
    {                 //p 0x80000004 + *(0x80000008 - 4) * 2

      case '+':
      case '-':
      if (tokens[result].type=='*'||tokens[result].type=='/'||tokens[result].type=='+'||tokens[result].type=='-')
      {result=i;}
      break;
      case '*':
      case '/':
      if (tokens[result].type=='*'||tokens[result].type=='/')
      {result=i;}
      break;
      default:break;
    } 
  }
  return result;
}
/*function to get evaluation result*/
word_t eval(int p,int q)
{
  if (p > q) {
    /* Bad expression */
    printf("Bad expression\n");
    assert(0);
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if (tokens[p].type == 'n')
    {return atoi(tokens[p].str);}
    else if (tokens[p].type == 'h')
    {
      int temp=strtoul(tokens[p].str, NULL, 16);  //0x80000004 + *(0x80000008 - 4) * 2

      return (unsigned int)temp;
    }
    else assert(0);
  }
  else if (check_parentheses(p, q) ) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op = get_op(p,q);
    if(op==-1)
    {
      if(tokens[p].type==TK_pointer)
{       int a=vaddr_read(eval(p+1,q),4);
       return a;}
       else if (tokens[p].type==TK_REG)
       {
        bool temp=true;
        bool *success=&temp;
        return isa_reg_str2val(tokens[p+1].str, success);
       }
    }
    int op_type=tokens[op].type;
    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);

    switch (op_type) {
      case '+': return val1 + val2;
      case '-': return val1 - val2;
      case '*': return val1 * val2;
      case '/': return val1 / val2;
      default: assert(0);
    }
  }
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i ++) {
  if (tokens[i].type == '*' && (i == 0 || tokens[i - 1].type == '('||(!is_operand(tokens[i-1]) && tokens[i-1].type!=')')) ) {
    tokens[i].type = TK_pointer;
  }
  
}
  printf("%u \n",eval(0,nr_token-1));

  return 0;
}

