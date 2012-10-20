/**
* *stepcalc.c
* *
* *Copyright (c) 2011 Weiketing
* *
* *Authors by:Wei Keting <weikting@gmail.com>
* *
* */

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "stepcalc.h"

typedef double (*OperatorFunc)(double n1,double n2);

typedef enum
{
    OPERATOR_NONE = -1,
    OPERATOR_ADD = 0,
    OPERATOR_SUB,
    OPERATOR_MULTI,
    OPERATOR_DIV,
    OPERATOR_LAST
}OperatorID;

#define CALC_ITEM_SIZE (sizeof(CalcItem))

static const char *operators[]=
{
    [OPERATOR_ADD] = "+",
    [OPERATOR_SUB] = "-",
    [OPERATOR_MULTI] = "x",
    [OPERATOR_DIV] = "/",
    [OPERATOR_LAST] = NULL,
};



static short op_matrix[4][4]=
{
/*     +        -      x       /         */
    { 0,      0,    -1,     -1, },      //    +
    { 0,      0,    -1,     -1, },      //    -
    { 1,      1,     0,      0, },      //    x
    { 1,      1,     0,      0, }       //    /
};

#define static 
#define inline


static inline CalcItem**
calc_item_exp(CalcItem *parent,const char *exp_string,char **end_str);

static inline void
calc_item_free(CalcItem* it);

static inline double
operator_add(double n1,double n2)
{
    return n1 + n2;
}

static inline double
operator_sub(double n1,double n2)
{
    return n1 - n2;
}

static inline double
operator_multi(double n1,double n2)
{
    return n1 * n2;
}

static inline double
operator_div(double n1,double n2)
{
    return n1 / n2;
}

static const OperatorFunc operators_funcs[]=
{
    [OPERATOR_ADD] = operator_add,
    [OPERATOR_SUB] = operator_sub,
    [OPERATOR_MULTI] = operator_multi,
    [OPERATOR_DIV] = operator_div,
    [OPERATOR_LAST] = NULL,
};

static inline const char *
operator_get(OperatorID opid)
{
    if(opid >OPERATOR_LAST|| opid == OPERATOR_NONE)
    {
        opid = OPERATOR_LAST;
    }
    return operators[opid];
}

static inline short
operator_cmpid(OperatorID id1,OperatorID id2)
{
    if(id1 == id2)
        return 0;
    if(id1 == OPERATOR_NONE)
        return -1;
    if(id2 == OPERATOR_NONE)
        return 1;
    return op_matrix[id1][id2];
}

static inline OperatorID
string_prefix_get_match_opid(const char *exp_string)
{
    OperatorID opid = OPERATOR_NONE;
    int i = 0;
    for(;operators[i];i++)
    {
        if(strncmp(operators[i],exp_string,strlen(operators[i])) == 0)
        {
            opid = i;
            break;
        }
    }
    return opid;
}

static inline short
operator_cmp_op(const char *op1,const char *op2)
{
    return operator_cmpid(string_prefix_get_match_opid(op1),string_prefix_get_match_opid(op2));
}

static inline CalcItem*
calc_item_new(CalcItemType type)
{
     CalcItem* it = malloc(CALC_ITEM_SIZE);
     if(it)
     {
         bzero(it,CALC_ITEM_SIZE);
         it->type = type;
    }
    return it;
}

static inline void
calc_item_free_exp(CalcItem** ex)
{
    CalcItem** exp = ex;
    if(exp)
    {
        for(;*exp;exp++)
        {
            calc_item_free(*exp);
        }
        free(ex);
    }
}

static inline void
calc_item_free(CalcItem* it)
{
    if(it)
    {
        switch(it->type)
        {
            case ITEM_EXP:
                calc_item_free_exp(it->it.exp);
            default:
                free(it);
            break;
        }
        it = NULL;
    }
}

static inline void
calc_item_merge_exp(CalcItem** ex,size_t gap1,size_t gap2)
{
    if(ex)
    {
        size_t i = 0;
        for(i=0;ex[gap1+i];i++)
        {
            calc_item_free(ex[gap1+i]);
            ex[gap1+i]=ex[gap2+i];
        }
    }
}
    
static inline CalcItem**
calc_item_exp(CalcItem *parent,const char *exp_string,char **end_str)
{
    char *e_str = 0;
    char *t_str = NULL;
    CalcItem    **res = NULL;
    CalcItem    *it = NULL;
    int i = 0;
    for(e_str=exp_string;*e_str;)
    {
        if(isblank(*e_str))
        {
                continue;
        }
        it = calc_item_get(parent,it,e_str,&t_str);
        if(it)
        {
            res = realloc(res,sizeof(CalcItem*)*(i+2));
            res[i] = it;
            i ++;
            res[i] = NULL;
        }else{
            fprintf(stderr,"error @%s\n",e_str);
        }
        e_str = t_str;
        if(*e_str == ')')
        {
            break;
        }
    }
    for(;*e_str;e_str++)
    {
        if(isblank(*e_str))
        {
            continue;
        }
        break;
    }
    if(end_str)
    {
        *end_str = e_str;
    }
    return res;
}

CalcItem*
calc_item_get(CalcItem *parent,CalcItem *last_item,const char *exp_string, char **end_str)
{
    char *e_str = NULL;
    OperatorID last_opid = OPERATOR_NONE;
    CalcItemType last_type = ITEM_OP;
    CalcItem    *res = NULL;
    OperatorID  opid = OPERATOR_NONE;
    const char *op = NULL;
    if(last_item)
    {
        last_type = last_item->type;
        if(last_type == ITEM_OP)
        {
            last_opid = last_item->it.opid;
        }
    }
    if(last_type == ITEM_OP)
    {
        if(exp_string[0] == '('|| (parent == NULL && last_item == NULL) )
        {
            e_str = exp_string;
            res = calc_item_new(ITEM_EXP);
            /// 在开头有(的表达式里,这个(是某个子表达的开头,
            /// 当parent==NULL时处理的是最大表达式,此时(不能被处理掉
            if(e_str[0] == '(' && parent != NULL)
                e_str ++;
            res->it.exp = calc_item_exp(res,e_str,&e_str);
            e_str ++;
        }else{
            res = calc_item_new(ITEM_NUM);
            res->it.num = strtod(exp_string,&e_str);
        }
    }else if(last_type == ITEM_NUM || last_type == ITEM_EXP){
        for(e_str=exp_string;*e_str;e_str++)
        {
            if(isblank(*e_str))
            {
                continue;
            }
            opid = string_prefix_get_match_opid(e_str);
            op = operator_get(opid);
            if(op)
            {
                res = calc_item_new(ITEM_OP);
                res->it.opid = opid;
                e_str += strlen(op);
            }
            break;
        }
    }
    if(end_str)
    {
        *end_str = e_str;
    }
    return res;
}

static inline void
calc_item_exp_real_calc(CalcItem *it)
{
    CalcItem **exp = it->it.exp;
    OperatorID op1;
    OperatorID op2;
    double num1 = 0,num2 = 0;
    int i = 0;
    if(exp)
    {
        for(;*exp;exp+=2)
        {
            op1 = op2 = OPERATOR_NONE;
            i++;
            if(exp[0]->type == ITEM_NUM)
            {
                num1 = exp[0]->it.num;
                if(exp[1])
                {
                    op1 = exp[1]->it.opid;
                    num2 = exp[2]->it.num;
                    if(exp[3])
                    {
                        op2 = exp[3]->it.opid;
                    }

                }
                if(op1 == OPERATOR_NONE)
                {
                    calc_item_free_exp(it->it.exp);
                    it->type = ITEM_NUM;
                    it->it.num = num1;
                    break;
                }else if(operator_cmpid(op1,op2) >= 0)
                {
                    double res = operators_funcs[op1](num1,num2);
                    exp[0]->it.num = res;
                    calc_item_merge_exp(exp,1,3);
                    break;
                }
            }
        }
    }
}

static void
calc_item_exp_calc(CalcItem *it)
{
    int has_exp = 0;
    if(it->type == ITEM_EXP)
    {
        CalcItem **exp = it->it.exp;
        if(exp)
        {
            for(; *exp;exp ++)
            {
                if((*exp)->type == ITEM_EXP)
                {
                    calc_item_exp_calc(*exp);
                    has_exp = 1;
                    break;
                }
            }
            if(!has_exp)
            {
                calc_item_exp_real_calc(it);
            }
        }else{
            fprintf(stderr,"__error_exp_item__\n");
        }
    }
}

char*
calc_item_to_string(const CalcItem *it,short is_top_item)
{
    char *res  = NULL;
    char *tres = NULL;
    char tstr[1024] = {0};
    const char *op = NULL;
    CalcItem **exp = NULL;
    int is_negative = 0;
    int n = 0;
    if(it)
    {
        switch(it->type)
        {
            case ITEM_NUM:
                if(it->it.num < 0.0)
                {
                    is_negative = 1;
                }
                n = snprintf(tstr,sizeof(tstr),"%s%0.2lf%s",is_negative?"(":"",it->it.num,is_negative?")":"");
                res = malloc(n+1);
                memcpy(res,tstr,n);
                res[n] = 0;
            break;
            case ITEM_OP:
                op = operator_get(it->it.opid);
                if(!op)
                {
                    op = "__error_op__";
                }
                res = strdup(op);
            break;
            case ITEM_EXP:
                n = 0;
                exp = it->it.exp;
                if(exp)
                {
                    if(!is_top_item)
                    {
                        n += snprintf(tstr+n,sizeof(tstr)-n,"(");
                    }
                    for(;*exp;exp++)
                    {
                        tres = calc_item_to_string(*exp,0);
                        n += snprintf(tstr+n,sizeof(tstr)-n," %s ",tres);
                        free(tres);
                        tres = NULL;
                    }
                    if(!is_top_item)
                    {
                        n += snprintf(tstr+n,sizeof(tstr)-n,")");
                    }
                }else{
                    n += sprintf(tstr,"(__error_exp__)");
                }
                res = malloc(n+1);
                memcpy(res,tstr,n);
                res[n] = 0;
            break;
            default:
                res = strdup("__error_item__");
            break;
        }
    }
///    printf("%s\n",res);
    return res;
}

#if defined(on_test)

int maind(int argc,char **argv)
{
    if(argc > 1)
    {
        char *end_str = NULL;
        char *res = NULL;
        CalcItem*   it = calc_item_get(NULL,NULL,argv[1],&end_str);
        res = calc_item_to_string(it,1);
        if(res)
        {
            printf("exp:%s\n",res);
            free(res);
        }

        calc_item_exp_calc(it);
        res = calc_item_to_string(it,1);
        if(res)
        {
            printf("exp:%s\n",res);
            free(res);
        }
        calc_item_free(it);
    }
    
    return 0;
}

int main1(int argc,char **argv)
{
    if(argc > 2)
    {
        printf("res:%d\n",operator_cmp_op(argv[1],argv[2]));
    }
    return 0;
}

int main(int argc,char **argv)
{
    return maind(argc,argv);
}
#endif