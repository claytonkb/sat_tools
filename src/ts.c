// ts.c

#include "ts.h"
#include "cnf.h"

// Iterative modified DPLL
//
int ts_solve(babel_env *be, st_state *bs){

    int curr_var = 1;
    int result = 0;
    int returning = 0;

    char *solver_stack = (char*)bs->solver_stack;
    solver_cont sc;

    while(1){

bs->dev_ctr++;
if((bs->dev_ctr % 100000) == 0){
    _prn(".");
    if((bs->dev_ctr % 100000000) == 0){
        _say("");
        _dd(bs->dev_ctr);
    }
}

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

        if(cnf_var_assign(bs, curr_var, DEC_ASSIGN1_VS)){
            if(!cnf_var_unsat(bs, curr_var)){
                 solver_stack[curr_var] = CONT_A_SC;
                 curr_var++;
                 continue;
            }
        }

cont_A:

        if(result == 1){
             returning = 1;
             curr_var--;
             continue;
         }
         // else 

        cnf_var_unassign(bs, curr_var);
        if(cnf_var_assign(bs, curr_var, DEC_ASSIGN0_VS)){

            if(cnf_var_unsat(bs, curr_var)){
                result = 0;
                goto cont_B;
            }
            else{
                solver_stack[curr_var] = CONT_B_SC; 
                curr_var++;
                continue;
            }

        }

cont_B:

        if(result == 0){
            if(curr_var == 1) // UNSAT
                break;
            cnf_var_unassign(bs, curr_var);
        }

        returning = 1;
        curr_var--;
        continue;


    }

    return result;

}





// Clayton Bauman 2018

