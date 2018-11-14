// kca.c

#include "kca.h"
#include "sls.h"
#include "pearson.h"
#include "cnf.h"
#include "array.h"
#include "mem.h"
#include <math.h>

// kca-transpose...

//
//
int kca_solve(kca_state *ks, int num_candidates, int max_gens){

    kca_solve_init(ks, num_candidates);
    return kca_solve_body(ks, max_gens);

}


//
//
int kca_solve_init(kca_state *ks, int num_candidates){

    kca_solve_init_literals(ks, num_candidates);
    kca_solve_init_score_map(ks, num_candidates);
    kca_solve_init_clause_map(ks, num_candidates);

}


//
//
int kca_solve_init_literals(kca_state *ks, int num_candidates){

    mword *literal_list  = mem_new_ptr(ks->be, ks->st->cl->num_assignments);

    int i;
    mword *new_literal;
    float candidate_score=0;
    mword *candidate_score_val;

    // create ks->literal_list
    for(i=0; i < ks->st->cl->num_assignments; i++){

        new_literal = mem_new_str(ks->be, num_candidates, '\0');
        kca_rand_literal(ks, new_literal);
        ldp(literal_list,i) = new_literal;

    }

    ks->literal_list   = literal_list;
    ks->num_candidates = num_candidates;

}


//
//
int kca_solve_init_score_map(kca_state *ks, int num_candidates){

    mword *candidate_score_map = mem_new_ptr(ks->be, num_candidates);

    int i;
    for(i=0; i<num_candidates; i++){
        ldp(candidate_score_map,i) 
            = list_cons(ks->be, _val(ks->be, 0), _val(ks->be, i));
    }

    ks->candidate_score_map = candidate_score_map;

}


//
//
int kca_solve_init_clause_map(kca_state *ks, int num_candidates){

    mword *lit_clause_map = mem_new_val(ks->be, ks->st->cl->num_assignments, 0);

    int i,j;
    int clause_length;
    int lit_id=0;

    // create ks->literal_list
    for(i=0; i < ks->st->cl->num_clauses; i++){

        clause_length = ks->st->cl->clause_lengths[i];

        for(j=0; j < clause_length; j++){
            ldv(lit_clause_map, lit_id) = i;
            lit_id++;
        }

    }

    ks->lit_clause_map = lit_clause_map;

}


// precondition:
//      candidate_list populated:
//          - generate random candidates
//          - score
//
int kca_solve_body(kca_state *ks, int max_gens){

    int var_id;
    int best_sat_count = 0;
    int counter = 0;
    int i,j,k,l;
    int overwrite_index;
    var_state votes[5]; // FIXME: Make number of votes parameterizable
    int       vote_ids[5];
    int votes_0 = 0;
    mword  candidate_id;
    mword *curr_candidate;
    float candidate_score;
    mword *candidate_score_val;
    float champion_score;
    float sample_rate = 0.5;

    while(counter++ < max_gens){

// sort candidates by score
// cull bottom half (overwrite below):
// generate n candidates in lower half with random literal assignment
//      score the new candidates
// for each remaining empty slot:
//      kca_generate_candidate(champion, i, k random candidates) k odd
//      score the new candidate

        qsort(ks->candidate_list, ks->num_candidates, sizeof(mword), cmp_kca_score);

        overwrite_index=(ks->num_candidates/2); // cull (overwrite) the bottom 50% of candidates

//        for(i=0; i<=(ks->num_candidates/20); i++){ // inject 5% random candidates to prevent getting stuck in a local minimum
//
//            kca_rand_candidate(ks, pcdr(rdp(ks->candidate_list, overwrite_index)));
//
//            candidate_score = kca_candidate_score(ks, pcdr(rdp(ks->candidate_list, overwrite_index)), sample_rate);
//            candidate_score_val = ldp(rdp(ks->candidate_list, overwrite_index),0);
//            *(float*)candidate_score_val = candidate_score;
//
//            overwrite_index++;
//
//        }

        // XXX: This loop will THRASH the cache... not sure anything can be done about it
        #define champion_index 0
        l = 0;

        for(i=overwrite_index; i<ks->num_candidates; i++){

            // FIXME bad loop:
            //      select candidate indices here, then loop over those...

            // NOTE: add parameter to kca_candidate_score() to allow to focus
            //       on either variables or clauses. Do some initial training
            //       across both simultaneously, then begin alternating.

            // NOTE: add parameter to kca_candidate_score() to indicate how
            //       extensively to measure score. Exhaustive measurement is way
            //       too costly, especially in early generations.

            vote_ids[0] = l;

            if(rand_bent_coin(0.5))
                vote_ids[1] = champion_index;
            else
                vote_ids[1] = ((unsigned)sls_mt_rand()%(ks->num_candidates/2));

            vote_ids[2] = ((unsigned)sls_mt_rand()%(ks->num_candidates/2));
            vote_ids[3] = ((unsigned)sls_mt_rand()%(ks->num_candidates/2));
            vote_ids[4] = ((unsigned)sls_mt_rand()%(ks->num_candidates/2));

            for(j=0; j<ks->st->cl->num_assignments; j++){

                curr_candidate = pcdr(rdp(ks->candidate_list, vote_ids[0]));
                votes[0] = array8_read(curr_candidate, j);

                curr_candidate = pcdr(rdp(ks->candidate_list, vote_ids[1]));
                votes[1] = array8_read(curr_candidate, j);

                curr_candidate = pcdr(rdp(ks->candidate_list, vote_ids[2]));
                votes[2] = array8_read(curr_candidate, j);

                curr_candidate = pcdr(rdp(ks->candidate_list, vote_ids[3]));
                votes[3] = array8_read(curr_candidate, j);

                curr_candidate = pcdr(rdp(ks->candidate_list, vote_ids[4]));
                votes[4] = array8_read(curr_candidate, j);

                votes_0 = 0;
                for(k=0;k<5;k++){
                    if(votes[k] == DEC_ASSIGN0_VS)
                        votes_0++;
                }

                if(votes_0 > 2) // KCA-literal
                    array8_write( pcdr(rdp(ks->candidate_list, i)), j, DEC_ASSIGN0_VS);
                else
                    array8_write( pcdr(rdp(ks->candidate_list, i)), j, DEC_ASSIGN1_VS );

            }

            l++;

            ldv(ldp(rdp(ks->candidate_list, i),0),0) =
                kca_candidate_score(ks, pcdr(rdp(ks->candidate_list, i)), sample_rate);

            candidate_score = kca_candidate_score(ks, pcdr(rdp(ks->candidate_list, i)), sample_rate);
            candidate_score_val = ldp(rdp(ks->candidate_list, i),0);
            *(float*)candidate_score_val = candidate_score;

        }

        champion_score = *(float*)(pcar(rdp(ks->candidate_list, 0)));
        sample_rate = champion_score / 8;

    }

    return best_sat_count;

}


int kca_solution(kca_state *ks, mword *candidate){
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
}


//
//
void kca_rand_candidate(kca_state *ks, mword *candidate){

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
float kca_candidate_score(kca_state *ks, mword *candidate, float sample_rate){

    float result=1.0;
    float variable_score;
    float clause_score;
    int i=0;

    while(result == 1.0){
        if(i++ > 10)
            break;
        variable_score = kca_variable_score(ks, (char*)candidate, sample_rate);
        clause_score   = kca_clause_score(ks, (char*)candidate,   sample_rate);
        result = variable_score * clause_score;
    }

    return result;

}


//
//
float kca_variable_score(kca_state *ks, char *candidate_assignment, float sample_rate){

    int i;
    int num_samples;
    mword var_id;
    float sum=0;

    if(sample_rate == 1.0f){
        for(i=1; i <= ks->st->cl->num_variables; i++){
            sum += kca_var_id_score(ks, candidate_assignment, i);
        }
        return sum / ks->st->cl->num_variables;
    }
    else{
        num_samples = ks->st->cl->num_variables * sample_rate;
        for(i=0; i < num_samples; i++){
            var_id = ((unsigned)sls_mt_rand() % (unsigned)ks->st->cl->num_variables) + 1;
            sum += kca_var_id_score(ks, candidate_assignment, var_id);
        }
        return sum / num_samples;
    }

}


// input: mu_vk and mu_~vk
//
float kca_var_id_score(kca_state *ks, char *candidate_assignment, int var_id){

    // sum = 0
    //
    // for each pos_lit:
    //      sum += candidate_assignment[lit_clause_map[i]]
    //
    // pos_avg = sum/num_lits
    //
    // for each neg_lit:
    //      sum += cnf_var_xor_score()
    //
    // neg_avg = sum/num_lits
    //
    // return (1 - abs(pos_avg - neg_avg))

    float pos_avg, neg_avg;
    mword  num_lits;
    mword *lit_clause_map;

    int i;
    int sum,lit_is_true;

    lit_clause_map = rdp(ks->st->lit_pos_clause_map, var_id);
    num_lits = size(lit_clause_map);
    sum=0;

    for(i=0; i < num_lits; i++){
        lit_is_true = ( candidate_assignment[lit_clause_map[i]] == DEC_ASSIGN1_VS );
        sum += lit_is_true;
    }

    pos_avg = (float)sum / num_lits;

    lit_clause_map = rdp(ks->st->lit_neg_clause_map,var_id);
    num_lits = size(lit_clause_map);
    sum=0;

    for(i=0; i < num_lits; i++){
        lit_is_true = ( candidate_assignment[lit_clause_map[i]] == DEC_ASSIGN0_VS );
        sum += lit_is_true;
    }

    neg_avg = (float)sum / num_lits;

    return (1 - fabs(pos_avg - neg_avg));

}


// input: mu_vk and mu_~vk
//
float kca_clause_score(kca_state *ks, char *candidate_assignment, float sample_rate){

    // for each clause: 
        // sum += cnf_clause_sat_lit(st_state *bs, int clause_id, char *candidate_assignment);
    //
    // return sum/num_clauses
    int i;
    int sum = 0;
    int num_samples;
    mword clause_id;
    int sat;

    if(sample_rate == 1.0f){
        for(i=0; i < ks->st->cl->num_clauses; i++){
            sat = cnf_clause_sat_lit(ks->st, i, candidate_assignment);
            sum += sat;
        }

        return ((float)sum) / ks->st->cl->num_clauses;
    }
    else{
        num_samples = ks->st->cl->num_clauses * sample_rate;

        for(i=0; i < num_samples; i++){
            clause_id = ((unsigned)sls_mt_rand() % (unsigned)ks->st->cl->num_clauses);
            sat = cnf_clause_sat_lit(ks->st, clause_id, candidate_assignment);
            sum += sat;
        }

        return ((float)sum) / num_samples;
    }

}


// Clayton Bauman 2018

