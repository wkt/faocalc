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


#define CALC_ITEM_SIZE (sizeof(CalcItem))

static const char *operators[]=
{
    [OPERATOR_ADD] = "+",
    [OPERATOR_SUB] = "-",
    [OPERATOR_MULTI] = "*",
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

static unsigned short MAX_DOT_CHAR  = 6;

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

static inline void
strip_dot_zero(char *s)
{
    int i = 0;
    int not_found = 1;
    int found_i = -1;
    for(i=0;s && s[i];i++)
    {
        if(s[i] == '.')
        {
            found_i = i;
        }
        if(found_i > 0 && i > found_i && s[i] != '0')
        {
            not_found = 0;
            break;
        }
    }
    if(s)
    {
        if(found_i > 0 && not_found)
        {
            s[found_i] = 0;
        }else{
            int len = strlen(s);
            for(i = len-1;i > 0;i--)
            {
                if(s[i] == '0')
                {
                    s[i] = 0;
                }else{
                    break;
                }
            }
        }
    }
}

static inline CalcResult *
calc_result_new(void)
{
    CalcResult *res = malloc(sizeof(CalcResult));
    bzero(res,sizeof(CalcResult));
    return res;
}

static inline void
calc_result_append(CalcResult *crt,const char *s)
{
    crt->array = realloc(crt->array,sizeof(char*)*(crt->length+2));
    crt->array[crt->length] = strdup(s);
    crt->array[crt->length+1] = NULL;
    crt->length ++;
}

static inline char*
calc_result_join(CalcResult *crt,const char *separator)
{
    char *result_string = NULL;
    size_t n = 0;
    int i;
    int len;
    int spr_len = 0;
    if(separator == NULL)
    {
        separator = "\n";
    }
    spr_len = strlen(separator);
    if(crt)
    {
        for(i=0;i<crt->length;i++)
        {
            if(crt->array[i])
            {
                len = strlen(crt->array[i]);
                result_string = realloc(result_string,n+len+spr_len);
                n+=sprintf(result_string+n,"%s%s",i==0?"":separator,crt->array[i]);
            }
        }
    }
    return result_string;
}

static inline void
calc_result_free(CalcResult *crt,short free_array_only)
{
    size_t i = 0;
    if(crt)
    {
        for(;i<crt->length;i++)
        {
            if(crt->array[i])
            {
                free(crt->array[i]);
                crt->array[i] = NULL;
            }
        }
        if(!free_array_only)free(crt);
    }
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
            if(i< gap2-gap1)
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
                    if(op2 == OPERATOR_NONE && i== 1)
                    {
///                        calc_item_print(it);
                        calc_item_exp_real_calc(it);
                    }
                    break;
                }
            }
        }
    }
}

static inline void
calc_item_to_result(CalcItem *it,CalcResult *crt)
{
    char *tmp_string = NULL;
    if(crt)
    {
        tmp_string = calc_item_to_string(it,1);
///        printf("--%s--\n",tmp_string);
        calc_result_append(crt,tmp_string);
        free(tmp_string);
        tmp_string = NULL;
    }
}

///1 表示完成,0
static inline short
calc_item_strip_num_only_exp(CalcItem *it,short strip_all)
{
    int i = 0;
    short d = 0;
    short f = 0;
    
head:
    d = 0;
    i = 0;
    f = 0;
    if(it && it->type == ITEM_EXP)
    {
        CalcItem **exp = it->it.exp;
        if(exp[1] == NULL)
        {
            if(exp[0]->type == ITEM_NUM)
            {
                calc_item_exp_real_calc(it);
                f=1;
            }
            if(exp[0]->type == ITEM_EXP)
            {
                CalcItem **ex = exp[0]->it.exp;
                exp[0]->it.exp = NULL;
                exp[0]->type = ITEM_NUM;
                exp[0]->it.num = 0;
                calc_item_free_exp(it->it.exp);
                it->it.exp = ex;
                f=1;
            }
        }else{
            for(;exp[i];i++)
            {
                if(exp[i]->type == ITEM_EXP)
                {
                    d = calc_item_strip_num_only_exp(exp[i],0);
                    if(d)
                    {
                        f = d;
                    }
                }
            }
        }
    }
    if(strip_all &&f)
    {
        goto head;
    }
    return f;
}

static void
calc_item_calc(CalcItem *it,CalcResult *crt)
{
    int has_exp = 0;
    
    CalcItem **exp;

    calc_item_to_result(it,crt);

    while(it->type == ITEM_EXP)
    {
        exp = it->it.exp;
        has_exp = 0;
        if(exp)
        {
            for(; *exp;exp ++)
            {
                if((*exp)->type == ITEM_EXP)
                {
                    calc_item_calc(*exp,NULL);
                    has_exp = 1;
                    break;
                }
            }
        }else{
            fprintf(stderr,"__error_exp_item__\n");
        }
        if(!has_exp)
        {
            calc_item_exp_real_calc(it);
        }
        if(crt == NULL)
            break;
        calc_item_to_result(it,crt);
    }
///    calc_item_to_result(it,crt);
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
                if(it->it.num < 0.0&& !is_top_item)
                {
                    is_negative = 1;
                }
                n = snprintf(tstr,sizeof(tstr),"%0.*lf",MAX_DOT_CHAR,it->it.num);
                if(is_negative)
                {
                    n += 4; 
                }
                res = malloc(n+1);
                strip_dot_zero(tstr);
                snprintf(res,n,"%s%s%s",is_negative?"( ":" ",tstr,is_negative?" )":" ");
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
                        n += snprintf(tstr+n,sizeof(tstr)-n,"%s",tres);
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

void
calc_item_print(const CalcItem *it)
{
    char *res = calc_item_to_string(it,1);
    fprintf(stderr,"%s\n",res);
    free(res);
}

#if defined(on_test)

int maind(int argc,char **argv)
{
    if(argc > 1)
    {
        char *end_str = NULL;
        char *res = NULL;
        CalcResult crt = {0};
        CalcItem*   it = calc_item_get(NULL,NULL,argv[1],&end_str);
        calc_item_strip_num_only_exp(it,1);
#if 0
        res = calc_item_to_string(it,1);
#else
        calc_item_calc(it,&crt);
        res = calc_result_join(&crt,"\n=");
        calc_result_free(&crt,1);
#endif
        fprintf(stderr,"%s\n",res);
        free(res);
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