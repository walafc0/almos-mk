/* RAND functions using 64b INTEGERs

  F. CANTONNET - HPCL - GWU */

double randlc (double *x, double a)
{
    /* This routine returns a uniform pseudorandom double precision number in the
       range (0, 1) by using the linear congruential generator

       x_{k+1} = a x_k  (mod 2^46)

       where 0 < x_k < 2^46 and 0 < a < 2^46.  This scheme generates 2^44 numbers
       before repeating.  The argument A is the same as 'a' in the above formula,
       and X is the same as x_0.  A and X must be odd double precision integers
       in the range (1, 2^46).  The returned value RANDLC is normalized to be
       between 0 and 1, i.e. RANDLC = 2^(-46) * x_1.  X is updated to contain
       the new seed x_1, so that subsequent calls to RANDLC using the same
       arguments will generate a continuous sequence.

       This routine should produce the same results on any computer with at least
       48 mantissa bits in double precision floating point data.  On 64 bit
       systems, double precision should be disabled.

       David H. Bailey     October 26, 1990 */

    unsigned long long i246m1, Lx, La;
    double d2m46;

    d2m46 = 0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5;
    //d2m46 = pow( 0.5, 46 );

    i246m1 = 0x00003FFFFFFFFFFFLL;

    Lx = *x;
    La = a;

    Lx = (Lx*La)&i246m1;
    *x = (double) Lx;
    return (d2m46 * (*x));
}

void vranlc (int n, double *x_seed, double a, double y[]) {
    /* This routine generates N uniform pseudorandom double precision numbers in
       the range (0, 1) by using the linear congruential generator

       x_{k+1} = a x_k  (mod 2^46)

       where 0 < x_k < 2^46 and 0 < a < 2^46.  This scheme generates 2^44 numbers
       before repeating.  The argument A is the same as 'a' in the above formula,
       and X is the same as x_0.  A and X must be odd double precision integers
       in the range (1, 2^46).  The N results are placed in Y and are normalized
       to be between 0 and 1.  X is updated to contain the new seed, so that
       subsequent calls to VRANLC using the same arguments will generate a
       continuous sequence.  If N is zero, only initialization is performed, and
       the variables X, A and Y are ignored.

       This routine is the standard version designed for scalar or RISC systems.
       However, it should produce the same results on any single processor
       computer with at least 48 mantissa bits in double precision floating point
       data.  On 64 bit systems, double precision should be disabled.
     */
    int i;
    double x;
    unsigned long long i246m1, Lx, La;
    double d2m46;

    d2m46 = 0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*0.5*
        0.5*0.5*0.5*0.5*0.5*0.5;
    //  d2m46 = pow( 0.5, 46.0 );
    i246m1 = 0x00003FFFFFFFFFFFLL;

    x = *x_seed;
    Lx = x;
    La = a;

    for (i = 1; i <= n; i++)
    {
        Lx = ((Lx*La)&i246m1);
        x = (double) Lx;
        y[i] = d2m46 * x;
    }
    *x_seed = x;
}
