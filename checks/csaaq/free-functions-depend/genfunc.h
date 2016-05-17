#include <other.h>
                                             void f1_1_F();
                                             void f1_2_F(int);
template <class T>                           void f1_1_T();
template <class T>                           void f1_2_T(int);
template <class T, template <class> class U> void f1_1_U(U<T>);
template <class T, template <class> class U> void f1_2_U(U<T>, int);

                   struct DefHF   { struct ID { }; struct IC; };
                   struct DecHF;
typedef DefHF DefYHF;
typedef DecHF DecYHF;
template <class T> struct DefHTF  { struct ID { }; struct IC; };
template <class T> struct DecHTF;
namespace {
                   struct DefHAN  { struct ID { }; struct IC; };
                   struct DecHAN;
typedef DefHF DefYHF;
typedef DecHF DecYHF;
template <class T> struct DefHTAN { struct ID { }; struct IC; };
template <class T> struct DecHTAN;
}
namespace N {
                   struct DefHN   { struct ID { }; struct IC; };
                   struct DecHN;
typedef DefHF DefYHF;
typedef DecHF DecYHF;
template <class T> struct DefHTN  { struct ID { }; struct IC; };
template <class T> struct DecHTN;
}

#define fs(tplt, typn, prefix, name, type)                                    \
    tplt void prefix##_1_##name(typn type);                                   \
    tplt void prefix##_2_##name(const typn type *);                           \
    tplt void prefix##_3_##name(volatile typn type &);                        \
    tplt void prefix##_4_##name(int, typn type);                              \
    tplt void prefix##_5_##name(int, const typn type *);                      \
    tplt void prefix##_6_##name(int, volatile typn type &);

#define fu(tplt, typn, prefix, name, type)                                    \
    template <class T, template <class> class U>                              \
    void prefix##_7_##name(U<typn type>);                                     \
    template <class T, template <class> class U>                              \
    void prefix##_8_##name(U<const typn type *>);                             \
    template <class T, template <class> class U>                              \
    void prefix##_9_##name(U<volatile typn type &>);

#define fss(f, tplt, typn, prefix, name,          type)                       \
            f( tplt,     , prefix, Dec##name,     Dec##type)                  \
            f( tplt,     , prefix, Def##name,     Def##type)                  \
            f( tplt, typn, prefix, Def##name##Id, Def##type::ID)              \
            f( tplt, typn, prefix, Def##name##If, Def##type::IC)

#define allf(f,prefix)                                                        \
fss(f,,, prefix, HF,   HF)                                                    \
fss(f,,, prefix, OHF,  OHF)                                                   \
fss(f,,, prefix, HAN,  HAN)                                                   \
fss(f,,, prefix, OHAN, OHAN)                                                  \
    f(,, prefix, DefYHF, DefYHF)                                              \
    f(,, prefix, DecYHF, DecYHF)                                              \
fss(f, template<class T>, typename, prefix, HTF,   HTF<T>)                    \
fss(f, template<class T>, typename, prefix, OHTF,  OHTF<T>)                   \
fss(f, template<class T>, typename, prefix, HTAN,  HTAN<T>)                   \
fss(f, template<class T>, typename, prefix, OHTAN, OHTAN<T>)

#define all(prefix) allf(fs, prefix) allf(fu, prefix)

all(f1)

namespace N {
all(f1)
}

struct E {
all(f1)
};

extern "C++" {
all(f2)
}

// ---------------------------------------------------------------------------- 
// Copyright (C) 2015 Bloomberg Finance L.P.
//                                                                              
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
