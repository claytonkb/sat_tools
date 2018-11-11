// sat_tools.h
//

#include "cnf_parse.h"
#include "babel.h"

#ifndef BACKTRACK_H
#define BACKTRACK_H

typedef enum clause_prop_enum 
    { CONFLICT_CP, 
      NO_PROP_CP,  PROP_CP,
      ASSIGN0_CP,  ASSIGN1_CP, } clause_prop;

typedef enum var_state_enum 
    { UNASSIGNED_VS,  NEGATE_VS,
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

    mword *clause_array;        // stores each clause (its variables)
    mword *clause_trie;         // temporary data-structure used to construct var_clause_map

    // XXX NOTE XXX 
    //      all var* elements are 1-indexed; the 0-index is ignored/MBZ/nil
    mword *var_array;           // stores the current assignment
    mword *var_clause_map;      // maps each variable to the clause_id's in which it appears
    mword *var_prop_clause_map; // maps each variable to only those clause_id's in which it appears in propagating position
    mword *var_prop_var_map;    // maps each variable to the variables it can propagate

    mword *var_lit_weights;      // lit_weight = occ_lit / occ_total
    mword *clause_weights;       // clause_weight = 2^-length * (SUMi weight(lit_i))

    mword *raw_clause_array;     // stores each clause (its variables)
    mword *raw_var_clause_map;   // maps each variable to the clause_id's in which it appears

    mword *reorder_clause_array; // clause_array, sorted by clause-score
    mword *reorder_var_clause_map; 
    mword *reorder_clause_id_map; 
    mword *permute_var_array;    // breadth-first-traversal of graph, based on reorder_clause_array
    mword *unpermute_var_array;

    mword *clause_sat;
    mword  clause_sat_count;

    mword *dev_reorder_clause_array;
    mword *dev_permute_var_clause_map;

    mword *var_edit_offsets;    // used by cnf_var_assign() and "_unassign()
    mword *var_edit_list;       // "        "           "

    mword *lit_pos_clause_map; // maps each positive literal-occurrence in cl->variables to its clause
    mword *lit_neg_clause_map; // maps each negative literal-occurrence in cl->variables to its clause

    mword dev_ctr;
    mword dev_break;
    mword *dev_ptr;
    jmp_buf *dev_jmp;

} st_state;

int  cmp_abs_int(const void *a, const void *b);
int cmp_size(const void *a, const void *b);

//int  st_solve(babel_env *be, st_state *bs);

void st_init(babel_env *be, st_state *bs);
void st_init_var_array(babel_env *be, st_state *bs);
void st_init_solver_stack(babel_env *be, st_state *bs);
void st_init_clause_array(babel_env *be, st_state *bs);
void st_init_var_clause_map(babel_env *be, st_state *bs);
void st_init_weights(babel_env *be, st_state *bs);
void st_init_reorder_clause_array(babel_env *be, st_state *bs);
void st_init_permute_variables(babel_env *be, st_state *bs);
void st_init_var_prop_clause_map(babel_env *be, st_state *bs);
void st_init_var_prop_var_map(babel_env *be, st_state *bs);
void st_init_var_edit_list(babel_env *be, st_state *bs);

mword *st_queue_new(babel_env *be);
void   st_enqueue(babel_env *be, mword *queue, mword *payload);
mword *st_dequeue(babel_env *be, mword *queue);
mword  st_queue_depth(mword *queue);


#endif // BACKTRACK_H


// Clayton Bauman 2018

