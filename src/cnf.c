// cnf.c

#include "cnf.h"
#include "babel.h"
#include "introspect.h"
#include "trie.h"
#include "list.h"
#include "mem.h"
#include "pearson.h"
#include "io.h"
#include "bstring.h"
#include "array.h"


// var != 0
//
int cnf_var_unsat(st_state *bs, int curr_var){

    mword *clause_array = bs->clause_array;
    mword *clause_indices = rdp(bs->var_clause_map, abs(curr_var));
    mword num_clause_indices = size(clause_indices);

    int i;
    int unsat = 0;

    for(i=0; i<num_clause_indices; i++){
        if( cnf_clause_unsat(bs, rdp(clause_array, clause_indices[i])) ){
            unsat=1;
            break;
        }
    }

    return unsat;

}


// caller must ensure var_id > 0
//
var_state cnf_var_read(st_state *bs, int var_id){

    return (var_state)array8_read( bs->var_array, var_id );

}


// caller must ensure var_id > 0
//
int cnf_var_assigned(st_state *bs, int var_id){

    return (cnf_var_read(bs, var_id) != UNASSIGNED_VS);

}


// caller must ensure var_id > 0
//
void cnf_var_write(st_state *bs, int var_id, var_state vs){

    array8_write( bs->var_array, var_id, (uint8_t)vs );

}


// caller must ensure var_id > 0
//
void cnf_var_negate(st_state *bs, int var_id){

    var_state vs = (uint8_t)cnf_var_read(bs, var_id);

    switch(vs){
        case DEC_ASSIGN0_VS:
            cnf_var_write(bs, var_id, DEC_ASSIGN1_VS);
            break;
        case DEC_ASSIGN1_VS:
            cnf_var_write(bs, var_id, DEC_ASSIGN0_VS);
            break;
        case IMP_ASSIGN0_VS:
            cnf_var_write(bs, var_id, IMP_ASSIGN1_VS);
            break;
        case IMP_ASSIGN1_VS:
            cnf_var_write(bs, var_id, IMP_ASSIGN0_VS);
            break;
        case UNASSIGNED_VS:
            break;
        case NEGATE_VS:
            break;
        default:
            _pigs_fly;
    }

}


// caller must ensure var_id > 0
//
int cnf_var_assign(st_state *bs, int var_id, var_state vs){

    mword  base_offset    = rdv(bs->var_edit_offsets, var_id);

    mword *clause_list   = rdp(bs->var_prop_clause_map, var_id);
    mword *var_list      = rdp(bs->var_prop_var_map, var_id);
    mword  var_list_size = size(var_list);

    // decision variable
    var_state old_vs = (uint8_t)cnf_var_read(bs, var_id);
    array8_write( bs->var_edit_list, rdv(bs->var_edit_offsets,var_id), old_vs );
    if(vs == NEGATE_VS){
        cnf_var_negate(bs, var_id);
    }
    else{
        cnf_var_write(bs, var_id, vs);
    }

    // implied variables
    mword  this_var;
    clause_prop cp;

    int i;
    for(i=0; i<var_list_size; i++){
        this_var = rdv(var_list,i);
        array8_write( bs->var_edit_list, base_offset+i+1, (uint8_t)cnf_var_read(bs, this_var) );
    }

    for(i=0; i<var_list_size; i++){

        cp = cnf_clause_propagate(bs, rdv(clause_list, i));
        this_var = rdv(var_list,i);

        if(cp == CONFLICT_CP){
            cnf_var_unassign(bs, var_id);
            return 0;
        }
        else if(cp == ASSIGN0_CP){
            cnf_var_write(bs, this_var, IMP_ASSIGN0_VS);
        }
        else if(cp == ASSIGN1_CP){
            cnf_var_write(bs, this_var, IMP_ASSIGN1_VS);
        }

    }

    return 1;

}


// caller must ensure var_id > 0
//
void cnf_var_unassign(st_state *bs, int var_id){

    mword  base_offset    = rdv(bs->var_edit_offsets, var_id);
    mword *var_list      = rdp(bs->var_prop_var_map, var_id);
    mword  var_list_size = size(var_list);
    var_state vs;

    // implied variables
    mword  this_var;

    int i;
    for(i=0; i<var_list_size; i++){
        this_var = rdv(var_list,i);
        vs = (clause_prop)array8_read( bs->var_edit_list, base_offset+i+1);
        cnf_var_write(bs, this_var, vs);
    }

    // decision variable
    vs = (var_state)array8_read( bs->var_edit_list, rdv(bs->var_edit_offsets,var_id)  );
    cnf_var_write(bs, var_id, vs);

}


// caller must ensure vs != UNASSIGNED_VS
// converts a var_state to a polarity (-1 or 1)
//
int cnf_var_vs_to_polar(var_state vs){

    if(vs == IMP_ASSIGN0_VS || vs == DEC_ASSIGN0_VS)
        return -1;
    else
        return 1;

}


// caller must ensure vs != UNASSIGNED_VS
// converts a var_state to a polarity (-1 or 1)
//
int cnf_polar_eq(int a, int b){

    int a_pos = (a > 0);
    int b_pos = (b > 0);

    return !(a_pos ^ b_pos);

}


// caller must ensure var_id > 0 && constant != 0
//
int cnf_var_eq(st_state *bs, int var_id, int constant){

    int read_var = cnf_var_vs_to_polar( cnf_var_read(bs, var_id) );

    if(    (read_var < 0) && (constant < 0)
        && (read_var > 0) && (constant > 0))
        return 1;
    else
        return 0;

}


//
//
int cnf_clause_all_sat(st_state *bs){

    mword num_clauses = bs->cl->num_clauses;
    int i;

    for(i=0; i<num_clauses; i++){
        if(!cnf_clause_sat(bs, rdp(bs->clause_array,i))){
            return 0;
        }
    }

    return 1;

}


// FIXME: We can combine the common code in the clause_sat/clause_unsat 
// functions together by calculating assignedness and satisfaction separately

// Returns 1 if all variables are assigned and clause is unsatisfied
// Otherwise, returns 0
//
int cnf_clause_unsat(st_state *bs, mword *clause){

    mword clause_size = size(clause);

    int i;
    int var, var_id;
    int sat = 0;
    var_state read_var;

    for(i=0; i<clause_size; i++){

        var = (int)rdv(clause,i);
        var_id = abs(var);

        read_var = cnf_var_read(bs, var_id);

        if(read_var == UNASSIGNED_VS)
            return 0;

        if(cnf_polar_eq(var, cnf_var_vs_to_polar(read_var)))
            sat = 1;

    }

    return (1 - sat);

}


// Returns 1 if all variables are assigned and clause is satisfied
// Otherwise, returns 0
//
int cnf_clause_sat(st_state *bs, mword *clause){

    mword clause_size = size(clause);

    int i;
    int var, var_id;
    int sat = 0;
    var_state read_var;

    for(i=0; i<clause_size; i++){

        var = (int)rdv(clause,i);
        var_id = abs(var);

        read_var = cnf_var_read(bs, var_id);

        if(read_var == UNASSIGNED_VS)
            return 0;

        if(cnf_polar_eq(var, cnf_var_vs_to_polar(read_var)))
            sat = 1;

    }

    return sat;

}


// Returns 1 if any variable in this clause evaluates to true
// Otherwise, returns 0
//
int cnf_clause_sat_lit(st_state *bs, int clause_id, char *candidate_assignment){

//    mword clause_size = size(rdp(bs->clause_array, clause_id));
    mword clause_size   = bs->cl->clause_lengths[clause_id];
    mword clause_offset = bs->cl->clauses[clause_id];

    int i;
    int var, lit, polarity;
    int sat = 0;
    var_state read_var;
   for(i=clause_offset; i<(clause_offset+clause_size); i++){

        var = candidate_assignment[i];
//_dd(var);
        lit = (int)bs->cl->variables[i];
//_dd(lit);
        polarity = (lit > 0);
//_dd(polarity);
//_say("-----");

        if(    (var == DEC_ASSIGN0_VS) && (polarity == 0)
            || (var == DEC_ASSIGN1_VS) && (polarity == 1))
            return 1;

    }

    return 0;

}


//  assumes:
//      all clause variables but last are assigned
//      the last clause-variable is the variable being propagated
//
clause_prop cnf_clause_propagate(st_state *bs, int clause_id){

    mword *clause = rdp(bs->clause_array, clause_id);

    int i;
    int var, var_id;
    var_state read_var;
    int propagating = 1;
    int pseudo_sat = 0;
    mword clause_size;

    if(cnf_clause_unsat(bs, clause)){
        return CONFLICT_CP;
    }
    else{

        clause_size = size(clause);

        for(i=0; i<clause_size-1; i++){

            var = (int)rdv(clause,i);
            var_id = abs(var);

            read_var = cnf_var_read(bs, var_id);

            if(read_var == UNASSIGNED_VS){
                propagating = 0;
                break;
            }

            if(cnf_polar_eq(var, cnf_var_vs_to_polar(read_var)))
                pseudo_sat = 1;

        }

        if(!propagating || pseudo_sat){
            return NO_PROP_CP;
        }
        else{
            var = (int)rdv(clause,clause_size-1);
            if(var < 0){
                return ASSIGN0_CP;
            }
            else{
                return ASSIGN1_CP;
            }

        }

    }

}


// Clayton Bauman 2018

