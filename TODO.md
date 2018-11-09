sat_tols
========

*BUGS AT BOTTOM*

    Refactor make.pl:
        if libbabel.a already exists, point make.pl to it
        this will allow sub-projects to compile lib_babel once and then pass
            the compiled library to sat_tools make.pl script

    Next:
        think about:
            pthreads
            pds_la* and graph data-struct support
            mem_frame implementation

    cnf_parse   parsing functions (creates clause_list struct)
    sat_state   main struct holding variable/clause info
    st_init_*   sat_tools initialization functions
    cnf_*       low-level var/clause manipulations + measurement
    ts_*        tree-search functions
    sls_*       sls functions


BUGS
====



