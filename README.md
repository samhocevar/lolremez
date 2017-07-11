# LolRemez

A Remez algorithm toolkit to approximate functions.

Tutorial available here: http://lolengine.net/wiki/doc/maths/remez

## Example

Approximate `atan(sqrt(3+x)-exp(1+x))` over the range `[-1,1]` with a 5th degree polynomial:

```sh
lolremez -d 5 -r -1:1 "atan(sqrt(3+x)-exp(1+x))"
```

Result:

```c++
/* Approximation of f(x) = atan(sqrt(3+x)-exp(1+x))
 * on interval [ -1.0000000000000000000, 1.0000000000000000000 ]
 * with a polynomial of degree 5. */
float f(float x)
{
    float u = -1.1514600677101831554e-1f;
    u = u * x + -3.1404157365765952659e-1f;
    u = u * x + 4.7315508009637667259e-1f;
    u = u * x + 5.7678822318891844828e-1f;
    u = u * x + -1.2480195820255797595f;
    return u * x + -7.6941172112944451609e-1f;
}
```

## Changes

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

