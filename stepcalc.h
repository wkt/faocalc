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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
ITEM_NUM,
ITEM_OP,
ITEM_EXP,
}CalcItemType;

typedef long double CommonNum;

typedef enum
{
    OPERATOR_NONE = -1,
    OPERATOR_ADD = 0,
    OPERATOR_SUB,
    OPERATOR_MULTI,
    OPERATOR_DIV,
    OPERATOR_LAST
}OperatorID;
    
typedef struct _CalcItem CalcItem;
typedef struct _CalcResult CalcResult;
    
struct _CalcItem
{
    CalcItemType type;
    union{
        CommonNum num;                 ///number
        OperatorID    opid;         ///operator id
        CalcItem **exp;             ///array of CalcItem*,expression
    }it;
};

struct _CalcResult
{
    size_t length;
    char **array;
};

CalcItem*
calc_item_get(CalcItem *parent,CalcItem *last_item,const char *exp_string, const char **end_str);

char*
calc_item_to_string(const CalcItem *it,short is_top_item);

void
calc_item_print(const CalcItem *it);
    
#ifdef __cplusplus
}
#endif
