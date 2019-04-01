// introspect.c

#include "introspect_sat.h"

//typedef struct {
//    uint64_t *variables;
//    uint64_t *clauses;
//    uint8_t  *clause_lengths;
//    int       num_variables;
//    int       num_clauses;
//    int       num_assignments;
//} clause_list;


str introspect_sat_gv(babel_env *be, clause_list *cl){

//str introspect_gv(babel_env *be, mword *bs){ // introspect_gv#
_trace;

    mword *result = mem_new_str(be, 1000, 0);

    mword str_offset  = 0;

    bsprintf(be, result, &str_offset, "digraph babel {\nnode [shape=record];\n");
//    bsprintf(be, result, &str_offset, "graph [rankdir = \"LR\"];\n");

////    introspect_gv_r(be, bs, result, &str_offset, 1);
//
////    bstruct_clean(be, bs);
//
//    bsprintf(be, result, &str_offset, "}\n");
//
//    array_shrink(be,result,0,str_offset-1,U8_ASIZE);

    return result;

}

// Clayton Bauman 2018

