// sls.c

#include "sls.h"
#include "babel.h"
#include "pearson.h"
#include "cnf.h"
//#include "backtrack.h"
#include "array.h"
#include <math.h>

static mword sls_mt_rand_state[4];

//
//
int sls_mt_srand(int seed){

    sls_mt_rand_state[0] = seed;
    sls_mt_rand_state[1] = seed;

}


//
//
int sls_mt_rand(void){
//_trace;
    static int toggle=0;

    static mword zero_hash[2] = {0, 0};

    mword *key    = (sls_mt_rand_state +    toggle *2);
    mword *result = (sls_mt_rand_state + (1-toggle)*2);

    pearson128(result, zero_hash, (char*)key, HASH_BYTE_SIZE);

    toggle = (toggle+1)%2;

    return rdv(sls_mt_rand_state,(toggle*2));

}


//
//
void sls_mt_rand_assignment(st_state *bs){

    mword num_variables = bs->cl->num_variables;
    int i;
    int rand_val;

    char *var_array = (char*)bs->var_array;

    for(i=1; i<=num_variables; i++){
        rand_val = (mword)sls_mt_rand();
        if(rand_val % 2)
            var_array[i] = DEC_ASSIGN0_VS;
        else
            var_array[i] = DEC_ASSIGN1_VS;
    }

}


// Phase 1 (basic Moser-Tardos alg):
//
int sls_mt_solve(st_state *bs, int max_tries){
//_trace;

    int counter = 0;
    int clause_id;
    int sat_count;
    int best_sat_count = 0;

    bs->clause_sat_count = sls_mt_update_clause_sat(bs);

    sls_mt_rand_assignment(bs);

    while(counter++ < max_tries){

        sat_count = sls_mt_clause_sat_count(bs);

        if(sat_count == bs->cl->num_clauses)
            break;

        best_sat_count = (sat_count > best_sat_count) ? sat_count : best_sat_count;

        clause_id = sls_mt_choose_unsat_clause(bs);

        if(clause_id == -1)
            break;

        sls_mt_clause_kick(bs, clause_id);

    }

    return best_sat_count;

}


//
//
void sls_mt_clause_kick(st_state *bs, int clause_id){

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
float sls_mt_logistic(float x){

    return 1.0f / (1.0f + expf(-1*x));

}


// p = probability that rand_bent_coin() returns 1
//
int rand_bent_coin(float p){

    uint32_t rand_val = sls_mt_rand() & 0x7fffffff;
    uint32_t z = 2<<30;
    uint32_t q  = (uint32_t)(p * z);

    return (rand_val < q);

}


// Use this fn to evalute var_id based on clause_sat
//
int sls_mt_var_sat_count_fast(st_state *bs, int var_id){
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
int sls_mt_var_sat_count(st_state *bs, int var_id){
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
void sls_mt_update_var_sat(st_state *bs, int var_id){
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
int cnf_update_clause_sat(st_state *bs, int clause_id){
//_trace;

    int sat = cnf_clause_sat(bs, rdp(bs->clause_array, clause_id));

    array8_write(bs->clause_sat, clause_id, sat);

    return sat;

}



//
//
int sls_mt_clause_sat_count(st_state *bs){
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
int sls_mt_update_clause_sat(st_state *bs){
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
mword sls_mt_choose_unsat_clause(st_state *bs){
//_trace;

    int i;
    int clause_id = -1;
    char *clause_sat = (char*)bs->clause_sat;

    mword start_clause = (sls_mt_rand() % bs->cl->num_clauses);

    for(i=0; i<bs->cl->num_clauses; i++){
        if(!clause_sat[(start_clause+i)%bs->cl->num_clauses])
            return (start_clause+i)%bs->cl->num_clauses;
    }

    return clause_id;

}


//
//
int sls_gsat_solve(st_state *bs, int max_tries, int max_flips){
//_trace;

    int inner_counter = 0;
    int outer_counter = 0;
    int var_id;
    int best_sat_count = 0;

    while(outer_counter++ < max_tries){

        sls_mt_rand_assignment(bs);
        inner_counter = 0;

        while(inner_counter++ < max_flips){
//_trace;

            bs->clause_sat_count = sls_mt_update_clause_sat(bs);
//_trace;

            best_sat_count = (bs->clause_sat_count > best_sat_count)
                    ?
                    bs->clause_sat_count 
                    : 
                    best_sat_count;
//_trace;

            if(bs->clause_sat_count == bs->cl->num_clauses){
                //fprintf(stderr,".");
                if(cnf_clause_all_sat(bs)){
                    //_say("SAT");
                    return best_sat_count;
                }
            }
//_trace;

            var_id = ((unsigned)sls_mt_rand() % (unsigned)bs->cl->num_variables) + 1;
//_trace;

            cnf_var_negate(bs, var_id);
//_trace;

            var_id = sls_gsat_var_choose(bs);
//_trace;

            cnf_var_negate(bs, var_id);
//_trace;

        }

    }

    return best_sat_count;

}


//
//
int sls_gsat_var_make_break(st_state *bs, int var_id){

    int make_count=0;
    int break_count=0;
    int make_break=0;
    int curr_sat;
    int flip_sat;
    int clause_id;


    mword *clauses = rdp(bs->var_clause_map, var_id);
    mword  num_clauses = size(clauses);

    int i;

    cnf_var_negate(bs, var_id);

    for(i=0; i<num_clauses; i++){

        clause_id = rdv(clauses, i);

        curr_sat = array8_read(bs->clause_sat, clause_id);
        flip_sat = cnf_clause_sat(bs, rdp(bs->clause_array, clause_id));

        make_break += (flip_sat-curr_sat);

    }

    cnf_var_negate(bs, var_id);

    return make_break;

}


//
//
int sls_gsat_var_choose(st_state *bs){

    int i;
    mword *clauses;
    int make_break;
    int best_make_break=0;
    int best_i=0;

    for(i=1; i <= bs->cl->num_variables; i++){

        make_break = sls_gsat_var_make_break(bs, i);
//_dd(i);
//_dd(make_break);
        best_make_break = (make_break > best_make_break) ? make_break : best_make_break;

        if(best_make_break == make_break)
            best_i = i;

    }

    return best_i;

}


// Clayton Bauman 2018

