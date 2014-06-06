// csatr_nesteddeclarations.t.hpp                                     -*-C++-*-
// -----------------------------------------------------------------------------
// Copyright 2012 Dietmar Kuehl http://www.dietmar-kuehl.de              
// Distributed under the Boost Software License, Version 1.0. (See file  
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).     
// -----------------------------------------------------------------------------

#if !defined(INCLUDED_CSATR_NESTEDDECLARATIONS)
#define INCLUDED_CSATR_NESTEDDECLARATIONS 1
#if !defined(INCLUDED_BDES_IDENT)
#  include <bdes_ident.h>
#endif
#ifndef INCLUDED_CSASCM_VERSION
#  include <csascm_version.h>
#endif

#ifndef INCLUDED_CSATR_OTHERS
#include "csatr_others.hpp"
#endif

// -----------------------------------------------------------------------------

struct good_global_struct;
struct nesteddeclarations_bad_global_struct
{
    int member;
    void method();
    struct nested {};
};
void nesteddeclarations_bad_global_function();
int nesteddeclarations_bad_global_variable;

namespace bde_verify
{
    struct good_bde_verify_struct;
    struct nesteddeclarations_bad_bde_verify_struct
    {
        int member;
        void method();
        struct nested {};
    };
    void nesteddeclarations_bad_bde_verify_function();
    int nesteddeclarations_bad_bde_verify_variable;

    namespace csamisc
    {
        struct good_bde_verify_csamisc_struct;
        struct nesteddeclarations_bad_bde_verify_csamisc_struct
        {
            int member;
            void method();
            struct nested {};
        };
        void nesteddeclarations_bad_bde_verify_csamisc_function();
        int nesteddeclarations_bad_bde_verify_csamisc_variable;
    }

    namespace csatr
    {
        struct good_struct1;
        struct nesteddeclarations_good_struct2
        {
            struct good_nested {};
            void good_method();
            int  good_member;
        };

        void nesteddeclarations_good_function();
        int nesteddeclarations_good_bde_verify_csatr_variable;
    }

    struct csatr_NestedDeclarations
    {
    };

    void swap(csatr_NestedDeclarations&, csatr_NestedDeclarations&);
    bool operator== (const csatr_NestedDeclarations&,
                    const  csatr_NestedDeclarations&);
    bool operator!= (const csatr_NestedDeclarations&,
                    const  csatr_NestedDeclarations&);
}

// -----------------------------------------------------------------------------

#endif /* INCLUDED_GROUPS_CSA_CSATR_CSATR_NESTEDDECLARATIONS_T */