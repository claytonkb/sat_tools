// backtrack.c

#include "backtrack.h"
#include "cnf.h"
#include "introspect.h"
#include "trie.h"
#include "list.h"
#include "mem.h"
#include "pearson.h"
#include "io.h"
#include "bstring.h"
#include "array.h"


//
//
int cmp_abs_int(const void *a, const void *b){
    return ( abs(*(int*)a) - abs(*(int*)b) );
}


//
//
void dev_dump(babel_env *be, mword *bs);
void dev_dump(babel_env *be, mword *bs){
    mword *temp = introspect_gv(be, bs);
    //_say((char*)temp);
    io_spit(be, "work/test.dot", temp, U8_ASIZE, OVERWRITE);
}


// Iterative modified DPLL
//
int backtrack_solve_it_dpll(babel_env *be, backtrack_state *bs){
_trace;



}




// Iterative optimized backtrack
//
int backtrack_solve_it(babel_env *be, backtrack_state *bs){
_trace;
    // 2 continuations: cont_A, cont_B

    int curr_var = 1;
    int result = 0;
    int returning = 0;

    char *solver_stack = (char*)bs->solver_stack;
    char *mt_var_array = (char*)bs->mt_var_array;
    solver_cont sc;

    while(1){
bs->dev_ctr++;
if((bs->dev_ctr % 20000) == 0)
    _prn(".");

        if(curr_var == 1 && result == 1) // SAT
             break;

        if(returning){
             returning = 0;
             sc = solver_stack[curr_var];
             switch(sc){
                 case CONT_A_SC:
                     goto cont_A;
                 case CONT_B_SC:
                     goto cont_B;
             }
        }

        if(curr_var > bs->cl->num_variables){
            if(cnf_clause_all_sat(bs)){
                result = 1;
                returning = 1;
            }
            curr_var--;
            continue;
        }

//        cnf_var_write(bs, curr_var, DEC_ASSIGN1_VS);
        cnf_var_write(bs, curr_var, mt_var_array[curr_var]);

        if(!cnf_var_unsat(bs, curr_var)){
             solver_stack[curr_var] = CONT_A_SC;
             curr_var++;
             continue;
        }

cont_A:

        if(result == 1){
             returning = 1;
             curr_var--;
             continue;
         }
         // else 
//        cnf_var_negate(bs, curr_var);
        if(mt_var_array[curr_var] == DEC_ASSIGN0_VS)
            cnf_var_write(bs, curr_var, DEC_ASSIGN1_VS);
        else
            cnf_var_write(bs, curr_var, DEC_ASSIGN0_VS);

        if(cnf_var_unsat(bs, curr_var)){
            result = 0;
            goto cont_B;
        }
        else{
            solver_stack[curr_var] = CONT_B_SC; 
            curr_var++;
            continue;
        }

cont_B:

        if(result == 0){
            if(curr_var == 1) // UNSAT
                break;
            cnf_var_write(bs, curr_var, UNASSIGNED_VS);
        }

        returning = 1;
        curr_var--;
        continue;

    }

    return result;

}


//
//
int backtrack_solve(babel_env *be, backtrack_state *bs){

    return backtrack_solve_r(be, bs, 1);

}


// recursive DPLL
//
int backtrack_solve_r(babel_env *be, backtrack_state *bs, int curr_var){


}


//
//
void backtrack_init(babel_env *be, backtrack_state *bs){

    clause_list *cl = bs->cl;

    bs->curr_var         = 0;
    bs->dev_ctr          = 0;
    bs->dev_break        = 0;
    bs->clause_sat_count = 0;

//    bs->clause_sat = mem_new_bits(be, cl->num_clauses);
    bs->clause_sat = mem_new_str(be, bs->cl->num_clauses, '\0');

    backtrack_init_var_array(be, bs);
    backtrack_init_solver_stack(be, bs);
    backtrack_init_clause_array(be, bs);
    backtrack_init_var_clause_map(be, bs);
    backtrack_init_var_prop_clause_map(be, bs);
    backtrack_init_var_prop_var_map(be, bs);
    backtrack_init_var_edit_list(be, bs);

}


//
//
void backtrack_init_var_array(babel_env *be, backtrack_state *bs){

    // Note: UNASSIGNED_VS must be first element of var_state enum declaration
    // to ensure that it is assigned integer value 0
    bs->var_array = mem_new_str(be, bs->cl->num_variables+1, '\0');

}



//
//
void backtrack_init_solver_stack(babel_env *be, backtrack_state *bs){

    bs->solver_stack = mem_new_str(be, bs->cl->num_variables+1, '\0');

}



//
//
void backtrack_init_clause_array(babel_env *be, backtrack_state *bs){

    clause_list *cl = bs->cl;

    int i,j,k;
    int last_clause_index=0;
    int clause_i;

    mword *clause_array = (mword*)mem_new_ptr(be, cl->num_clauses);
    mword *curr_clause;
    mword clause_size;

    for(i=1; i<cl->num_clauses; i++){

        clause_i = cl->clauses[i];
        clause_size = clause_i - last_clause_index;
        curr_clause = (mword*)mem_new_val(be, clause_size, 0);

        k=0;
        for(j=last_clause_index; j<clause_i; j++){
            ldv(curr_clause,k) = cl->variables[j];
            k++;
        }

        last_clause_index = cl->clauses[i];

        ldp(clause_array,i-1) = curr_clause;

    }

    // final clause
    clause_i = UNITS_8TOM(sfield(cl->variables));
    clause_size = clause_i - last_clause_index;
    curr_clause = (mword*)mem_new_val(be, clause_size, 0);

    k=0;
    for(j=last_clause_index; j<clause_i; j++){
        ldv(curr_clause,k) = cl->variables[j];
        k++;
    }

    ldp(clause_array,i-1) = curr_clause;

    // sort each element of clause_array
    for(i=1; i<cl->num_clauses; i++){
        qsort(rdp(clause_array,i), size(rdp(clause_array,i)), sizeof(mword), cmp_abs_int);
    }

    bs->clause_array = clause_array;

}


//
//
void backtrack_init_var_clause_map(babel_env *be, backtrack_state *bs){

    int i,j;
    clause_list *cl = bs->cl;

    mword *clause_array = bs->clause_array;

    mword *clause_trie = trie_new(be);
    mword clause_array_size = size(clause_array);
    mword curr_clause_size;

    mword *trie_entry;
    mword *hash_key = mem_new_val(be,HASH_SIZE,0);
    mword *new_list;
    mword *curr_index = mem_new_val(be, 2, 0);
    mword *curr_clause;

    for(i=0;i<clause_array_size;i++){

        curr_clause = rdp(clause_array,i);
        curr_clause_size = size(curr_clause);

        for(j=0;j<curr_clause_size;j++){

            ldv(curr_index,0) = abs(rdv(curr_clause,j));
            pearson128(hash_key, be->zero_hash, (char*)curr_index, MWORD_SIZE);

            trie_entry = trie_lookup_hash(be, clause_trie, hash_key, be->nil);

            if(!is_nil(trie_entry))
                trie_entry = trie_entry_get_payload(be, trie_entry);

            new_list = list_cons(be, _val(be, i), be->nil);
            list_push(be, trie_entry, new_list);
            trie_insert(be, clause_trie, hash_key, be->nil, new_list);

        }

    }

    mword *var_clause_map    = mem_new_ptr(be, cl->num_variables+1);

    for(i=1; i <= cl->num_variables; i++){

        ldv(curr_index,0) = i;
        pearson128(hash_key, be->zero_hash, (char*)curr_index, MWORD_SIZE);
        trie_entry = trie_lookup_hash(be, clause_trie, hash_key, be->nil);

        if(!is_nil(trie_entry))
            trie_entry = trie_entry_get_payload(be, trie_entry);

        ldp(var_clause_map,i) = list_to_val_array(be, trie_entry);

    }

    bs->var_clause_map = var_clause_map;

}


//
//
void backtrack_init_var_prop_clause_map(babel_env *be, backtrack_state *bs){

    clause_list *cl = bs->cl;

    mword *var_prop_clause_map = mem_new_ptr(be, cl->num_variables+1);
    mword *var_propagate_entry;
    mword *clause;
    mword  prop_var;

    mword *new_list;

    int i,j,k;

    mword *clause_array = bs->clause_array;
    mword clause_size;

    for(i=0; i < cl->num_clauses; i++){

        new_list = list_cons(be, _val(be, i), be->nil);

        clause = rdp(clause_array, i);
        clause_size = size(clause);

        if(clause_size == 1){
            prop_var = abs(rdv(clause,0));
        }
        else{
            prop_var = abs(rdv(clause,clause_size-2));
        }

        var_propagate_entry = rdp(var_prop_clause_map, prop_var);

        list_push(be, var_propagate_entry, new_list);

        ldp(var_prop_clause_map, prop_var) = new_list;

    }

    // XXX
    // We can trim the var_prop_clause_map in the following way:
    //
    //  for each variable 1..num_variables
    //      mark var_assigned[variable]
    //      for each clause in var_prop_clause_map[i]
    //          are all but last variable marked as assigned in var_assigned?
    //          if not: eliminate this clause from the propagate_clause list
    //
    // once a variable is propagated, the clause associated with that variable
    //      must be satisfied (otherwise, we will have backtracked)
    //      therefore, we mark this clause as satisfied and perform no further
    //      sat checks involving it. Does this matter?

    mword *head;
    mword *curr;
    mword *tail;
    mword all_assigned;
    mword curr_clause_id;
    
    for(i=1; i <= cl->num_variables; i++){

        cnf_var_write(bs, i, DEC_ASSIGN0_VS);

        curr = rdp(var_prop_clause_map, i);
        head = be->nil;
        tail = head;

        while(!is_nil(curr)){

            curr_clause_id = vcar(pcar(curr));

            clause = rdp(clause_array, curr_clause_id);
            clause_size = size(clause);

            all_assigned = 1;
            for(j=0; j<clause_size-1; j++){
                if(!cnf_var_assigned(bs, abs(rdv(clause,j)))){
                    all_assigned = 0;
                    break;
                }
            }

            if(all_assigned){
                if(is_nil(head)){
                    head = list_cons(be, pcar(curr), be->nil);
                    tail = head;
                }
                else{
                    ldp(tail,1) = list_cons(be, pcar(curr), be->nil);
                    tail = pcdr(tail);
                }
            }

            curr = pcdr(curr);

        }

        // write head back to var_
        ldp(var_prop_clause_map, i) = head;

    }

    for(i=1; i <= cl->num_variables; i++)
        cnf_var_write(bs, i, UNASSIGNED_VS);

    mword *temp;
    for(i=1; i <= cl->num_variables; i++){
        var_propagate_entry = rdp(var_prop_clause_map, i);
        if(is_nil(var_propagate_entry))
            continue;
        temp = list_to_val_array(be, var_propagate_entry);
        ldp(var_prop_clause_map,i) = temp;
    }

    bs->var_prop_clause_map = var_prop_clause_map;

}


//
//
void backtrack_init_var_prop_var_map(babel_env *be, backtrack_state *bs){

    clause_list *cl = bs->cl;

    int i,j;

    mword *var_prop_var_map = mem_new_ptr(be, cl->num_variables+1);
    mword *clause;
    mword  clause_size;
    mword *clause_list;
    mword  clause_list_size;
    mword  prop_var;
    mword *prop_var_list;

    for(i=1; i <= cl->num_variables; i++){

        clause_list = rdp(bs->var_prop_clause_map,i);

        if(is_nil(clause_list))
            continue;

        clause_list_size = size(clause_list);

        prop_var_list = mem_new_val(be, clause_list_size, 0);

        for(j=0; j<clause_list_size; j++){

            clause = rdp(bs->clause_array, rdv(clause_list, j));
            clause_size = size(clause);

            prop_var = rdv(clause,clause_size-1);

            ldv(prop_var_list,j) = abs(prop_var);

        }

        ldp(var_prop_var_map,i) = prop_var_list;

    }

    bs->var_prop_var_map = var_prop_var_map;

}


//
//
void backtrack_init_var_edit_list(babel_env *be, backtrack_state *bs){

    int i,j;

    mword *var_list;
    mword  var_list_size;
    mword  curr_offset = 0;

    mword *var_edit_offsets = mem_new_val(be, bs->cl->num_variables+1, 0);

    // zero out all entries
    for(i=1; i <= bs->cl->num_variables; i++){

        ldv(var_edit_offsets, i) = curr_offset;

        var_list = rdp(bs->var_prop_var_map, i);

        if(is_nil(var_list))
            var_list_size = 0;
        else
            var_list_size = size(var_list);

        curr_offset += (var_list_size+1);

    }

    bs->var_edit_offsets = var_edit_offsets;
    bs->var_edit_list    = mem_new_str(be, curr_offset, '\0');

}


// Clayton Bauman 2018

