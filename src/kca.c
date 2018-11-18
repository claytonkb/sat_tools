// kca.c

#include "kca.h"
#include "sls.h"
#include "pearson.h"
#include "cnf.h"
#include "array.h"
#include "mem.h"
#include <math.h>

// kca-hybrid

//
//
int kca_solve(kca_state *ks, int num_candidates, int max_gens){

    kca_solve_init(ks, num_candidates);
    return kca_solve_body(ks, max_gens);

}


//
//
int kca_solve_init(kca_state *ks, int num_candidates){

    kca_solve_init_matrices(ks, num_candidates);
    kca_solve_init_lh_score_map(ks, num_candidates);
    kca_solve_init_stats(ks, num_candidates);
    kca_solve_init_clause_map(ks, num_candidates);

}


// precondition:
//      literal_list populated
//
int kca_solve_body(kca_state *ks, int max_gens){

    score_sel sel = CLAUSE_VAR_SCORE;
//    score_sel sel = CLAUSE_SCORE;

    kca_solve_generate_new_candidates(ks, sel);

float champ_score;

    while(max_gens--){

        // sort new candidates and merge firstfruits, if any, into lh_matrix
        kca_solve_merge_new_generation(ks);

champ_score = *(float*)pcar(rdp(ks->lh_score_map, 0));
_df(champ_score);

        // recalculate lit_count of new entries in lh_matrix
        kca_solve_update_lit_count(ks);

//        if(max_gens % 6 > 3){
//            sel = CLAUSE_VAR_SCORE;
//_say("CLAUSE_VAR_SCORE");
//        }
//        else if(max_gens % 6 > 1){
//            sel = VAR_SCORE;
//_say("VAR_SCORE");
//        }
//        else{ //(max_gens % 15 > 0)
//            sel = CLAUSE_SCORE;
//_say("CLAUSE_SCORE");
//        }

        // generate next batch of candidates, scoring is performed during generation
        kca_solve_generate_new_candidates(ks, sel);

    }

    return 0;

}


//
//
int kca_solve_init_matrices(kca_state *ks, int num_candidates){

    mword *lh_matrix = mem_new_ptr(ks->be, ks->st->cl->num_assignments);
    mword *rh_matrix = mem_new_ptr(ks->be, num_candidates/2);

    int i;
    mword *new_literal;
    mword *new_candidate;

    // create lh_matrix rows...
    for(i=0; i < ks->st->cl->num_assignments; i++){
        new_literal = mem_new_str(ks->be, num_candidates/2, '\0');
//        kca_rand_literal(ks, new_literal);
        ldp(lh_matrix, i) = new_literal;
    }

    mword *score;

    // create rh_matrix rows...
    for(i=0; i < num_candidates/2; i++){
        score = _val(ks->be, 0);
        new_candidate = mem_new_str(ks->be, ks->st->cl->num_assignments, '\0');
        kca_rand_candidate(ks, new_candidate);
        ldp(rh_matrix, i) = list_cons(ks->be, score, new_candidate);
    }

    ks->rh_matrix      = rh_matrix;
    ks->lh_matrix      = lh_matrix;
    ks->num_candidates = num_candidates;

}


//
//
int kca_solve_init_lh_score_map(kca_state *ks, int num_candidates){

    mword *lh_score_map = mem_new_ptr(ks->be, num_candidates/2);

    int i;
    for(i=0; i<num_candidates/2; i++){
        ldp(lh_score_map,i) 
            = list_cons(ks->be, _val(ks->be, 0), _val(ks->be, i));
    }

    ks->lh_score_map = lh_score_map;

}


//
//
int kca_solve_init_stats(kca_state *ks, int num_candidates){

    int i;

    ks->sat_count_array = mem_new_val(ks->be, num_candidates/2, 0);
    ks->lit_count_array = mem_new_val(ks->be, ks->st->cl->num_assignments, 0);

    for(i=0; i < ks->st->cl->num_assignments; i++)
        ldv(ks->lit_count_array,i) = (num_candidates/4);

    ks->var_pos_count_array = mem_new_ptr(ks->be, num_candidates/2);
    ks->var_neg_count_array = mem_new_ptr(ks->be, num_candidates/2);

    for(i=0; i < num_candidates/2; i++){
        ldp(ks->var_pos_count_array,i) = mem_new_val(ks->be, ks->st->cl->num_variables+1, 0);
        ldp(ks->var_neg_count_array,i) = mem_new_val(ks->be, ks->st->cl->num_variables+1, 0);
    }

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



//
//
int kca_solve_generate_new_candidates(kca_state *ks, score_sel sel){

//    int first_cand_id = (ks->num_candidates/2) + 1;
    unsigned char *curr_cand;
    var_state lit_choice;

    int i,j;

    kca_solve_reset_sat_counts(ks);

    for(i = 0; i < (ks->num_candidates/2); i++){

        curr_cand = (unsigned char*)pcdr(rdp(ks->rh_matrix, i));

        ks->last_sat_clause = -1;

        for(j=0; j < ks->st->cl->num_assignments; j++){
            lit_choice = kca_rand_lit(ks, j);
            curr_cand[j] = (unsigned char)lit_choice;
            kca_solve_update_counts(ks, i, j, lit_choice);
        }

    }

    kca_solve_score_candidates(ks, sel);

    qsort(ks->rh_matrix, ks->num_candidates/2, sizeof(mword), cmp_kca_score);

}



//
//
int kca_solve_reset_sat_counts(kca_state *ks){

    int i,j;
    mword *curr_cand_pos_counts;
    mword *curr_cand_neg_counts;

    for(i=0; i < ks->num_candidates/2; i++){
        ks->sat_count_array[i] = 0;
//        ks->rh_score_array[i] = 0;
    }

    for(i=0; i < ks->num_candidates/2; i++){
        curr_cand_pos_counts = rdp(ks->var_pos_count_array,i);
        curr_cand_neg_counts = rdp(ks->var_neg_count_array,i);
        for(j=0; j < ks->st->cl->num_variables; j++){
            ldv(curr_cand_pos_counts,j) = 0;
            ldv(curr_cand_neg_counts,j) = 0;
        }
    }

//    ks->last_sat_clause = -1;

}


//
//
int kca_solve_update_counts(kca_state *ks, int cand_id, int lit_id, var_state lit_choice){

    int var_id = abs(rdv(ks->st->cl->variables, lit_id));
    mword *var_count;

    if(((int)rdv(ks->st->cl->variables,lit_id) > 0)
        && (lit_choice == DEC_ASSIGN1_VS)){

        var_count = rdp(ks->var_pos_count_array, cand_id);
        var_count[var_id]++;

        if(ks->last_sat_clause < (int)ks->lit_clause_map[lit_id]){
            ks->sat_count_array[cand_id]++;
            ks->last_sat_clause = ks->lit_clause_map[lit_id];
        }

    }
    else if(((int)rdv(ks->st->cl->variables,lit_id) < 0)
        && (lit_choice == DEC_ASSIGN0_VS)){

        var_count = rdp(ks->var_neg_count_array, cand_id);
        var_count[var_id]++;

        if(ks->last_sat_clause < (int)ks->lit_clause_map[lit_id]){
            ks->sat_count_array[cand_id]++;
            ks->last_sat_clause = ks->lit_clause_map[lit_id];
        }

    }
    //else do nothing

}


//
//
int kca_solve_score_candidates(kca_state *ks, score_sel sel){

    // for cand_id = 0 to M/2
    //      rh_score_array[cand_id] = sls_kca_score(sat_count, pos_count_array, neg_count_array)
    int i;
    mword *cand_score;
    float score;

    for(i=0; i < ks->num_candidates/2; i++){
        cand_score = pcar(rdp(ks->rh_matrix,i));
        score = kca_candidate_score(ks, i, sel);
//_df(score);
        *(float*)cand_score = score;
    }

}


//
//
float  kca_candidate_score(kca_state *ks, int cand_id, score_sel sel){

    float var_score=0.0f;
    float clause_score = kca_clause_score(ks, ks->sat_count_array[cand_id]);
    int i;

    mword *cand_pos_count_array = rdp(ks->var_pos_count_array, cand_id);
    mword *cand_neg_count_array = rdp(ks->var_neg_count_array, cand_id);

    for(i=1; i <= ks->st->cl->num_variables; i++){
        var_score += kca_var_id_score(ks, i, cand_pos_count_array[i], cand_neg_count_array[i]);
    }

//_df(clause_score);
//_df(var_score / ks->st->cl->num_variables);

    switch(sel){
        case CLAUSE_VAR_SCORE:
            return (var_score / ks->st->cl->num_variables) * clause_score;
        case CLAUSE_SCORE:
            return clause_score;
        case VAR_SCORE:
            return (var_score / ks->st->cl->num_variables);
        default:
            _pigs_fly;
    }

}


//
//
float kca_var_id_score(kca_state *ks, int var_id, int var_pos_count, int var_neg_count){

    // pos_avg = var_pos_count/num_lits
    //
    // neg_avg = var_neg_count/num_lits
    //
    // return (1 - abs(pos_avg - neg_avg))

    float pos_avg, neg_avg;
    mword  num_lits;
    mword *lit_clause_map;

    lit_clause_map = rdp(ks->st->lit_pos_clause_map, var_id);
    num_lits = size(lit_clause_map);
//_dd(var_pos_count);
//_dd(num_lits);
    pos_avg = (float)var_pos_count / num_lits;
//_df(pos_avg);

    lit_clause_map = rdp(ks->st->lit_neg_clause_map,var_id);
    num_lits = size(lit_clause_map);
//_dd(var_pos_count);
//_dd(num_lits);

    neg_avg = (float)var_neg_count / num_lits;
//_df(neg_avg);

    return (1 - fabs(pos_avg - neg_avg));

}


//
//
float kca_clause_score(kca_state *ks, int sat_count){

    return (float)sat_count / ks->st->cl->num_clauses;

}


//
//
var_state kca_rand_lit(kca_state *ks, int lit_id){

    float p = (float)ks->lit_count_array[lit_id] / (ks->num_candidates/2);

    if(rand_bent_coin(p))
        return DEC_ASSIGN1_VS;
    else
        return DEC_ASSIGN0_VS;

}


// FIXME: Get rid of rh_score_array and use rh_matrix instead!!!
//
int kca_solve_merge_new_generation(kca_state *ks){

    int num_firstfruits=0;
    float curr_lh_score;
    float curr_rh_score;
//_trace;
    // calculate num_firstfruits
    int lh_index = (ks->num_candidates/2)-1; // consider sorting in major order
    int rh_index = 0;
    int lh_cand_id;
//_trace;

    curr_lh_score = *(float*)pcar(rdp(ks->lh_score_map, lh_index));
    curr_rh_score = *(float*)pcar(rdp(ks->rh_matrix, rh_index));
//_trace;

    while(curr_lh_score < curr_rh_score){
//_trace;
        num_firstfruits++;
        if((lh_index == 0) || (rh_index == ks->num_candidates/2))
            break;
        lh_index--;
        rh_index++;
        curr_lh_score = *(float*)pcar(rdp(ks->lh_score_map, lh_index));
        curr_rh_score = *(float*)pcar(rdp(ks->rh_matrix, rh_index));
    }
_dd(num_firstfruits);

    unsigned char *lh_lit;
    unsigned char *rh_lit;

    // overwrite worst lh_matrix candidates with best rh_matrix candidates
    // XXX THIS IS THE SLOW LOOP XXX
    int i,j;
//_trace;
    lh_index = (ks->num_candidates/2)-1; // consider sorting in major order
    for(i=0; i<num_firstfruits; i++){
//_trace;
        lh_cand_id = rdv(pcdr(rdp(ks->lh_score_map, lh_index)),0);
        rh_lit = (unsigned char*)pcdr(rdp(ks->rh_matrix, i));
        for(j=0; j < ks->st->cl->num_assignments; j++){
//_trace;
            lh_lit = (unsigned char*)rdp(ks->lh_matrix, j);
            lh_lit[lh_cand_id] = rh_lit[j]; // Ouch! :(
        }
        ldv(pcar(rdp(ks->lh_score_map, lh_index)),0) = rdv(pcar(rdp(ks->rh_matrix, i)),0);
        lh_index--;
    }
//_trace;

    // qsort the lh_score_map
    qsort(ks->lh_score_map, ks->num_candidates/2, sizeof(mword), cmp_kca_score);
//_trace;

    return num_firstfruits;

}


// NOTE: don't necessarily need to perform this update on every generation
//
int kca_solve_update_lit_count(kca_state *ks){

    int i,j;
    unsigned char *curr_lit;
    int lit_count;

    for(i=0; i < ks->st->cl->num_assignments; i++){

        curr_lit = (unsigned char*)rdp(ks->lh_matrix,i);
        lit_count=0;

        if( (int)rdv(ks->st->cl->variables, i) > 0 ){
            for(j=0; j < ks->num_candidates/2; j++){
                if(curr_lit[j] == DEC_ASSIGN1_VS)
                    lit_count++;
            }
        }
        else{ // (int)rdv(ks->st->cl->variables, i) > 0
            for(j=0; j < ks->num_candidates/2; j++){
                if(curr_lit[j] == DEC_ASSIGN0_VS)
                    lit_count++;
            }
        }

        ldv(ks->lit_count_array,i) = lit_count;

    }
}


// synonym: kca_rand_literal
//
void kca_rand_candidate(kca_state *ks, mword *candidate){

    mword num_literals = array8_size(candidate);
    int i;
    int rand_val;

    unsigned char *var_array = (unsigned char*)candidate;

    for(i=0; i<num_literals; i++){
        rand_val = (mword)sls_mt_rand();
        if(rand_val % 2)
            var_array[i] = DEC_ASSIGN0_VS;
        else
            var_array[i] = DEC_ASSIGN1_VS;
    }

}


// Clayton Bauman 2018

