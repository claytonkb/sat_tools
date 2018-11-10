// sat_tools.c

#include "sat_tools.h"
#include "cnf.h"
#include "introspect.h"
#include "trie.h"
#include "list.h"
#include "mem.h"
#include "pearson.h"
#include "io.h"
#include "bstring.h"
#include "bstruct.h"
#include "array.h"
#include <math.h>


//
//
int cmp_size(const void *a, const void *b){
    return ( size(*(mword*)a) - size(*(mword*)b) );
}


//
//
int cmp_abs_int(const void *a, const void *b){
    return ( abs(*(int*)a) - abs(*(int*)b) );
}


//
//
void st_init(babel_env *be, st_state *bs){

    // clause_list               --> [init_clause_array]          --> raw_clause_array
    //
    // raw_clause_array          --> [init_var_clause_map]        --> raw_var_clause_map
    //
    // raw_clause_array       --+
    // raw_var_clause_map     --+--> [init_reorder_clause_array]  --> reordered_clause_array
    //
    // raw_var_clause_map     --+
    // reordered_clause_array --+--> [init_permute_variables] --+--> clause_array
    //                                                          +--> var_clause_map

    clause_list *cl = bs->cl;

    bs->curr_var         = 0;
    bs->dev_ctr          = 0;
    bs->dev_break        = 0;
    bs->clause_sat_count = 0;

    bs->clause_sat = mem_new_str(be, bs->cl->num_clauses, '\0');

    st_init_var_array(be, bs);
    st_init_solver_stack(be, bs);
    st_init_clause_array(be, bs);
    st_init_var_clause_map(be, bs);
    st_init_weights(be, bs);
    st_init_reorder_clause_array(be, bs);
    st_init_permute_variables(be, bs);
    st_init_var_prop_clause_map(be, bs);
    st_init_var_prop_var_map(be, bs);
    st_init_var_edit_list(be, bs);

}


//
//
void st_init_var_array(babel_env *be, st_state *bs){

    // Note: UNASSIGNED_VS must be first element of var_state enum declaration
    // to ensure that it is assigned integer value 0
    bs->var_array = mem_new_str(be, bs->cl->num_variables+1, '\0');

}



//
//
void st_init_solver_stack(babel_env *be, st_state *bs){

    bs->solver_stack = mem_new_str(be, bs->cl->num_variables+1, '\0');

}



//
//
void st_init_clause_array(babel_env *be, st_state *bs){

    clause_list *cl = bs->cl;

    int i,j,k;
    int last_clause_index=0;
    int clause_i;

    mword *raw_clause_array = (mword*)mem_new_ptr(be, cl->num_clauses);
    mword *curr_clause;
    mword  clause_size;

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

        ldp(raw_clause_array,i-1) = curr_clause;

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

    ldp(raw_clause_array,i-1) = curr_clause;

    // sort each element of raw_clause_array
    for(i=0; i<cl->num_clauses; i++){
        qsort(rdp(raw_clause_array,i), size(rdp(raw_clause_array,i)), sizeof(mword), cmp_abs_int);
    }

    bs->raw_clause_array = raw_clause_array;

}


//
//
void st_init_var_clause_map(babel_env *be, st_state *bs){

    int i,j;
    clause_list *cl = bs->cl;

    mword *raw_clause_array = bs->raw_clause_array;

    mword *clause_trie = trie_new(be);
    mword clause_array_size = size(raw_clause_array);
    mword curr_clause_size;

    mword *trie_entry;
    mword *hash_key = mem_new_val(be,HASH_SIZE,0);
    mword *new_list;
    mword *curr_index = mem_new_val(be, 2, 0);
    mword *curr_clause;

    for(i=0;i<clause_array_size;i++){

        curr_clause = rdp(raw_clause_array,i);
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

    mword *raw_var_clause_map = mem_new_ptr(be, cl->num_variables+1);

    for(i=1; i <= cl->num_variables; i++){

        ldv(curr_index,0) = i;
        pearson128(hash_key, be->zero_hash, (char*)curr_index, MWORD_SIZE);
        trie_entry = trie_lookup_hash(be, clause_trie, hash_key, be->nil);

        if(!is_nil(trie_entry))
            trie_entry = trie_entry_get_payload(be, trie_entry);

        ldp(raw_var_clause_map,i) = list_to_val_array(be, trie_entry);

    }

    bs->raw_var_clause_map = raw_var_clause_map;

}


//
//
void st_init_weights(babel_env *be, st_state *bs){

    double sum_var_weight;
    double var_weight;
    double clause_weight;
    int num_lit_occ;
    int var_id;

    mword *var_lit_weights = mem_new_val(be, bs->cl->num_variables+1, 0);

    int i,j;
    for(i=1; i <= bs->cl->num_variables; i++){
        num_lit_occ = size(rdp(bs->raw_var_clause_map,i));
        var_weight = (double)num_lit_occ / bs->cl->num_assignments;
        *((double*)var_lit_weights+i) = var_weight;
//        _dd(i);
//        _df(var_weight);
    }

    mword *clause_weights = mem_new_val(be, bs->cl->num_clauses, 0);
    mword *clause;
    mword  clause_size;

    for(i=0; i<bs->cl->num_clauses; i++){

        clause = rdp(bs->raw_clause_array,i);
        clause_size = size(clause);
        sum_var_weight = 0;

        // sum var_weights in this clause
        for(j=0; j < clause_size; j++){
            var_id = abs(rdv(clause,j));
            var_weight = *((double*)var_lit_weights+var_id);
            sum_var_weight += var_weight;
        }
//        _df(sum_var_weight);
//        printf("%lf\n", sum_var_weight);
//        clause_weight = (double)expf(-1 * clause_size) ;
        clause_weight = (double)expf((-1.0f * clause_size)) * sum_var_weight;
//        _df(clause_weight);
        *((double*)clause_weights+i) = clause_weight;

    }

}


//
//
void st_init_reorder_clause_array(babel_env *be, st_state *bs){

    // FIXME:
    //      sort by score, not clause-length
    //      score is calculated as:
    //          (2^-clause_length) * (SUM [var_weights in this clause])
    mword *sort_clause_array = bstruct_cp(be, bs->raw_clause_array);
//    qsort(sort_clause_array, size(sort_clause_array), sizeof(mword), cmp_size);

    int i,j;

    mword *bfs_clause_array     = mem_new_ptr(be, size(sort_clause_array));
    mword *reorder_clause_array = mem_new_ptr(be, size(sort_clause_array));

    for(i=0; i<bs->cl->num_clauses; i++){
        ldp(bfs_clause_array, i) = rdp(sort_clause_array, i);
    }

    mword *clause_queue = st_queue_new(be);

    // put clause_id 0 into clause_queue
    // XXX NOTE: Think about inserting top K clause_id's into the clause queue 
    //      to ensure that the BFS has a certain degree of "spread" throughout 
    //      the graph.
    st_enqueue(be, clause_queue, _val(be, 0));

    ldp(bfs_clause_array, 0) = be->nil;

    mword *curr_clause;
    mword  curr_clause_id;
    mword *clauses;
    mword  num_clause_vars;
    mword  num_clauses;
    mword  clause_id;
    mword  reorder_clause_id = 0;
    mword *reorder_clause_id_map = mem_new_val(be, bs->cl->num_clauses, 0);

    int counter = 0;
    int depth;

    while( st_queue_depth(clause_queue) != 0 ){

        curr_clause_id = rdv( st_dequeue(be, clause_queue), 0 );

        curr_clause = rdp(bs->raw_clause_array, curr_clause_id);
        ldp(bfs_clause_array, curr_clause_id) = be->nil;
        num_clause_vars = size(curr_clause);

        ldv(reorder_clause_id_map, curr_clause_id) = reorder_clause_id; // used to build reorder_var_clause_map
        ldp(reorder_clause_array,  reorder_clause_id) = curr_clause;
        reorder_clause_id++;

        // for each var_id in curr_clause:
        for(i=0; i<num_clause_vars; i++){

            clauses = rdp(bs->raw_var_clause_map, abs(rdv(curr_clause,i)));
            num_clauses = size(clauses);

            // for each clause containing var_id:
            for(j=0; j<num_clauses; j++){

                clause_id = rdv(clauses,j);

                if(is_nil(rdp(bfs_clause_array,clause_id))){
                    continue;
                }
                else{
                    st_enqueue(be, clause_queue, _val(be, clause_id)); // place this clause_id into clause_queue
                    ldp(bfs_clause_array,clause_id) = be->nil;
                }

            }

        }

    }

    bs->reorder_clause_array = reorder_clause_array;

    mword *reorder_var_clause_map = mem_new_ptr(be, bs->cl->num_variables+1);
    mword *new_clauses;

    for(i=1; i <= bs->cl->num_variables; i++){
        clauses = rdp(bs->raw_var_clause_map, i);
        num_clauses = size(clauses);
        if(num_clauses == 0)
            continue;
        new_clauses = mem_new_val(be, num_clauses, 0);
        for(j=0; j<num_clauses; j++){
            ldv(new_clauses, j) = rdv(reorder_clause_id_map, rdv(clauses,j));
        }
        ldp(reorder_var_clause_map, i) = new_clauses;
    }

    bs->reorder_var_clause_map = reorder_var_clause_map;
    bs->reorder_clause_id_map = reorder_clause_id_map;

}


// precondition: need to have var_clause_map, based on reorder_clause_array
// postcondition: Will need to generate fresh var_clause_map based on reorder_var_array
//
void st_init_permute_variables(babel_env *be, st_state *bs){

    mword *permute_var_array = mem_new_val(be, bs->cl->num_variables+1, 0);
    mword curr_var=1;

    mword *clause;
    mword  num_vars;
    mword  var_id;

    int i,j;
    for(i=0; i<bs->cl->num_clauses; i++){

        clause = rdp(bs->raw_clause_array, i);
        num_vars = size(clause);

        // for each var_id in clause:
        for(j=0; j<num_vars; j++){
            var_id = abs((int)clause[j]);
            if(rdv(permute_var_array, var_id)){
                continue;
            }
            else{
                ldv(permute_var_array, var_id) = curr_var;
                curr_var++;
            }
        }

    }

    bs->permute_var_array = permute_var_array;

    mword *unpermute_var_array = mem_new_val(be, bs->cl->num_variables+1, 0);

    for(i=1; i<=bs->cl->num_variables; i++){
        var_id = rdv(permute_var_array, i);
        ldv(unpermute_var_array, var_id) = i;
    }

    bs->unpermute_var_array = unpermute_var_array;

    mword  new_var_id;
    mword *new_clause;
    mword *clause_array = mem_new_ptr(be,bs->cl->num_clauses);

    // permute each variable while copying raw_clause_array --> clause_array
    for(i=0; i<bs->cl->num_clauses; i++){

        clause = rdp(bs->raw_clause_array, i);
        num_vars = size(clause);

        new_clause = mem_new_val(be, num_vars, 0);

        // for each var_id in clause:
        for(j=0; j<num_vars; j++){

            new_var_id = permute_var_array[abs((int)clause[j])];

            if((int)clause[j] < 0)
                new_clause[j] = -1*new_var_id;
            else
                new_clause[j] = new_var_id;
        }

        ldp(clause_array,i) = new_clause;

    }

    mword *var_clause_map = mem_new_ptr(be,bs->cl->num_variables+1);

    // copy raw_var_clause_map to var_clause_map, re-arranging entries by
    // permute_var_array while copying
    for(i=1; i<bs->cl->num_variables+1; i++){
        // old     --> new
        // index 4 --> index 1
        new_var_id = permute_var_array[i];
        ldp(var_clause_map, new_var_id) = rdp(bs->raw_var_clause_map, i);
    }

    bs->dev_reorder_clause_array    = clause_array;
    bs->dev_permute_var_clause_map = var_clause_map;

    bs->clause_array   = bs->raw_clause_array;
    bs->var_clause_map = bs->raw_var_clause_map;

//    bs->clause_array = clause_array;
//    bs->var_clause_map = var_clause_map;

//bs->dev_ptr = permute_var_array;
//longjmp(*bs->dev_jmp, 0);

}


//
//
void st_init_var_prop_clause_map(babel_env *be, st_state *bs){

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
void st_init_var_prop_var_map(babel_env *be, st_state *bs){

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
void st_init_var_edit_list(babel_env *be, st_state *bs){

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


///////////////////////////////////////////////////////////////////////////


// queue: [ptr queue_head queue_tail depth]
//
mword *st_queue_new(babel_env *be){

    mword *queue = mem_new_ptr(be, 3);
    ldp(queue,2) = _val(be, 0);

    return queue;

}


// queue: [ptr queue_head queue_tail depth]
//
void st_enqueue(babel_env *be, mword *queue, mword *payload){

    mword *depth = rdp(queue,2);

    // cons the payload
    mword *curr_tail = rdp(queue,1);
    mword *new_tail = list_cons(be, payload, be->nil);

    if(*depth == 0){
        ldp(queue,0) = new_tail;
        ldp(queue,1) = new_tail;
    }
    else{
        ldp(curr_tail,1) = new_tail;
        ldp(queue,1) = new_tail;
    }

    ldv(depth,0) = rdv(depth,0)+1;

}


// queue: [ptr queue_head queue_tail depth]
//
mword *st_dequeue(babel_env *be, mword *queue){

    mword *depth = rdp(queue,2);

    if(*depth == 0)
        return be->nil;

    mword *curr_head = rdp(queue,0);

    if(*depth == 1){
        ldp(queue,0) = be->nil;
        ldp(queue,1) = be->nil;
    }
    else{
        ldp(queue,0) = pcdr(curr_head);
    }

    ldv(depth,0) = rdv(depth,0)-1;

    return pcar(curr_head);

}


//
//
mword st_queue_depth(mword *queue){

    return vcar(rdp(queue,2));

}



// Clayton Bauman 2018

