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

typedef struct _CalcItem CalcItem;

struct _CalcItem
{
    CalcItemType type;
    union{
        double num;             ///number
        int    opid;            ///operator id
        CalcItem **exp;      ///array of CalcItem*,expression
    }it;
};

CalcItem*
calc_item_get(CalcItem *parent,CalcItem *last_item,const char *exp_string, char **end_str);

#ifdef __cplusplus
}
#endif
