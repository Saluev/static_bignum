#ifndef STATIC_BIGNUM_HPP
#define STATIC_BIGNUM_HPP

#include <bitset>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace static_bignum {

// some forward declarations
template<class A> struct DecimalRepresentation;

using word = uint64_t;
static const word WORD_MAX = word(-1); // HACK
static const size_t bits_per_word = 8 * sizeof(word);

struct Zero {
    static const word digit = 0;
    static const size_t length = 0;
    using Word = word;
    static inline std::string bin(void) {
        return "";
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
    using Next = T;
    using Word = word;
    static inline std::string bin(void) {
        std::bitset<bits_per_word> bset(n);
        return Next::bin() + bset.to_string();
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

using One  = BigUnsigned<1, Zero>;

template<class A> struct Optimize;

template<>
struct Optimize<Zero> {
    using Result = Zero;
};

template<>
struct Optimize<BigUnsigned<0, Zero>> {
    using Result = Zero;
};

template<word n, class T>
struct Optimize<BigUnsigned<n, T>> {
private:
    using Tail = typename Optimize<T>::Result;
public:
    using Result = typename std::conditional<
        n == 0 && std::is_same<Tail, Zero>::value,
        Zero,
        BigUnsigned<n, Tail>
    >::type;
};

#define BIGUNSIGNED_1(x1) typename static_bignum::Optimize<static_bignum::BigUnsigned<(x1)>>::Result
//#define BIGUNSIGNED_1(x1)         static_bignum::BigUnsigned<(x1), static_bignum::NoneType>
#define BIGUNSIGNED_2(x1, x2)         static_bignum::BigUnsigned<(x2), BIGUNSIGNED_1((x1))>
#define BIGUNSIGNED_3(x1, x2, x3)     static_bignum::BigUnsigned<(x3), BIGUNSIGNED_2((x2), (x1))>
#define BIGUNSIGNED_4(x1, x2, x3, x4) static_bignum::BigUnsigned<(x4), BIGUNSIGNED_3((x3), (x2), (x1))>

///////////////////////////////////////////////////////////////////////////
//////////////////////////////// addition /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct Sum;

template<> struct Sum<Zero, Zero> { using Result = Zero; };

template<word n, class T>
struct Sum<Zero, BigUnsigned<n, T>> { using Result = BigUnsigned<n, T>; };

template<word n, class T>
struct Sum<BigUnsigned<n, T>, Zero> { using Result = BigUnsigned<n, T>; };

template<word a_n, word b_n, class a_T, class b_T>
struct Sum<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    static const word carry = b_n > WORD_MAX - a_n ? 1 : 0;
    using Result = BigUnsigned<a_n + b_n, typename Sum<
        typename Sum<a_T, b_T>::Result,
        BIGUNSIGNED_1(carry)
    >::Result>;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_SUM(x, y) static_bignum::Sum<x, y>::Result
#endif

///////////////////////////////////////////////////////////////////////////
/////////////////////////////// subtraction ///////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct Difference;

template<> struct Difference<Zero, Zero> { using Result = Zero; };

template<word n, class T>
struct Difference<BigUnsigned<n, T>, Zero> { using Result = BigUnsigned<n, T>; };

template<word n, class T>
struct Difference<Zero, BigUnsigned<n, T>> { using Result = ErrorType; };

template<class A> struct Difference<ErrorType, A> { using Result = ErrorType; };
template<class A> struct Difference<A, ErrorType> { using Result = ErrorType; };

template<word a_n, word b_n, class a_T, class b_T>
struct Difference<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    using A = BigUnsigned<a_n, a_T>;
    using B = BigUnsigned<b_n, b_T>;
    using C = typename Difference<a_T, b_T>::Result;
    using Result_T = typename std::conditional< // deciding whether carry or not
        a_n >= b_n, C, typename Difference<C, One>::Result
    >::type;
//     static const word Result_n = // it will be done automatically
//         a_n >= b_n ? (a_n - b_n) : ((UINT64_MAX - b_n) + (a_n + 1));
    using Result = typename std::conditional< // removing trailing zeros // TODO Optimize
        a_n == b_n && std::is_same<Result_T, Zero>::value,
        Zero,
        BigUnsigned<a_n - b_n, Result_T>
    >::type;
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


template<size_t shift> struct BigShiftLeft   <Zero, shift> { using Result = Zero; };
template<size_t shift> struct BigShiftRight  <Zero, shift> { using Result = Zero; };
template<size_t shift> struct SmallShiftLeft <Zero, shift> { using Result = Zero; };
template<size_t shift> struct SmallShiftRight<Zero, shift> {
    static const word carry = 0;
    using Result = Zero;
};

template<word n, class T, size_t shift>
struct BigShiftLeft<BigUnsigned<n, T>, shift> {
    using Argument = BigUnsigned<n, T>;
    using Result   = BigUnsigned<0ULL, typename BigShiftLeft<Argument, shift-1>::Result>;
};
template<word n, class T, size_t shift>
struct SmallShiftLeft<BigUnsigned<n, T>, shift> {
    static_assert(shift < bits_per_word, "shift in SmallShiftLeft must be less than bits per word");
    static const word carry = n >> (bits_per_word - shift);
    using Result = BigUnsigned<
        n << shift,
        typename Sum<
            typename SmallShiftLeft<T, shift>::Result,
            BIGUNSIGNED_1(carry)
        >::Result
    >;
};

template<word n, class T, size_t shift>
struct BigShiftRight<BigUnsigned<n, T>, shift> {
    using Argument = BigUnsigned<n, T>;
    using Result   = typename BigShiftRight<T, shift-1>::Result;
};
template<word n, class T, size_t shift>
struct SmallShiftRight<BigUnsigned<n, T>, shift> {
    static_assert(shift < bits_per_word, "shift in SmallShiftRight must be less than bits per word");
    static const word carry = n << (bits_per_word - shift);
    using Result = BigUnsigned<
        (n >> shift) | SmallShiftRight<T, shift>::carry,
        typename SmallShiftRight<T, shift>::Result
    >;
};

template<word n, class T>
struct BigShiftLeft   <BigUnsigned<n, T>, 0> { using Result = BigUnsigned<n, T>; };
template<word n, class T>
struct BigShiftRight  <BigUnsigned<n, T>, 0> { using Result = BigUnsigned<n, T>; };
template<word n, class T>
struct SmallShiftLeft <BigUnsigned<n, T>, 0> { using Result = BigUnsigned<n, T>; };
template<word n, class T>
struct SmallShiftRight<BigUnsigned<n, T>, 0> {
    static const word carry = 0;
    using Result = BigUnsigned<n, T>;
};

template<class A, size_t shift>
struct ShiftLeft {
    using Result = typename BigShiftLeft<
        typename SmallShiftLeft<A, shift % bits_per_word>::Result, shift / bits_per_word
    >::Result;
};
template<size_t shift> struct ShiftLeft<Zero, shift> { using Result = Zero; };
template<word n, class T>
struct ShiftLeft<BigUnsigned<n, T>, 0> { using Result = BigUnsigned<n, T>; };

template<class A, size_t shift>
struct ShiftRight {
    using Result = typename SmallShiftRight<
        typename BigShiftRight<A, shift / bits_per_word>::Result, shift % bits_per_word
    >::Result;
};
template<size_t shift> struct ShiftRight<Zero, shift> { using Result = Zero; };
template<word n, class T>
struct ShiftRight<BigUnsigned<n, T>, 0> { using Result = BigUnsigned<n, T>; };

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_SHL(x, y) static_bignum::ShiftLeft <x, y>::Result
#define SBN_SHR(x, y) static_bignum::ShiftRight<x, y>::Result
#endif


///////////////////////////////////////////////////////////////////////////
///////////////////////////////// product /////////////////////////////////
///////////////////////////////////////////////////////////////////////////

template<class A, class B> struct Product;

template<> struct Product<Zero, Zero>     { using Result = Zero; };
template<> struct Product<Zero, One>      { using Result = Zero; };
template<> struct Product<One, Zero>      { using Result = Zero; };
template<> struct Product<One, One>       { using Result = One;  };
template<word n, class T> struct Product<Zero, BigUnsigned<n, T>> { using Result = Zero; };
template<word n, class T> struct Product<BigUnsigned<n, T>, Zero> { using Result = Zero; };
template<word n, class T> struct Product<BigUnsigned<n, T>, One>  { using Result = BigUnsigned<n, T>; };
template<word n, class T> struct Product<One, BigUnsigned<n, T>>  { using Result = BigUnsigned<n, T>; };

template<word a_n, word b_n, class a_T, class b_T>
struct Product<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T> > {
    // a * b = P ** 2 * (a / P) * (b / P)
    //       +          (a % P) * (b % P)
    //       + P      * (a % P) * (b / P)
    //       + P      * (a / P) * (b % P)
    using A = BigUnsigned<a_n, a_T>;
    using B = BigUnsigned<b_n, b_T>;
    using Adiv2 = typename Optimize<typename SmallShiftRight<A, 1>::Result>::Result;
    using Bdiv2 = typename Optimize<typename SmallShiftRight<B, 1>::Result>::Result;
    using Amod2 = BIGUNSIGNED_1(a_n & 1);
    using Bmod2 = BIGUNSIGNED_1(b_n & 1);
    using Result = typename Sum<
        typename Sum<
            typename SmallShiftLeft<typename Product<Adiv2, Bdiv2>::Result, 2>::Result,
            BIGUNSIGNED_1((a_n & 1) * (b_n & 1))
        >::Result, typename Sum<
            typename SmallShiftLeft<typename Product<Adiv2, Bmod2>::Result, 1>::Result,
            typename SmallShiftLeft<typename Product<Amod2, Bdiv2>::Result, 1>::Result
        >::Result
    >::Result;
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
struct GreaterThan<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T>> {
    static const bool value =
        GreaterThan<a_T, b_T>::value ? true :
        (GreaterThanOrEqualTo<a_T, b_T>::value && a_n > b_n);
};

template<word a_n, word b_n, class a_T, class b_T>
struct GreaterThanOrEqualTo<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T>> {
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

template<class A, class B> struct Max;
template<class A, class B> struct Min;
template<> struct Max<Zero, Zero> { using Result = Zero; };
template<> struct Min<Zero, Zero> { using Result = Zero; };

template<word n, class T>
struct Max<BigUnsigned<n, T>, Zero> {
    using Result = BigUnsigned<n, T>;
};

template<word n, class T>
struct Max<Zero, BigUnsigned<n, T>> {
    using Result = BigUnsigned<n, T>;
};

template<word n, class T>
struct Min<BigUnsigned<n, T>, Zero> {
    using Result = Zero;
};

template<word n, class T>
struct Min<Zero, BigUnsigned<n, T>> {
    using Result = Zero;
};

template<word a_n, word b_n, class a_T, class b_T>
struct Max<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T>> {
    using A = BigUnsigned<a_n, a_T>;
    using B = BigUnsigned<b_n, b_T>;
    using Result = typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, A, B
    >::type;
};

template<word a_n, word b_n, class a_T, class b_T>
struct Min<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T>> {
    using A = BigUnsigned<a_n, a_T>;
    using B = BigUnsigned<b_n, b_T>;
    using Result = typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, B, A
    >::type;
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
    using Quotient = ErrorType;
    using Residue  = ErrorType;
};

template<word n, class T>
struct Division<BigUnsigned<n,  T>, One> {
    using Quotient = BigUnsigned<n,  T>;
    using Residue  = Zero;
};

template<class A, class B>
struct DummyDivision {
    using Quotient = Zero;
    using Residue  = A;
};

template<word a_n, word b_n, class a_T, class b_T>
struct Division<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T>> {
private:
    using A = BigUnsigned<a_n, a_T>;
    using B = BigUnsigned<b_n, b_T>;
    using D = typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value,
        Division<A, typename SmallShiftLeft<B, 1>::Result>,
        DummyDivision<A, B>
    >::type;
    using Q = typename D::Quotient;
    using R = typename D::Residue;
public:
    using Quotient = typename Sum<
        typename SmallShiftLeft<Q, 1>::Result,
        typename std::conditional<GreaterThanOrEqualTo<R, B>::value, One, Zero>::type
    >::Result;
    using Residue = typename std::conditional<
        GreaterThanOrEqualTo<R, B>::value,
        typename Difference<R, B>::Result,
        R
    >::type;
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
    static_assert(s == 1 || s == -1, "only 1 or -1 are allowed as signs");
    static const int sign = s;
    using Abs = T;
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
struct Minus<BigSigned<s, T>> {
    using Result = BigSigned<-s, T>;
};
template<word n, class T>
struct Minus<BigUnsigned<n, T>> {
    using Result = BigSigned<-1, BigUnsigned<n, T>>;
};

#ifdef STATIC_BIGNUM_USE_MACRO
#define SBN_MINUS(x) static_bignum::Minus<x>::Result
#endif

template<class A> struct Sign;
template<class A> struct Signed;
template<class A> struct Unsigned;
template<word n, class T>
struct Signed<BigUnsigned<n, T>> {
    using Result = BigSigned<1, BigUnsigned<n, T>>;
};
template<int s, class A>
struct Signed<BigSigned<s, A>> {
    using Result = BigSigned<s, A>;
};
template<word n, class T>
struct Unsigned<BigUnsigned<n, T>> {
    using Result = BigUnsigned<n, T>;
};
template<int s, class A>
struct Unsigned<BigSigned<s, A>> {
    using Result = A;
};
template<word n, class T>
struct Sign<BigUnsigned<n, T>> {
    static const int value = 1;
};
template<int s, class A>
struct Sign<BigSigned<s, A>> {
    static const int value = s;
};


using PositiveZero = BigSigned< 1, Zero>;
using NegativeZero = BigSigned<-1, Zero>;
using SignedOne = typename Signed<One>::Result;

template<int s, class a_T, class b_T>
struct Sum<BigSigned<s, a_T>, BigSigned<s, b_T>> {
    using Result = BigSigned<s, typename Sum<a_T, b_T>::Result>;
};

template<int s, class a_T, class b_T>
struct Sum<BigSigned<s, a_T>, BigSigned<-s, b_T>> {
    using Result = BigSigned<
        GreaterThanOrEqualTo<a_T, b_T>::value ? s : -s,
        typename std::conditional<
            GreaterThanOrEqualTo<a_T, b_T>::value,
            typename Difference<a_T, b_T>::Result,
            typename Difference<b_T, a_T>::Result
        >::type
    >;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Difference<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    using Result = typename Sum<BigSigned<a_s, a_T>, BigSigned<-b_s, b_T>>::Result;
};

template<int s, class T, size_t shift>
struct ShiftLeft<BigSigned<s, T>, shift> {
    using Result = BigSigned<s, typename ShiftLeft<T, shift>::Result>;
};

template<int s, class T, size_t shift>
struct ShiftRight<BigSigned<s, T>, shift> {
    using Result = BigSigned<s, typename ShiftRight<T, shift>::Result>;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Product<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    using Result = BigSigned<a_s * b_s, typename Product<a_T, b_T>::Result>;
};

template<int a_s, int b_s, class a_T, class b_T>
struct GreaterThan<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    static const bool value = a_s > b_s || (a_s == b_s && GreaterThan<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct GreaterThanOrEqualTo<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    static const bool value = a_s > b_s || (a_s == b_s && GreaterThanOrEqualTo<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct LessThan<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    static const bool value = a_s < b_s || (a_s == b_s && LessThan<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct LessThanOrEqualTo<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    static const bool value = a_s < b_s || (a_s == b_s && LessThanOrEqualTo<a_T, b_T>::value);
};

template<int a_s, int b_s, class a_T, class b_T>
struct Max<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    using A = BigSigned<a_s, a_T>;
    using B = BigSigned<b_s, b_T>;
    using Result = typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, A, B
    >::type;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Min<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    using A = BigSigned<a_s, a_T>;
    using B = BigSigned<b_s, b_T>;
    using Result = typename std::conditional<
        GreaterThanOrEqualTo<A, B>::value, B, A
    >::type;
};

template<int a_s, int b_s, class a_T, class b_T>
struct Division<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
private:
    using D = Division<a_T, b_T>;
    using Q = typename D::Quotient;
    using R = typename D::Residue;
public:
    using Quotient = BigSigned<a_s * b_s, Q>;
    using Residue  = BigSigned<a_s, R>;
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
struct DecimalRepresentation<BigUnsigned<n, Zero>> {
    static inline std::string str(void) {
        return std::to_string(n);
    }
};

template<word n, class T>
struct DecimalRepresentation<BigUnsigned<n, T>> {
private:
    static const word modulo = 1000000000ULL * 1000000000ULL;
    static const word modulo_log = 18;
    using D = Division<BigUnsigned<n, T>, BIGUNSIGNED_1(modulo)>;
    using Q = typename D::Quotient;
    using R = typename D::Residue;
    static_assert(R::digit < modulo, "invalid division by power of 10");
public:
    static std::string str(void ){
        std::string tail = DecimalRepresentation<Q>::str();
        if(tail == "0") tail = "";
        std::string curr = std::to_string(R::digit);
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
    static_assert(GreaterThanOrEqualTo<R_prev, R_curr>::value, "a must be >= b in Euclidean algorithm");
private:
    using Q = typename Division<R_prev, R_curr>::Quotient;
    using R = typename Division<R_prev, R_curr>::Residue;
    using RecursiveCall = EuclideanAlgorithmImplementation<
        R_curr, R,
        S_curr, typename Difference<S_prev, typename Product<Q, S_curr>::Result>::Result,
        T_curr, typename Difference<T_prev, typename Product<Q, T_curr>::Result>::Result
    >;
public:
    using GCD = typename RecursiveCall::GCD;
    using S   = typename RecursiveCall::S;
    using T   = typename RecursiveCall::T;
};

template<class R_prev, class S_prev, class S_curr, class T_prev, class T_curr>
struct EuclideanAlgorithmImplementation<R_prev, PositiveZero, S_prev, S_curr, T_prev, T_curr> {
    using GCD = R_prev;
    using S   = S_prev;
    using T   = T_prev;
};
template<class R_prev, class S_prev, class S_curr, class T_prev, class T_curr>
struct EuclideanAlgorithmImplementation<R_prev, NegativeZero, S_prev, S_curr, T_prev, T_curr> {
    using GCD = R_prev;
    using S   = S_prev;
    using T   = T_prev;
};

template<class A, class B>
struct EuclideanAlgorithm;

template<int a_s, int b_s, class a_T, class b_T>
struct EuclideanAlgorithm<BigSigned<a_s, a_T>, BigSigned<b_s, b_T>> {
    using A = BigSigned<a_s, a_T>;
    using B = BigSigned<b_s, b_T>;
private:
    static const bool swap = GreaterThan<B, A>::value;
    using Implementation = EuclideanAlgorithmImplementation<
        typename Max<A, B>::Result,
        typename Min<A, B>::Result,
        SignedOne, PositiveZero,
        PositiveZero, SignedOne
    >;
public:
    // GCD = A * S + B * T
    using GCD = typename Implementation::GCD;
    using S = typename std::conditional<
        swap, typename Implementation::T, typename Implementation::S>::type;
    using T = typename std::conditional<
        swap, typename Implementation::S, typename Implementation::T>::type;
};

template<word a_n, word b_n, class a_T, class b_T>
struct EuclideanAlgorithm<BigUnsigned<a_n, a_T>, BigUnsigned<b_n, b_T>> {
    using A = BigUnsigned<a_n, a_T>;
    using B = BigUnsigned<b_n, b_T>;
private:
    using Implementation = EuclideanAlgorithm<
        typename Signed<A>::Result,
        typename Signed<B>::Result
    >;
public:
    using GCD = typename Implementation::GCD;
    using S   = typename Implementation::S;
    using T   = typename Implementation::T;
};

template<class A, class N>
struct SumUpUntilPositive;
template<bool positive, class A, class N>
struct SumUpUntilPositiveImplementation;

template<class A, class N>
struct SumUpUntilPositiveImplementation<true, A, N> {
    using Result = typename Unsigned<A>::Result;
};
template<class A, class N>
struct SumUpUntilPositiveImplementation<false, A, N> {
    using Result = typename SumUpUntilPositive<
        typename Sum<
            typename Signed<A>::Result,
            typename Signed<N>::Result
        >::Result,
        N
    >::Result;
};

template<class A, class N>
struct SumUpUntilPositive {
    using Result = typename SumUpUntilPositiveImplementation<
        (Sign<A>::value > 0), A, N
    >::Result;
};

};

#endif