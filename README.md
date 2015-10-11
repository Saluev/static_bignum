# C++ Static Bignum Library

Well, this is just a header-only C++11 library for compile-time long
arithmetic computations. Provided as is under the MIT license.

## Usage

Typical usage example is as follows:

    #include <iostream>
    #define STATIC_BIGNUM_USE_MACRO // yes, pollute the global namespace
                                    // with your ugly macros, please
    #include "static_bignum.hpp"
    
    int main(int argc, char** argv) {
        using X = typename SBN_PROD(BIGUNSIGNED_1(12345678999ULL), BIGUNSIGNED_1(99987654321ULL));
        std::cout << "bin: " << X::bin() << std::endl;
        std::cout << "dec: " << X::dec() << std::endl;
    }

The program using `static_bignum.hpp` should be compiled using `--std=c++11` flag.
The output will be like

    $ ./a.out
    bin: 00000000000000000000000000000000000000000000000000000000010000101110101011110100110001101001010110100100110111001100111001100111
    dec: 1234415484110041304679 # correct!

There is also a method to save result to dynamic memory:

    uint64_t x[X::length];
    X::write_to(x);

The least significant 64 bit word will be written to x[0]. **Warning**: if using
signed computations, don't forget to take sign (`X::sign`) into account.

## API

* `BIGUNSIGNED_1, ..., BIGUNSIGNED_4`: produce a static long unsigned number.
    Last argument is the least significant 64 bit word. For example, `2 ** 65` should
    be created using `BIGUNSIGNED_2(2, 0)`.
* `BIGSIGNED_1, ..., BIGSIGNED_4`: produce a static long signed number.
    Arguments are same as for `BIGUNSIGNED_x`. The sign will be positive.
* `SBN_MINUS(X)`: produce `-X`. Turns unsigned numbers
    into signed negative, and flips sign of signed numbers.
* `SBN_SUM(X, Y)`: produce `X + Y`. Currently works only with both signed or
    both unsigned numbers.
* `SBN_DIFF(X, Y)`: produce `X - Y`. Currently works only with both signed or
    both unsigned numbers. When used with two unsigned numbers, produces an
    error if `Y > X`.
* `SBN_PROD(X, Y)`: produce `X * Y`. Currently works only with both signed or
    both unsigned numbers.
* `SBN_DIV(X, Y)`: produce `X / Y`. Currently works only with both signed or
    both unsigned numbers. Produces an error if `Y = 0`.
* `SBN_MOD(X, Y)`: produce `X % Y`. Currently works only with both signed or
    both unsigned numbers. Produces an error if `Y = 0`.
* `SBN_SHL(X, n), SBN_SHR(X, n)`: produce `X << n` or `X >> n`. Works with
    either signed or unsigned `X` and `n` small enough for `size_t`.
* `SBN_GT(X, Y), SBN_GTE(X, Y), SBN_LT(X, Y), SBN_LTE(X, Y)`: produce `true`
    if corresponding inequality holds. Currently works only with both signed
    or both unsigned numbers.

## Bugs & Contributing

There are lots of bugs, I'm sure of that. Please provide code examples
reproducing them when creating an issue.

--------------------

© 2015 Tigran Saluev
