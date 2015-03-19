#ifndef STATIC_BIGNUM_HPP
#define STATIC_BIGNUM_HPP

#include <compatibility.hpp>

#include <bitset.hpp>
#include <stdint.h>
// #include <limits>
#include <string>
#include <type_traits.hpp>

namespace static_bignum {

// some forward declarations
template<class A> struct DecimalRepresentation;

typedef uint64_t word;
static const word WORD_MAX = word(-1); // HACK
static const size_t bits_per_word = 8 * sizeof(word);

struct Zero {
    static const word digit = 0;
    static const size_t length = 0;
    typedef word Word;
    static inline std::string bin_impl(void) {
        return "";
    }
    static inline std::string bin(void) {
        return "0b0";
    }
    static inline void write_to(word* dest) {
        // no op
    }
    static inline word get_digit(size_t which) {
        return word(0);
    }
};

struct ErrorType {
    
};

template<word n, class T = Zero>
struct BigUnsigned {
    static const word digit = n;
    static const size_t length = T::length + 1;
    typedef T Next;
    typedef word Word;
    static inline std::string bin_impl(void) {
        //std::bitset<bits_per_word> bset(n);
        return Next::bin_impl(); //+ bset.to_string();
    }
    static inline std::string bin(void) {
        return "0b" + bin_impl();
    }
    static inline std::string dec(void) {
        return DecimalRepresentation<BigUnsigned>::str();
    }
    static inline void write_to(word* dest) {
        *dest = n;
        T::write_to(dest + 1);
    }
    static inline word get_digit(size_t which) {
        return which == 0 ? n : T::get_digit(which - 1);
    }
};

typedef BigUnsigned<1, Zero> One;

template<class A> struct Optimize;

template<>
struct Optimize<Zero> {
    typedef Zero Result;
};

template<>
struct Optimize<BigUnsigned<0, Zero> > {
    typedef Zero Result;
};

template<word n, class T>
struct Optimize<BigUnsigned<n, T> > {
private:
    typedef typename Optimize<T>::Result Tail;
public:
    typedef typename std::conditional<
        n == 0 && std::is_same<Tail, Zero>::value,
        Zero,
        BigUnsigned<n, Tail>
    >::type Result;
};

#define BIGUNSIGNED_1(x1) typename static_bignum::Optimize<static_bignum::BigUnsigned<(x1)> >::Result
//#define BIGUNSIGNED_1(x1)         static_bignum::BigUnsigned<(x1), static_bignum::NoneType>
#define BIGUNSIGNED_2(x1, x2)         static_bignum::BigUnsigned<(x2), BIGUNSIGNED_1((x1))>
#define BIGUNSIGNED_3(x1, x2, x3)     static_bignum::BigUnsigned<(x3), BIGUNSIGNED_2((x2), (x1))>
#define BIGUNSIGNED_4(x1, x2, x3, x4) static_bignum::BigUnsigned<(x4), BIGUNSIGNED_3((x3), (x2), (x1))>

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// addition /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct Sum;

template<> struct Sum<Zero, Zero> { typedef Zero Result; };

template<word n, class T>
struct Sum<Zero, BigUnsigned<n, T> > { typedef BigUnsigned<n, T> Result; };

template<word n, class T>
struct Sum<BigUnsigned<n, T>, Zero> { typedef BigUnsigned<n, T> Result; };

template<word a_n, word b_n, class a_T, class b_T>
struct Sum<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    static const word carry = b_n > WORD_MAX - a_n ? 1 : 0;
    typedef BigUnsigned<a_n + b_n, typename Sum<
        typename Sum<a_T, b_T>::Result,
        BIGUNSIGNED_1(carry)
    >::Result> Result;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_SUM(x, y) static_bignum::Sum<x, y>::Result
#endif

///////////////////////////////////////////////////////////////////////////
/////////////////////////////// subtraction ///////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct Difference;

template<> struct Difference<Zero, Zero> { typedef Zero Result; };

template<word n, class T>
struct Difference<BigUnsigned<n, T>, Zero> { typedef BigUnsigned<n, T> Result; };

template<word n, class T>
struct Difference<Zero, BigUnsigned<n, T> > { typedef ErrorType Result; };

template<class A> struct Difference<ErrorType, A> { typedef ErrorType Result; };
template<class A> struct Difference<A, ErrorType> { typedef ErrorType Result; };

template<word a_n, word b_n, class a_T, class b_T>
struct Difference<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    typedef BigUnsigned<a_n, a_T> A;
    typedef BigUnsigned<b_n, b_T> B;
    typedef typename Difference<a_T, b_T>::Result C;
    typedef typename std::conditional< // deciding whether carry or not
        a_n >= b_n, C, typename Difference<C, One>::Result
    >::type Result_T;
//     static const word Result_n = // it will be done automatically
//         a_n >= b_n ? (a_n - b_n) : ((UINT64_MAX - b_n) + (a_n + 1));
    typedef typename std::conditional< // removing trailing zeros // TODO Optimize
        a_n == b_n && std::is_same<Result_T, Zero>::value,
        Zero,
        BigUnsigned<a_n - b_n, Result_T>
    >::type Result;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_DIFF(x, y) static_bignum::Difference<x, y>::Result
#endif

///////////////////////////////////////////////////////////////////////////
////////////////////////////////// shift //////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// FIXME left shift by 193 turns 1-word integer into 8-word, by 129 --
//       into 6-word if applying BigShiftLeft before SmallShiftLeft

template<class A, size_t shift> struct BigShiftLeft;
template<class A, size_t shift> struct BigShiftRight;
template<class A, size_t shift> struct SmallShiftLeft;
template<class A, size_t shift> struct SmallShiftRight;


template<size_t shift> struct BigShiftLeft   <Zero, shift> { typedef Zero Result; };
template<size_t shift> struct BigShiftRight  <Zero, shift> { typedef Zero Result; };
template<size_t shift> struct SmallShiftLeft <Zero, shift> { typedef Zero Result; };
template<size_t shift> struct SmallShiftRight<Zero, shift> {
    static const word carry = 0;
    typedef Zero Result;
};

template<word n, class T, size_t shift>
struct BigShiftLeft<BigUnsigned<n, T>, shift> {
    typedef BigUnsigned<n, T> Argument;
    typedef BigUnsigned<0ULL, typename BigShiftLeft<Argument, shift-1>::Result> Result;
};
template<word n, class T, size_t shift>
struct SmallShiftLeft<BigUnsigned<n, T>, shift> {
    /*static_assert(shift < bits_per_word, "shift in SmallShiftLeft must be less than bits per word");*/
    static const word carry = n >> (bits_per_word - shift);
    typedef BigUnsigned<
        n << shift,
        typename Sum<
            typename SmallShiftLeft<T, shift>::Result,
            BIGUNSIGNED_1(carry)
        >::Result
    > Result;
};

template<word n, class T, size_t shift>
struct BigShiftRight<BigUnsigned<n, T>, shift> {
    typedef BigUnsigned<n, T> Argument;
    typedef typename BigShiftRight<T, shift-1>::Result Result;
};
template<word n, class T, size_t shift>
struct SmallShiftRight<BigUnsigned<n, T>, shift> {
    /*static_assert(shift < bits_per_word, "shift in SmallShiftRight must be less than bits per word");*/
    static const word carry = n << (bits_per_word - shift);
    typedef BigUnsigned<
        (n >> shift) | SmallShiftRight<T, shift>::carry,
        typename SmallShiftRight<T, shift>::Result
    > Result;
};

template<word n, class T>
struct BigShiftLeft   <BigUnsigned<n, T>, 0> { typedef BigUnsigned<n, T> Result; };
template<word n, class T>
struct BigShiftRight  <BigUnsigned<n, T>, 0> { typedef BigUnsigned<n, T> Result; };
template<word n, class T>
struct SmallShiftLeft <BigUnsigned<n, T>, 0> { typedef BigUnsigned<n, T> Result; };
template<word n, class T>
struct SmallShiftRight<BigUnsigned<n, T>, 0> {
    static const word carry = 0;
    typedef BigUnsigned<n, T> Result;
};

template<class A, size_t shift>
struct ShiftLeft {
    typedef typename BigShiftLeft<
        typename SmallShiftLeft<A, shift % bits_per_word>::Result, shift / bits_per_word
    >::Result Result;
};
template<size_t shift> struct ShiftLeft<Zero, shift> { typedef Zero Result; };
template<word n, class T>
struct ShiftLeft<BigUnsigned<n, T>, 0> { typedef BigUnsigned<n, T> Result; };

template<class A, size_t shift>
struct ShiftRight {
    typedef typename SmallShiftRight<
        typename BigShiftRight<A, shift / bits_per_word>::Result, shift % bits_per_word
    >::Result Result;
};
template<size_t shift> struct ShiftRight<Zero, shift> { typedef Zero Result; };
template<word n, class T>
struct ShiftRight<BigUnsigned<n, T>, 0> { typedef BigUnsigned<n, T> Result; };

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_SHL(x, y) static_bignum::ShiftLeft <x, y>::Result
#define SBN_SHR(x, y) static_bignum::ShiftRight<x, y>::Result
#endif


///////////////////////////////////////////////////////////////////////////
///////////////////////////////// product /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct Product;

template<> struct Product<Zero, Zero>     { typedef Zero Result; };
template<> struct Product<Zero, One>      { typedef Zero Result; };
template<> struct Product<One, Zero>      { typedef Zero Result; };
template<> struct Product<One, One>       { typedef One Result;  };
template<word n, class T> struct Product<Zero, BigUnsigned<n, T> > { typedef Zero Result; };
template<word n, class T> struct Product<BigUnsigned<n, T>, Zero>  { typedef Zero Result; };
template<word n, class T> struct Product<BigUnsigned<n, T>, One>   { typedef BigUnsigned<n, T> Result; };
template<word n, class T> struct Product<One, BigUnsigned<n, T> >  { typedef BigUnsigned<n, T> Result; };

template<word a_n, word b_n, class a_T, class b_T>
struct Product<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    // a * b = P ** 2 * (a / P) * (b / P)
    //       +          (a % P) * (b % P)
    //       + P      * (a % P) * (b / P)
    //       + P      * (a / P) * (b % P)
    typedef BigUnsigned<a_n, a_T> A;
    typedef BigUnsigned<b_n, b_T> B;
    typedef typename Optimize<typename SmallShiftRight<A, 1>::Result>::Result Adiv2;
    typedef typename Optimize<typename SmallShiftRight<B, 1>::Result>::Result Bdiv2;
    typedef BIGUNSIGNED_1(a_n & 1) Amod2;
    typedef BIGUNSIGNED_1(b_n & 1) Bmod2;
    typedef typename Sum<
        typename Sum<
            typename SmallShiftLeft<typename Product<Adiv2, Bdiv2>::Result, 2>::Result,
            BIGUNSIGNED_1((a_n & 1) * (b_n & 1))
        >::Result, typename Sum<
            typename SmallShiftLeft<typename Product<Adiv2, Bmod2>::Result, 1>::Result,
            typename SmallShiftLeft<typename Product<Amod2, Bdiv2>::Result, 1>::Result
        >::Result
    >::Result Result;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_PROD(x, y) static_bignum::Product<x, y>::Result
#endif

///////////////////////////////////////////////////////////////////////////
/////////////////////////////// comparison ////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct GreaterThan;
template<class A, class B> struct GreaterThanOrEqualTo;

template<class A> struct GreaterThan<Zero, A>          { static const bool value = false; };
template<class A> struct GreaterThanOrEqualTo<Zero, A> { static const bool value = false; };
template<> struct GreaterThanOrEqualTo<Zero, Zero>     { static const bool value = true;  };

template<word n, class T>
struct GreaterThanOrEqualTo<BigUnsigned<n, T>, Zero> {
    static const bool value = true;
};

template<word n, class T>
struct GreaterThan<BigUnsigned<n, T>, Zero> {
    static const bool value = n > 0 || GreaterThan<T, Zero>::value;
};

template<word a_n, word b_n, class a_T, class b_T>
struct GreaterThan<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    static const bool value =
        GreaterThan<a_T, b_T>::value ? true :
        (GreaterThanOrEqualTo<a_T, b_T>::value && a_n > b_n);
};

template<word a_n, word b_n, class a_T, class b_T>
struct GreaterThanOrEqualTo<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    static const bool value =
        GreaterThan<a_T, b_T>::value ? true :
        (GreaterThanOrEqualTo<a_T, b_T>::value && a_n >= b_n);
};

template<class A, class B>
struct LessThan {
    static const bool value = !GreaterThanOrEqualTo<A, B>::value;
};

template<class A, class B>
struct LessThanOrEqualTo {
    static const bool value = !GreaterThan<A, B>::value;
};

template<class A, class B>
struct EqualTo {
    static const bool value = GreaterThanOrEqualTo<A, B>::value && LessThanOrEqualTo<A, B>::value;
};

template<class A, class B> struct Max;
template<class A, class B> struct Min;
template<> struct Max<Zero, Zero> { typedef Zero Result; };
template<> struct Min<Zero, Zero> { typedef Zero Result; };

template<word n, class T>
struct Max<BigUnsigned<n, T>, Zero> {
    typedef BigUnsigned<n, T> Result;
};

template<word n, class T>
struct Max<Zero, BigUnsigned<n, T> > {
    typedef BigUnsigned<n, T> Result;
};

template<word n, class T>
struct Min<BigUnsigned<n, T>, Zero> {
    typedef Zero Result;
};

template<word n, class T>
struct Min<Zero, BigUnsigned<n, T> > {
    typedef Zero Result;
};

template<word a_n, word b_n, class a_T, class b_T>
struct Max<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    typedef BigUnsigned<a_n, a_T> A;
    typedef BigUnsigned<b_n, b_T> B;
    typedef typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, A, B
    >::type Result;
};

template<word a_n, word b_n, class a_T, class b_T>
struct Min<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    typedef BigUnsigned<a_n, a_T> A;
    typedef BigUnsigned<b_n, b_T> B;
    typedef typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, B, A
    >::type Result;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_GT(x, y)  (static_bignum::GreaterThan<x, y>::value)
#define SBN_GTE(x, y) (static_bignum::GreaterThanOrEqualTo<x, y>::value)
#define SBN_LT(x, y)  (static_bignum::LessThan<x, y>::value)
#define SBN_LTE(x, y) (static_bignum::LessThanOrEqualTo<x, y>::value)
#endif

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// division /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B>
struct Division;

template<class A>
struct Division<A, Zero> {
    typedef ErrorType Quotient;
    typedef ErrorType Residue;
};

template<word n, class T>
struct Division<BigUnsigned<n,  T>, One> {
    typedef BigUnsigned<n,  T> Quotient;
    typedef Zero Residue;
};

template<class A, class B>
struct DummyDivision {
    typedef Zero Quotient;
    typedef A Residue;
};

template<word a_n, word b_n, class a_T, class b_T>
struct Division<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
private:
    typedef BigUnsigned<a_n, a_T> A;
    typedef BigUnsigned<b_n, b_T> B;
    typedef typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value,
        Division<A, typename SmallShiftLeft<B, 1>::Result>,
        DummyDivision<A, B>
    >::type D;
    typedef typename D::Quotient Q;
    typedef typename D::Residue R;
public:
    typedef typename Sum<
        typename SmallShiftLeft<Q, 1>::Result,
        typename std::conditional<GreaterThanOrEqualTo<R, B>::value, One, Zero>::type
    >::Result Quotient;
    typedef typename std::conditional<
        GreaterThanOrEqualTo<R, B>::value,
        typename Difference<R, B>::Result,
        R
    >::type Residue;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_DIV(x, y) static_bignum::Division<x, y>::Quotient
#define SBN_MOD(x, y) static_bignum::Division<x, y>::Residue
#endif

///////////////////////////////////////////////////////////////////////////
//////////////////////////// signed operations ////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<int s, class T>
struct BigSigned {
    /*static_assert(s == 1 || s == -1, "only 1 or -1 are allowed as signs");*/
    static const int sign = s;
    typedef T Abs;
    static inline std::string bin(void) {
        return (s < 0 ? "-" : "") + T::bin();
    }
    static inline std::string dec(void) {
        return (s < 0 ? "-" : "") + T::dec();
    }
    static inline void write_to(word* dest) {
        T::write_to(dest);
    }
};

#define BIGSIGNED_1(x1)             static_bignum::BigSigned<1, BIGUNSIGNED_1(x1)>
#define BIGSIGNED_2(x1, x2)         static_bignum::BigSigned<1, BIGUNSIGNED_2((x1), (x2))>
#define BIGSIGNED_3(x1, x2, x3)     static_bignum::BigSigned<1, BIGUNSIGNED_3((x1), (x2), (x3))>
#define BIGSIGNED_4(x1, x2, x3, x4) static_bignum::BigSigned<1, BIGUNSIGNED_4((x1), (x2), (x3), (x4))>


template<class A> struct Minus;
template<int s, class T>
struct Minus<BigSigned<s, T> > {
    typedef BigSigned<-s, T> Result;
};
template<word n, class T>
struct Minus<BigUnsigned<n, T> > {
    typedef BigSigned<-1, BigUnsigned<n, T> > Result;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_MINUS(x) static_bignum::Minus<x>::Result
#endif

template<class A> struct Sign;
template<class A> struct Signed;
template<class A> struct Unsigned;
template<word n, class T>
struct Signed<BigUnsigned<n, T> > {
    typedef BigSigned<1, BigUnsigned<n, T> > Result;
};
template<int s, class A>
struct Signed<BigSigned<s, A> > {
    typedef BigSigned<s, A> Result;
};
template<word n, class T>
struct Unsigned<BigUnsigned<n, T> > {
    typedef BigUnsigned<n, T> Result;
};
template<int s, class A>
struct Unsigned<BigSigned<s, A> > {
    typedef A Result;
};
template<word n, class T>
struct Sign<BigUnsigned<n, T> > {
    static const int value = 1;
};
template<int s, class A>
struct Sign<BigSigned<s, A> > {
    static const int value = s;
};


typedef BigSigned< 1, Zero> PositiveZero;
typedef BigSigned<-1, Zero> NegativeZero;
typedef typename Signed<One>::Result SignedOne;

template<int s, class a_T, class b_T>
struct Sum<BigSigned<s, a_T>, BigSigned<s, b_T> > {
    typedef BigSigned<s, typename Sum<a_T, b_T>::Result> Result;
};

template<int s, class a_T, class b_T>
struct Sum<BigSigned<s, a_T>, BigSigned<-s, b_T> > {
    typedef BigSigned<
        GreaterThanOrEqualTo<a_T, b_T>::value ? s : -s,
        typename std::conditional<
            GreaterThanOrEqualTo<a_T, b_T>::value,
            typename Difference<a_T, b_T>::Result,
            typename Difference<b_T, a_T>::Result
        >::type
    > Result;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Difference<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    typedef typename Sum<BigSigned<a_s, a_T>, BigSigned<-b_s, b_T> >::Result Result;
};

template<int s, class T, size_t shift>
struct ShiftLeft<BigSigned<s, T>, shift> {
    typedef BigSigned<s, typename ShiftLeft<T, shift>::Result> Result;
};

template<int s, class T, size_t shift>
struct ShiftRight<BigSigned<s, T>, shift> {
    typedef BigSigned<s, typename ShiftRight<T, shift>::Result> Result;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Product<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    typedef BigSigned<a_s * b_s, typename Product<a_T, b_T>::Result> Result;
};

template<int a_s, int b_s, class a_T, class b_T>
struct GreaterThan<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    static const bool value = a_s > b_s || (a_s == b_s && GreaterThan<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct GreaterThanOrEqualTo<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    static const bool value = a_s > b_s || (a_s == b_s && GreaterThanOrEqualTo<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct LessThan<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    static const bool value = a_s < b_s || (a_s == b_s && LessThan<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct LessThanOrEqualTo<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    static const bool value = a_s < b_s || (a_s == b_s && LessThanOrEqualTo<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct Max<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    typedef BigSigned<a_s, a_T> A;
    typedef BigSigned<b_s, b_T> B;
    typedef typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, A, B
    >::type Result;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Min<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    typedef BigSigned<a_s, a_T> A;
    typedef BigSigned<b_s, b_T> B;
    typedef typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, B, A
    >::type Result;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Division<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
private:
    typedef Division<a_T, b_T> D;
    typedef typename D::Quotient Q;
    typedef typename D::Residue R;
public:
    typedef BigSigned<a_s * b_s, Q> Quotient;
    typedef BigSigned<a_s, R> Residue;
};

// TODO mixed definitions, e. g. signed + unsigned

///////////////////////////////////////////////////////////////////////////
///////////////////////// decimal representation //////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A> struct DecimalRepresentation;

template<>
struct DecimalRepresentation<Zero> {
    static inline std::string str(void) {
        return "0";
    }
};

template<word n>
struct DecimalRepresentation<BigUnsigned<n, Zero> > {
    static inline std::string str(void) {
        return to_string(n);
    }
};

template<word n, class T>
struct DecimalRepresentation<BigUnsigned<n, T> > {
private:
    static const word modulo = 1000000000ULL * 1000000000ULL;
    static const word modulo_log = 18;
    typedef Division<BigUnsigned<n, T>, BIGUNSIGNED_1(modulo)> D;
    typedef typename D::Quotient Q;
    typedef typename D::Residue R;
    /*static_assert(R::digit < modulo, "invalid division by power of 10");*/
public:
    static std::string str(void ){
        std::string tail = DecimalRepresentation<Q>::str();
        if(tail == "0") tail = "";
        std::string curr = to_string(R::digit);
        if(tail != "")
            while(curr.size() < modulo_log)
                curr = "0" + curr;
        return tail + curr;
    }
};

///////////////////////////////////////////////////////////////////////////
/////////////////////////////// algorithms ////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<
    class R_prev, class R_curr,
    class S_prev, class S_curr,
    class T_prev, class T_curr>
struct EuclideanAlgorithmImplementation {
    /*static_assert(GreaterThanOrEqualTo<R_prev, R_curr>::value, "a must be >= b in Euclidean algorithm");*/
private:
    typedef typename Division<R_prev, R_curr>::Quotient Q;
    typedef typename Division<R_prev, R_curr>::Residue R;
    typedef EuclideanAlgorithmImplementation<
        R_curr, R,
        S_curr, typename Difference<S_prev, typename Product<Q, S_curr>::Result>::Result,
        T_curr, typename Difference<T_prev, typename Product<Q, T_curr>::Result>::Result
    > RecursiveCall;
public:
    typedef typename RecursiveCall::GCD GCD;
    typedef typename RecursiveCall::S S;
    typedef typename RecursiveCall::T T;
};

template<class R_prev, class S_prev, class S_curr, class T_prev, class T_curr>
struct EuclideanAlgorithmImplementation<R_prev, PositiveZero, S_prev, S_curr, T_prev, T_curr> {
    typedef R_prev GCD;
    typedef S_prev S;
    typedef T_prev T;
};
template<class R_prev, class S_prev, class S_curr, class T_prev, class T_curr>
struct EuclideanAlgorithmImplementation<R_prev, NegativeZero, S_prev, S_curr, T_prev, T_curr> {
    typedef R_prev GCD;
    typedef S_prev S;
    typedef T_prev T;
};

template<class A, class B>
struct EuclideanAlgorithm;

template<int a_s, int b_s, class a_T, class b_T>
struct EuclideanAlgorithm<BigSigned<a_s, a_T>, BigSigned<b_s, b_T> > {
    typedef BigSigned<a_s, a_T> A;
    typedef BigSigned<b_s, b_T> B;
private:
    static const bool swap = GreaterThan<B, A>::value;
    typedef EuclideanAlgorithmImplementation<
        typename Max<A, B>::Result,
        typename Min<A, B>::Result,
        SignedOne, PositiveZero,
        PositiveZero, SignedOne
    > Implementation;
public:
    // GCD = A * S + B * T
    typedef typename Implementation::GCD GCD;
    typedef typename std::conditional<
        swap, typename Implementation::T, typename Implementation::S>::type S;
    typedef typename std::conditional<
        swap, typename Implementation::S, typename Implementation::T>::type T;
    /*static_assert(
        EqualTo<GCD, typename Sum<
            typename Product<A, S>::Result,
            typename Product<B, T>::Result
        >::Result>::value
    , "\n\nEuclidean algorithm didn't work correctly\n");*/
};

template<word a_n, word b_n, class a_T, class b_T>
struct EuclideanAlgorithm<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    typedef BigUnsigned<a_n, a_T> A;
    typedef BigUnsigned<b_n, b_T> B;
private:
    typedef EuclideanAlgorithm<
        typename Signed<A>::Result,
        typename Signed<B>::Result
    > Implementation;
public:
    typedef typename Implementation::GCD GCD;
    typedef typename Implementation::S S;
    typedef typename Implementation::T T;
};

template<class A, class N>
struct SumUpUntilPositive;
template<bool positive, class A, class N>
struct SumUpUntilPositiveImplementation;

template<class A, class N>
struct SumUpUntilPositiveImplementation<true, A, N> {
    typedef typename Unsigned<A>::Result Result;
};
template<class A, class N>
struct SumUpUntilPositiveImplementation<false, A, N> {
    typedef typename SumUpUntilPositive<
        typename Sum<
            typename Signed<A>::Result,
            typename Signed<N>::Result
        >::Result,
        N
    >::Result Result;
};

template<class A, class N>
struct SumUpUntilPositive {
    typedef typename SumUpUntilPositiveImplementation<
        (Sign<A>::value > 0), A, N
    >::Result Result;
};

} // namespace static_bignum

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_MERSENNE_PRIME(pow) \
    SBN_DIFF(SBN_SHL(static_bignum::One, pow), static_bignum::One)
#endif

#endif