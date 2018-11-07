// mt.c

#include "mt.h"
#include "babel.h"
#include "pearson.h"
#include "cnf.h"
#include "backtrack.h"
#include "array.h"
#include <math.h>

static mword mt_rand_state[4];

//
//
int mt_srand(int seed){

    mt_rand_state[0] = seed;
    mt_rand_state[1] = seed;

}


//
//
int mt_rand(void){
//_trace;
    static int toggle=0;

    static mword zero_hash[2] = {0, 0};

    mword *key    = (mt_rand_state +    toggle *2);
    mword *result = (mt_rand_state + (1-toggle)*2);

    pearson128(result, zero_hash, (char*)key, HASH_BYTE_SIZE);

    toggle = (toggle+1)%2;

    return rdv(mt_rand_state,(toggle*2));

}


//
//
void mt_rand_assignment(backtrack_state *bs){

    mword num_variables = bs->cl->num_variables;
    int i;
    int rand_val;

    char *var_array = (char*)bs->var_array;

    for(i=1; i<=num_variables; i++){
        rand_val = (mword)mt_rand();
        if(rand_val % 2)
            var_array[i] = DEC_ASSIGN0_VS;
        else
            var_array[i] = DEC_ASSIGN1_VS;
    }

}


// Phase 1 (basic Moser-Tardos alg):
//
int mt_solve(backtrack_state *bs, int max_tries){
//_trace;

    int counter = 0;
    int clause_id;
    int sat_count;
    int best_sat_count = 0;

    bs->clause_sat_count = mt_update_clause_sat(bs);

    mt_rand_assignment(bs);

    while(counter++ < max_tries){

        sat_count = mt_clause_sat_count(bs);

        if(sat_count == bs->cl->num_clauses)
            break;

        best_sat_count = (sat_count > best_sat_count) ? sat_count : best_sat_count;

        clause_id = mt_choose_unsat_clause(bs);

        if(clause_id == -1)
            break;

        mt_clause_kick(bs, clause_id);

    }

    return best_sat_count;

}


//
//
void mt_clause_kick(backtrack_state *bs, int clause_id){

    mword *clause = rdp(bs->clause_array, clause_id);
    mword num_vars = size(clause);
    int this_var;

    int i;

    for(i=0;i<num_vars;i++){

        this_var = abs(rdv(clause,i));

        // jiggle the variable
        if(rand_bent_coin(0.5))
            cnf_var_write(bs, this_var, DEC_ASSIGN1_VS);
        else
            cnf_var_write(bs, this_var, DEC_ASSIGN0_VS);

    }

}


//
//
float mt_logistic(float x){

    return 1.0f / (1.0f + expf(-1*x));

}


// p = probability that rand_bent_coin() returns 1
//
int rand_bent_coin(float p){

    uint32_t rand_val = mt_rand() & 0x7fffffff;
    uint32_t z = 2<<30;
    uint32_t q  = (uint32_t)(p * z);

    return (rand_val < q);

}


// Use this fn to evalute var_id based on clause_sat
//
int mt_var_sat_count_fast(backtrack_state *bs, int var_id){
//_trace;

    mword *clause_indices    = rdp(bs->var_clause_map, var_id);
    mword num_clause_indices = size(clause_indices);
    char *clause_sat = (char*)bs->clause_sat;

    int i;
    int sat_count = 0;

    for(i=0; i<num_clause_indices; i++){
        sat_count += clause_sat[clause_indices[i]];
    }

    return sat_count;

}


// Use this fn to evaluate change to var_id without updating clause_sat
//
int mt_var_sat_count(backtrack_state *bs, int var_id){
//_trace;

    mword *clause_indices    = rdp(bs->var_clause_map, var_id);
    mword num_clause_indices = size(clause_indices);

    int i;
    int sat_count = 0;

    for(i=0; i<num_clause_indices; i++){
        sat_count += cnf_clause_sat(bs, rdp(bs->clause_array, clause_indices[i]));
    }

    return sat_count;

}


//
//
void mt_update_var_sat(backtrack_state *bs, int var_id){
//_trace;

    mword *clause_indices    = rdp(bs->var_clause_map, var_id);
    mword num_clause_indices = size(clause_indices);

    int i;
    for(i=0; i<num_clause_indices; i++){
        cnf_update_clause_sat(bs, clause_indices[i]);
    }

}


// Updates bs->clause_sat with results of cnf_clause_sat()
//
int cnf_update_clause_sat(backtrack_state *bs, int clause_id){
//_trace;

    int sat = cnf_clause_sat(bs, rdp(bs->clause_array, clause_id));

    array8_write(bs->clause_sat, clause_id, sat);

    return sat;

}



//
//
int mt_clause_sat_count(backtrack_state *bs){
//_trace;

    int i;
    int sat_count = 0;

    for(i=0; i<bs->cl->num_clauses; i++){
        if(cnf_clause_sat(bs, rdp(bs->clause_array,i)))
            sat_count++;
    }

    return sat_count;

}


//
//
int mt_update_clause_sat(backtrack_state *bs){
//_trace;

    int i;
    int sat_count = 0;
    char *clause_sat = (char*)bs->clause_sat;

    for(i=0; i<bs->cl->num_clauses; i++){
        if(cnf_clause_sat(bs, rdp(bs->clause_array,i))){
            clause_sat[i] = 1;
            sat_count++;
        }
        else{
            clause_sat[i] = 0;
        }
    }

    return sat_count;

}


// returns -1 if all clauses are SAT
//
mword mt_choose_unsat_clause(backtrack_state *bs){
//_trace;

    int i;
    int clause_id = -1;
    char *clause_sat = (char*)bs->clause_sat;

    mword start_clause = (mt_rand() % bs->cl->num_clauses);

    for(i=0; i<bs->cl->num_clauses; i++){
        if(!clause_sat[(start_clause+i)%bs->cl->num_clauses])
            return (start_clause+i)%bs->cl->num_clauses;
    }

    return clause_id;

}


// Clayton Bauman 2018

