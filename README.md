# LolRemez

A Remez algorithm implementation to approximate functions using polynomials.

A tutorial is available [in the wiki section](https://github.com/samhocevar/lolremez/wiki).

## Example

Approximate `atan(sqrt(3+x³)-exp(1+x))` over the range `[sqrt(2),pi²]` with a 5th degree polynomial for `double` floats:

```sh
lolremez --double -d 5 -r "sqrt(2):pi²" "atan(sqrt(3+x³)-exp(1+x))"
```

Result:

```c++
/* Approximation of f(x) = atan(sqrt(3+x³)-exp(1+x))
 * on interval [ sqrt(2), pi² ]
 * with a polynomial of degree 5. */
double f(double x)
{
    double u = -3.9557569330471555e-5;
    u = u * x + 1.2947712130833294e-3;
    u = u * x + -1.6541397035559147e-2;
    u = u * x + 1.0351664953941214e-1;
    u = u * x + -3.2051562487328135e-1;
    return u * x + -1.1703528319321961;
}
```

## Available functions

Binary functions/operators:

 - \+ \- \* / %
 - *atan2(y, x)*, *pow(x, y)*
 - *min(x, y)*, *max(x, y)*
 - *fmod(x, y)*

Exponent shortcuts:

 - *x²*, *x³*…

Constants:

 - *e*,
 - *pi*, *π*
 - *tau*, *τ* (Tau is [great for trolling](https://tauday.com/tau-manifesto))

Math functions:

 - *abs()* (absolute value)
 - *sqrt()* (square root), *cbrt()* (cubic root)
 - *exp()*, *exp2()*, *erf()*, *log()*, *log2()*, *log10()*
 - *sin()*, *cos()*, *tan()*
 - *asin()*, *acos()*, *atan()*
 - *sinh()*, *cosh()*, *tanh()*

Parsing rules:

 - *-a^b* is *-(a^b)*
 - *a^b^c* is *(a^b)^c*

## Changes

### In Git trunk:

 - Implement *erf()*.
 - Implement *%* and *fmod()*.
 - lolremez can now also act as a simple command line calculator.

### News for LolRemez 0.4:

 - Allow expressions in the range specification: `-r -pi/4:pi/4` is now valid.
 - Allow using “pi” and “e” in expressions, as well as “π”.
 - Bugfixes in the expression parser.
 - Support for hexadecimal floats, *e.g.* `0x1.999999999999999ap-4`.
 - New `--float`, `--double` and `--long-double` options to choose precision.
 - Trim down DLL dependencies to allow for lighter binaries.

### News for LolRemez 0.3:

 - implemented an expression parser so that the user does not have to
   recompile the software before each use.
 - C/C++ function output.
 - use threading to find zeros and extrema.
 - use successive parabolic interpolation to find extrema.

### News for LolRemez 0.2:

 - significant performance and accuracy improvements thanks to various
   bugfixes and a better extrema finder for the error function.
 - user can now define accuracy of the final result.
 - exp(), sin(), cos() and tan() are now about 20% faster.
 - multiplying a real number by an integer power of two is now a virtually
   free operation.
 - fixed a rounding bug in the real number printing routine.

### Initial release: LolRemez 0.1

