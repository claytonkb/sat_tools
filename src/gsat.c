// gsat.c

#include "mt.h"
#include "gsat.h"
#include "babel.h"
#include "pearson.h"
#include "cnf.h"
#include "backtrack.h"
#include "array.h"


//
//
int gsat_solve(backtrack_state *bs, int max_tries, int max_flips){
//_trace;

    int inner_counter = 0;
    int outer_counter = 0;
    int var_id;
    int best_sat_count = 0;

    while(outer_counter++ < max_tries){

        mt_rand_assignment(bs);
        inner_counter = 0;

        while(inner_counter++ < max_flips){

            bs->clause_sat_count = mt_update_clause_sat(bs);

            best_sat_count = (bs->clause_sat_count > best_sat_count)
                    ?
                    bs->clause_sat_count 
                    : 
                    best_sat_count;

            if(bs->clause_sat_count == bs->cl->num_clauses){
                //fprintf(stderr,".");
                if(cnf_clause_all_sat(bs)){
                    //_say("SAT");
                    return best_sat_count;
                }
            }

            var_id = ((unsigned)mt_rand() % (unsigned)bs->cl->num_variables) + 1;

            cnf_var_negate(bs, var_id);

            var_id = gsat_var_choose(bs);

            cnf_var_negate(bs, var_id);

        }

    }

    return best_sat_count;

}


//
//
int gsat_var_make_break(backtrack_state *bs, int var_id){

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
int gsat_var_choose(backtrack_state *bs){

    int i;
    mword *clauses;
    int make_break;
    int best_make_break=0;
    int best_i=0;

    for(i=1; i <= bs->cl->num_variables; i++){

        make_break = gsat_var_make_break(bs, i);
//_dd(i);
//_dd(make_break);
        best_make_break = (make_break > best_make_break) ? make_break : best_make_break;

        if(best_make_break == make_break)
            best_i = i;

    }

    return best_i;

}

// Clayton Bauman 2018

