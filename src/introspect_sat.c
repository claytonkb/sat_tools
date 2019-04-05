// introspect.c

#include "sat_tools.h"
#include "babel.h"
#include "introspect_sat.h"
#include "bstring.h"
#include "mem.h"
#include "array.h"
#include "math.h"

//typedef struct {
//    uint64_t *variables;
//    uint64_t *clauses;
//    uint8_t  *clause_lengths;
//    int       num_variables;
//    int       num_clauses;
//    int       num_assignments;
//} clause_list;

// XXX Use the following command to view:
//      neato -Tsvg work/test.dot > work/test.svg
str introspect_sat_gv(babel_env *be, st_state *st, clause_list *cl){

    int i,j;

    int approx_num_lines = cl->num_assignments+cl->num_variables+cl->num_clauses;
    mword *result = mem_new_str(be, 80*approx_num_lines+500, 0);
//_dd(80*approx_num_lines+500);

    mword str_offset  = 0;

    bsprintf(be, result, &str_offset, "digraph babel {\nnode [shape=record];\n");
    bsprintf(be, result, &str_offset, "graph [rankdir = \"LR\"];\n");
//    bsprintf(be, result, &str_offset, "splines=ortho;\n");
    
    bsprintf(be, result, &str_offset, "subgraph variables {\n");
        bsprintf(be, result, &str_offset, "var_x0  [style=invis]\n");
        bsprintf(be, result, &str_offset, "var_x00 [style=invis]\n");
        bsprintf(be, result, &str_offset, "splines=ortho;\n");
        for(i=0;i<cl->num_variables;i++)
            bsprintf(be, result, &str_offset, "var_x%d [style=bold,shape=record,label=\"<f0> x%d|<f1> ~x%d\"]\n", i+1, i+1, i+1);
        bsprintf(be, result, &str_offset, "var_x0 -> var_x00 ");
        for(i=0;i<cl->num_variables;i++)
            bsprintf(be, result, &str_offset, "-> var_x%d", i+1);
        bsprintf(be, result, &str_offset, " [style=invis];\n");
    bsprintf(be, result, &str_offset, "}\n");

    bsprintf(be, result, &str_offset, "subgraph clauses {\n");
        bsprintf(be, result, &str_offset, "rank=same;\n");
        for(i=0;i<cl->num_clauses;i++)
            bsprintf(be, result, &str_offset, "cls_%d [style=bold,shape=oval]\n", i);
        bsprintf(be, result, &str_offset, "cls_0 ");
        for(i=1;i<cl->num_clauses;i++)
            bsprintf(be, result, &str_offset, "-> cls_%d", i);
        bsprintf(be, result, &str_offset, " [style=invis];\n");
    bsprintf(be, result, &str_offset, "}\n");

    mword *clause;
    int tempi;

    for(i=0;i<size(st->clause_array);i++){
        clause = rdp(st->clause_array,i);
        for(j=0;j<size(clause);j++){
            tempi = rdv(clause,j);
            if(tempi<0){
                bsprintf(be, result, &str_offset, "var_x%d:f0 -> cls_%d [constraint=false, style=solid, arrowhead=\"none\"];\n", abs(tempi), i);
            }
            else{
                bsprintf(be, result, &str_offset, "var_x%d:f1 -> cls_%d [constraint=false, style=solid, arrowhead=\"none\"];\n", tempi, i);
            }
        }
    }

    bsprintf(be, result, &str_offset, "}\n");

    array_shrink(be,result,0,str_offset-1,U8_ASIZE);
//    mword final_size = array8_size(result);
//    _dd(final_size);

    return result;

}


#if 0
// XXX Use the following command to view:
//      neato -Tsvg work/test.dot > work/test.svg
str introspect_sat_gv(babel_env *be, st_state *st, clause_list *cl){

    int i,j;

    int approx_num_lines = cl->num_assignments+cl->num_variables+cl->num_clauses;
    mword *result = mem_new_str(be, 80*approx_num_lines+500, 0);
//_dd(80*approx_num_lines+500);

    mword str_offset  = 0;

    bsprintf(be, result, &str_offset, "graph babel {\nnode [shape=record];\n");
    bsprintf(be, result, &str_offset, "graph [rankdir = \"LR\"];\n");
    bsprintf(be, result, &str_offset, "edge [style=bold, weight=100];\n");
    bsprintf(be, result, &str_offset, "overlap=scale;\n");

    for(i=0;i<cl->num_variables;i++)
        bsprintf(be, result, &str_offset, "var_x%d [style=bold,shape=record,color=magenta,label=\"<f0> x%d|<f1> ~x%d\"]\n", i+1, i+1, i+1);

    for(i=0;i<cl->num_clauses;i++)
        bsprintf(be, result, &str_offset, "cls_%d [style=bold,shape=oval,color=cyan]\n", i);

    mword *clause;
    int tempi;

    for(i=0;i<size(st->clause_array);i++){
        clause = rdp(st->clause_array,i);
        for(j=0;j<size(clause);j++){
            tempi = rdv(clause,j);
            if(tempi<0){
                bsprintf(be, result, &str_offset, "var_x%d:f0 -- cls_%d [minlen=5]\n", abs(tempi), i);
            }
            else{
                bsprintf(be, result, &str_offset, "var_x%d:f1 -- cls_%d [minlen=5]\n", tempi, i);
            }
        }
    }

    bsprintf(be, result, &str_offset, "}\n");

    array_shrink(be,result,0,str_offset-1,U8_ASIZE);
//    mword final_size = array8_size(result);
//    _dd(final_size);

    return result;

}
#endif

// Clayton Bauman 2018

