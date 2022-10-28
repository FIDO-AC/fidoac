#!/usr/bin/env sage -python

from sage.all import *

# Base field characteristic
prime_q = 6210044120409721004947206240885978274523751269793792001
Fq = GF(prime_q)

# G1
ed_coeff_a = 1 # Not twisted
ed_coeff_d = 600581931845324488256649384912508268813600056237543024

#we_coeff_a = 
#weierstrass_curve_g1 = EllipticCurve(Fq, [0, -(coeff_d+1), 0, -4*coeff_d, 4*coeff_d*(coeff_d+1)])

# Cofactors
order_g1 = weierstrass_curve_g1.order()
# Cofactor in G1 = 4


# See: https://tools.ietf.org/id/draft-struik-lwip-curve-representations-00.html#rfc.appendix.C.3
def montgomery_to_weierstrass(A, B):
    we_a = (3 - A^2) // (3*B^2)
    we_b = (2 * A^3 - 9 * A) // 27 * B^3
    return (we_a, we_b)

# https://eprint.iacr.org/2008/013.pdf
def twisted_edwards_to_montgomery(a, d):
    mont_a = (2 * (a+d)) // (a-d)
    mont_b = 4 // (a-d)
    return (mont_a, mont_b)

def twisted_edwards_to_weierstrass(a, d):
    mont = twisted_edwards_to_montgomery(a, d)
    res = montgomery_to_weierstrass(mont[0], mont[1])
    return res
