// sls.c

#include "sls.h"
//#include "babel.h"
#include "pearson.h"
#include "cnf.h"
#include "array.h"
//#include "list.h"
#include "mem.h"
#include <math.h>

//
//
int cmp_kca_score(const void *a, const void *b){
    return ( **(float**)a - **(float**)b );
}



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


//// uniform distribution random variable
////
//float sls_rand_range(float range_lo, float range_hi){
//
//    float rand01 = (1.0f * (float)sls_mt_rand()) / (float)RAND_MAX;
//    float range_mag = range_hi-range_lo;
//    return ((float)rand01 * range_mag + range_lo);
//
//}


//
//
void sls_mt_rand_assignment(st_state *st){

    mword num_variables = st->cl->num_variables;
    int i;
    int rand_val;

    char *var_array = (char*)st->var_array;

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
int sls_mt_solve(st_state *st, int max_tries){
//_trace;

    int counter = 0;
    int clause_id;
    int sat_count;
    int best_sat_count = 0;

    st->clause_sat_count = sls_mt_update_clause_sat(st);

    sls_mt_rand_assignment(st);

    while(counter++ < max_tries){

        sat_count = sls_mt_clause_sat_count(st);

        if(sat_count == st->cl->num_clauses)
            break;

        best_sat_count = (sat_count > best_sat_count) ? sat_count : best_sat_count;

        clause_id = sls_mt_choose_unsat_clause(st);

        if(clause_id == -1)
            break;

        sls_mt_clause_kick(st, clause_id);

    }

    return best_sat_count;

}


//
//
void sls_mt_clause_kick(st_state *st, int clause_id){

    mword *clause = rdp(st->clause_array, clause_id);
    mword num_vars = size(clause);
    int this_var;

    int i;

    for(i=0;i<num_vars;i++){

        this_var = abs(rdv(clause,i));

        // jiggle the variable
        if(rand_bent_coin(0.5))
            cnf_var_write(st, this_var, DEC_ASSIGN1_VS);
        else
            cnf_var_write(st, this_var, DEC_ASSIGN0_VS);

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
int sls_mt_var_sat_count_fast(st_state *st, int var_id){
//_trace;

    mword *clause_indices    = rdp(st->var_clause_map, var_id);
    mword num_clause_indices = size(clause_indices);
    char *clause_sat = (char*)st->clause_sat;

    int i;
    int sat_count = 0;

    for(i=0; i<num_clause_indices; i++){
        sat_count += clause_sat[clause_indices[i]];
    }

    return sat_count;

}


// Use this fn to evaluate change to var_id without updating clause_sat
//
int sls_mt_var_sat_count(st_state *st, int var_id){
//_trace;

    mword *clause_indices    = rdp(st->var_clause_map, var_id);
    mword num_clause_indices = size(clause_indices);

    int i;
    int sat_count = 0;

    for(i=0; i<num_clause_indices; i++){
        sat_count += cnf_clause_sat(st, rdp(st->clause_array, clause_indices[i]));
    }

    return sat_count;

}


//
//
void sls_mt_update_var_sat(st_state *st, int var_id){
//_trace;

    mword *clause_indices    = rdp(st->var_clause_map, var_id);
    mword num_clause_indices = size(clause_indices);

    int i;
    for(i=0; i<num_clause_indices; i++){
        cnf_update_clause_sat(st, clause_indices[i]);
    }

}


// Updates st->clause_sat with results of cnf_clause_sat()
//
int cnf_update_clause_sat(st_state *st, int clause_id){
//_trace;

    int sat = cnf_clause_sat(st, rdp(st->clause_array, clause_id));

    array8_write(st->clause_sat, clause_id, sat);

    return sat;

}



//
//
int sls_mt_clause_sat_count(st_state *st){
//_trace;

    int i;
    int sat_count = 0;

    for(i=0; i<st->cl->num_clauses; i++){
        if(cnf_clause_sat(st, rdp(st->clause_array,i)))
            sat_count++;
    }

    return sat_count;

}


//
//
int sls_mt_update_clause_sat(st_state *st){
//_trace;

    int i;
    int sat_count = 0;
    char *clause_sat = (char*)st->clause_sat;

    for(i=0; i<st->cl->num_clauses; i++){

        if(cnf_clause_sat(st, rdp(st->clause_array,i))){

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
mword sls_mt_choose_unsat_clause(st_state *st){
//_trace;

    int i;
    int clause_id = -1;
    char *clause_sat = (char*)st->clause_sat;

    mword start_clause = (sls_mt_rand() % st->cl->num_clauses);

    for(i=0; i<st->cl->num_clauses; i++){
        if(!clause_sat[(start_clause+i)%st->cl->num_clauses])
            return (start_clause+i)%st->cl->num_clauses;
    }

    return clause_id;

}


//
//
int sls_gsat_solve(st_state *st, int max_tries, int max_flips){
//_trace;

    int inner_counter = 0;
    int outer_counter = 0;
    int var_id;
    int best_sat_count = 0;

    while(outer_counter++ < max_tries){

        sls_mt_rand_assignment(st);
        inner_counter = 0;

        while(inner_counter++ < max_flips){

            st->clause_sat_count = sls_mt_update_clause_sat(st);

            best_sat_count = (st->clause_sat_count > best_sat_count)
                    ?
                    st->clause_sat_count 
                    : 
                    best_sat_count;

            if(st->clause_sat_count == st->cl->num_clauses){
                //fprintf(stderr,".");
                if(cnf_clause_all_sat(st)){
                    //_say("SAT");
                    return best_sat_count;
                }
            }

            if(rand_bent_coin(0.5))
                var_id = ((unsigned)sls_mt_rand() % (unsigned)st->cl->num_variables) + 1;
            else
                var_id = sls_gsat_var_choose(st);

            cnf_var_negate(st, var_id);

        }

    }

    return best_sat_count;

}


//
//
int sls_gsat_var_make_break(st_state *st, int var_id){

    int make_count=0;
    int break_count=0;
    int make_break=0;
    int curr_sat;
    int flip_sat;
    int clause_id;

    mword *clauses = rdp(st->var_clause_map, var_id);
    mword  num_clauses = size(clauses);

    int i;

    cnf_var_negate(st, var_id);

    for(i=0; i<num_clauses; i++){

        clause_id = rdv(clauses, i);

        curr_sat = array8_read(st->clause_sat, clause_id);
        flip_sat = cnf_clause_sat(st, rdp(st->clause_array, clause_id));

        make_break += (flip_sat-curr_sat);

    }

    cnf_var_negate(st, var_id);

    return make_break;

}


//
//
int sls_gsat_var_choose(st_state *st){

    int i;
    mword *clauses;
    int make_break;
    int best_make_break=0;
    int best_i=0;

    for(i=1; i <= st->cl->num_variables; i++){

        make_break = sls_gsat_var_make_break(st, i);
//_dd(i);
//_dd(make_break);
        best_make_break = (make_break > best_make_break) ? make_break : best_make_break;

        if(best_make_break == make_break)
            best_i = i;

    }

    return best_i;

}


//
//
int sls_kca_solve(kca_state *ks, int num_candidates, int max_gens){

    sls_kca_solve_init(ks, num_candidates);
    return sls_kca_solve_body(ks, max_gens);

}


//
//
int sls_kca_solve_init(kca_state *ks, int num_candidates){

    mword *candidate_list  = mem_new_ptr(ks->be, num_candidates); // XXX: consider using a pyramidal-array when candidate_list gets large

    int i;
    mword *new_candidate;
    float candidate_score;

    // create ks->candidate_list
    for(i=0; i<num_candidates; i++){

        new_candidate = mem_new_str(ks->be, ks->st->cl->num_assignments, '\0');

        sls_kca_rand_candidate(ks, new_candidate);

        candidate_score = sls_kca_candidate_score(ks, new_candidate);
        ldp(candidate_list, i) = sls_kca_make_candidate(ks->be, candidate_score, new_candidate);//list_cons(ks->be, _val(ks->be, candidate_score), new_candidate);

    }

    ks->candidate_list  = candidate_list;
    ks->num_candidates  = num_candidates;
//    ks->candidate_votes = mem_new_val(ks->be, num_candidates, '\0');

}


// precondition:
//      candidate_list populated:
//          - generate random candidates
//          - score
//
int sls_kca_solve_body(kca_state *ks, int max_gens){

    int var_id;
    int best_sat_count = 0;
    int counter = 0;
    int i,j,k,l;
    int overwrite_index;
    var_state votes[5];
    int votes_0 = 0;

    while(counter++ < max_gens){

//      check top-k champion candidates:
//          convert each literal to a variable setting and 
//              call cnf_var_write() to set the variable
//
//        st->clause_sat_count = sls_mt_update_clause_sat(st);
//
//        best_sat_count = (st->clause_sat_count > best_sat_count)
//                ?
//                st->clause_sat_count 
//                : 
//                best_sat_count;
//
//        if( (st->clause_sat_count == st->cl->num_clauses)
//                && cnf_clause_all_sat(st))
//                return best_sat_count;

// sort candidates by score
// cull bottom half (overwrite below):
// generate n candidates in lower half with random literal assignment
//      score the new candidates
// for each remaining empty slot:
//      sls_kca_generate_candidate(champion, i, k random candidates) k odd
//      score the new candidate

        qsort(ks->candidate_list, ks->num_candidates, sizeof(mword), cmp_kca_score);

        overwrite_index=(ks->num_candidates/2); // cull (overwrite) the bottom 50% of candidates

        for(i=0; i<(ks->num_candidates/100); i++){ // inject 1% random candidates to prevent getting stuck in a local minimum
            sls_kca_rand_candidate(ks, pcdr(rdp(ks->candidate_list, overwrite_index)));
            overwrite_index++;
        }

        // XXX: This loop will THRASH the cache... not sure anything can be done about it
        #define champion_index 0
        l = 0;
        for(i=overwrite_index; i<ks->num_candidates; i++){
            l++;
            for(j=0; j<ks->st->cl->num_assignments; j++){

                votes[0] = array8_read( pcdr(rdp(ks->candidate_list, champion_index)),                      j);
                votes[1] = array8_read( pcdr(rdp(ks->candidate_list, l)),                                   j); // l <- 1..(num_candidates/2)
                votes[2] = array8_read( pcdr(rdp(ks->candidate_list, (sls_mt_rand()%ks->num_candidates) )), j);
                votes[3] = array8_read( pcdr(rdp(ks->candidate_list, (sls_mt_rand()%ks->num_candidates) )), j);
                votes[4] = array8_read( pcdr(rdp(ks->candidate_list, (sls_mt_rand()%ks->num_candidates) )), j);

                votes_0 = 0;
                for(k=0;k<5;k++)
                    if(votes[k] == DEC_ASSIGN0_VS)
                        votes_0++;

                if(votes_0 > 2) // KCA-literal
                    array8_write( pcdr(rdp(ks->candidate_list, l)), j, DEC_ASSIGN0_VS);
                else
                    array8_write( pcdr(rdp(ks->candidate_list, l)), j, DEC_ASSIGN1_VS );

            }

        }

    }

    return best_sat_count;

}


//
//
void sls_kca_rand_candidate(kca_state *ks, mword *candidate){

    mword num_literals = array8_size(candidate);
    int i;
    int rand_val;

    char *var_array = (char*)candidate;

    for(i=1; i<=num_literals; i++){
        rand_val = (mword)sls_mt_rand();
        if(rand_val % 2)
            var_array[i] = DEC_ASSIGN0_VS;
        else
            var_array[i] = DEC_ASSIGN1_VS;
    }

}


//
//
float sls_kca_candidate_score(kca_state *ks, mword *candidate){

    // XXX: This is incorrect, we need a function that will give the clause-sat 
    //      count from a literal array
    return (float)(sls_mt_clause_sat_count(ks->st)) / ks->st->cl->num_clauses;

}


//// input: mu_vk and mu_~vk
////
//float sls_kca_variable_score(void){
//
//}
//
//
//// input: mu_vk and mu_~vk
////
//float sls_kca_clause_score(kca_state *ks, int clause_id, char *clause, int clause_size){
//    // look up rdp(st->clause_array, clause_id) ... val-array
//    // for each literal in this clause:
//    //      3 11 -17
//    //      ^ if match found, return 1.0
//}


// [ptr [val <score>] [val8 <assignment>] ]
//
mword *sls_kca_make_candidate(babel_env *be, mword score, mword *assignment){

    return list_cons(be, _val(be, score), assignment);

}


// Clayton Bauman 2018

