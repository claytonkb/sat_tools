// backtrack.h
//

#ifndef BACKTRACK_H
#define BACKTRACK_H

#include "cnf_parse.h"

#include "babel.h"
#include "mem.h"

typedef enum clause_prop_enum 
    { CONFLICT_CP, 
      NO_PROP_CP,  PROP_CP,
      ASSIGN0_CP,  ASSIGN1_CP, } clause_prop;

typedef enum var_state_enum 
    { UNASSIGNED_VS, NEGATE_VS,
      DEC_ASSIGN0_VS, DEC_ASSIGN1_VS, 
      IMP_ASSIGN0_VS, IMP_ASSIGN1_VS } var_state;

typedef enum solver_cont_enum 
    { CONT_A_SC, CONT_B_SC } solver_cont;

typedef struct{

    uint32_t var;
    uint8_t  vs;
    uint8_t  padding0;
    uint8_t  padding1;
    uint8_t  padding2;

} edit_list_entry;


typedef struct{

    mword curr_var;
    mword *solver_stack;

    clause_list *cl;

    mword *clause_array;
    mword *clause_trie;
    mword *clause_sat;
    mword  clause_sat_count;

    mword *mt_var_array;
    float  mt_temp;
    float  mt_annealing_rate;

    // all var* elements are 1-indexed; the 0-bit is ignored/MBZ/nil
    mword *var_array;
    mword *var_clause_map;
    mword *var_prop_clause_map;
    mword *var_prop_var_map;
    mword *var_edit_offsets;
    mword *var_edit_list;

    mword dev_ctr;
    mword dev_break;
    jmp_buf *dev_jmp;

} backtrack_state;

int  cmp_abs_int(const void *a, const void *b);
int  backtrack_solve(babel_env *be, backtrack_state *bs);
int  backtrack_solve_r(babel_env *be, backtrack_state *bs, int start_var);
int  backtrack_solve_it(babel_env *be, backtrack_state *bs);
int  backtrack_solve_it_dpll(babel_env *be, backtrack_state *bs);

void backtrack_init(babel_env *be, backtrack_state *bs);
void backtrack_init_var_array(babel_env *be, backtrack_state *bs);
void backtrack_init_solver_stack(babel_env *be, backtrack_state *bs);
void backtrack_init_clause_array(babel_env *be, backtrack_state *bs);
void backtrack_init_var_clause_map(babel_env *be, backtrack_state *bs);
void backtrack_init_var_prop_clause_map(babel_env *be, backtrack_state *bs);
void backtrack_init_var_prop_var_map(babel_env *be, backtrack_state *bs);
void backtrack_init_var_edit_list(babel_env *be, backtrack_state *bs);


#endif // BACKTRACK_H


// Clayton Bauman 2018

