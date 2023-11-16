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

#include "sdb.h"

#define NR_WP 32


typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  char* expr;
  uint32_t value;
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;
WP* new_wp() {
  if (free_ == NULL) {
    panic("No free watchpoint!") ;
  }

  WP *wp = free_;
  free_ = free_->next;
  wp->next = head;
  head=wp;
  return wp;
}

void free_wp(WP *wp) {
  wp->next = free_;
  free_ = wp;
}
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
void  set_WP(char* Expr) 
{
  WP* wp=new_wp();
  wp->expr=Expr;
  wp->value=expr(Expr,NULL);
  printf("已设置节点 NO.%d 表达式%s 现值为 %u\n",wp->NO,wp->expr,wp->value);
}
/*删除序号对应节点并且把其归还回free_*/
void del_WP(int NO)
{ 
  WP* temp=head;
    WP* before =NULL;
  while(temp!=NULL)
  {
    if (temp->NO==NO)
    { 
      if (before==NULL  )
      {
        head=temp->next;
free_wp(temp);
      }
      else{before->next=temp->next;
      free_wp(temp);}
      return ;
    }
    before=temp;
    temp=temp->next;
  }
  
  panic("Wrong NO! Watchpoint not found!");
}
/*打印指定监视点的表达式及结果*/
void print_WP(int NO )
{
  WP* temp=head;
  while(temp!=NULL)
  {
    if(temp->NO==NO)
    {
      printf("NO.%d: %s value:%u",NO,temp->expr,expr(temp->expr,NULL));
    }
    temp=temp->next;
  }
  panic("Wrong NO!Fail to Print! Watchpoint not found!");
}
void info_WP()
{
  WP* temp=head;
  if (temp==NULL)
  {
    printf("No watchpoint!\n");
  }
  while(temp!=NULL)
  {

    printf("NO.%d: %s value:%u\n",temp->NO,temp->expr,expr(temp->expr,NULL));
    temp=temp->next;
  }
}
int difftest_check()
{
  int result=0;
  WP* temp=head;
  while(temp!=NULL)
  {
    if (temp->value!=expr(temp->expr,NULL))
    {
      result=1;
      int new_value=expr(temp->expr,NULL);
      printf("Watchpoint NO.%d: %s old_value:%u new_value:%u\n",temp->NO,temp->expr,temp->value,new_value);
      temp->value=new_value;
      }
  }
  return result;
}