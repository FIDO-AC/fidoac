#!/usr/bin/env sage -python3

"""
Simple script to reproduce steps of the bdfg21 scheme using hard-coded
tests data, matching that in test_polynomial_commitment.cpp
"""

from sage.all import *
from typing import Tuple


def bls12_377() -> Tuple[GF, GF, EllipticCurve, EllipticCurve]:
    r = 8444461749428370424248824938781546531375899335154063827935233455917409239041
    q = 258664426012969094010652733694893533536393512754914660539884262666720468348340822774968888139573360124440321458177

    Fr = GF(r)
    Fq = GF(q)

    # Fq2 = Fq[u] / [u^2 + 5]
    Fqx.<j> = PolynomialRing(Fq, 'j')
    Fq2.<u> = Fq.extension(j^2 + 5)

    G1 = EllipticCurve(Fq, [0, 1])
    g1 = G1(
            81937999373150964239938255573465948239988671502647976594219695644855304257327692006745978603320413799295628339695,
        241266749859715473739788878240585681733927191168601896383759122102112907357779751001206799952863815012735208165030)
    assert g1.order() == r
    t = G1.trace_of_frobenius()

    G2 = EllipticCurve(Fq2, [0, 1/u])
    assert (G2.order() % r) == 0
    g2 = G2(
        111583945774695116443911226257823823434468740249883042837745151039122196680777376765707574547389190084887628324746 +
        129066980656703085518157301154335215886082112524378686555873161080604845924984124025594590925548060469686767592854 * u,
        168863299724668977183029941347596462608978380503965103341003918678547611204475537878680436662916294540335494194722 +
        233892497287475762251335351893618429603672921469864392767514552093535653615809913098097380147379993375817193725968 * u)
    assert g2.order() == r

    # This is not required for now.  Disabling.

    # # Fq6 is constructed as Fq2[v]/(v^3 - u), and
    # # Fq12 is constructed as Fq6[w]/(w^2 + v)
    # # We bypass Fq6:
    # #
    # #   w^2 = -v, v^3 = u <=> w^2^3 = w^6 = (-v)^3 = -(v^3) = -u
    # #   <=> w^6 + u = 0
    # Fq2x.<l> = PolynomialRing(Fq2, 'l')
    # fq12_poly = l^6 - u
    # assert(fq12_poly.is_irreducible())
    # Fq12.<w> = Fq2.extension(fq12_poly)
    # v = (w^2)
    # assert v^3 == Fq12(u)

    # G2_untwisted = EllipticCurve(Fq12, [Fq12(0), Fq12(1)])

    # def g2_untwist(g2_pt):
    #     g2_pt_x, g2_pt_y = g2_pt.xy()
    #     return G2_untwisted(g2_pt_x * v, g2_pt_y * v * w)

    # def pairing(P, Q):
    #     P_x, P_y = P.xy()
    #     P_fq12 = G2_untwisted(Fq12(P_x), Fq12(P_y))
    #     Q_fq12 = g2_untwist(Q)
    #     return P_fq12.ate_pairing(Q_fq12, r, 12, t, q)

    # return Fr, Fq, G1, G2
    return Fr, G1, g1, G2, g2 #, pairing #, GT, gt


Fr, G1, g1, G2, g2 = bls12_377()

Frx.<x> = PolynomialRing(Fr, 'x')


"""
Utility functions
"""


def evaluate_f_set(f_set, z):
    return [f(z) for f in f_set]

def evaluate_polynomials(f_sets, z_s):
    assert len(f_sets) == len(z_s)
    return [evaluate_f_set(f_sets[i], z_s[i]) for i in range(len(f_sets))]

def compute_Z_polynomial(points):
    Z = Frx(1)
    for z in points:
        Z = Z * (x - z)

    assert Z.degree() == len(points)
    return Z

def polynomial_at_secret(poly, srs):
    poly_1 = 0 * g1
    for i in range(poly.degree() + 1):
        poly_1 = poly_1 + (int(poly[i]) * srs[i])
    return poly_1


class BDFG21:
    """
    Some steps from the BDFG21 scheme. See bdfg21.hpp.
    """

    # TODO: Accept settings (Fr, g1, etc) in a constructor. For now, take
    # settings from scope.

    def setup_from_secret(self, max_degree, secret):
        return [(secret^i)*g1 for i in range(max_degree+1)]

    def commitments(self, srs, f_sets):
        return [
            [polynomial_at_secret(f, srs) for f in f_set]
            for f_set in f_sets]

    def create_evaluation_witness_phase1(
            self, f_sets, z_s, evaluations, srs, gamma):
        """
        Return [(f/Z_T)(x)]_1 and the polynomial (f/Z_T). See bdfg21.hpp.
        """
        assert len(f_sets) == len(z_s)
        assert len(evaluations) == len(z_s)

        # Compute the polynomial f.
        i = 0
        f = Frx(0)
        f_over_Z_T_prime = Frx(0)
        for j in range(len(f_sets)):
            f_set = f_sets[j]
            evals = evaluations[j]
            assert len(f_set) == len(evals)
            T_minus_S_i = [z_s[k] for k in range(len(f_sets)) if k != j]
            Z_T_minus_S_i = compute_Z_polynomial(T_minus_S_i)

            f_j = Frx(0)
            H_j = Frx(0)
            H_j_A = Frx(0)
            H_j_B = Fr(0)

            for k in range(len(f_set)):
                gamma_pow_i = pow(gamma, i)
                poly_k = f_set[k]
                r_k = evals[k]
                f_i = (gamma_pow_i * Z_T_minus_S_i * (poly_k - r_k))
                f_j = f_j + f_i

                # Alternative way to compute f/Z_T (matching bdfg21.tcc)
                H_j_A = H_j_A + (gamma_pow_i * poly_k)
                H_j_B = H_j_B + (gamma_pow_i * r_k)

                i = i + 1

            f = f + f_j

            H_j = H_j_A - H_j_B
            G_j_div = H_j / (x - z_s[j])
            assert G_j_div.denominator().is_one()
            G_j = G_j_div.numerator()

            f_over_Z_T_prime = f_over_Z_T_prime + G_j

        Z_T = compute_Z_polynomial(z_s)
        f_over_Z_T_div = f / Z_T
        assert f_over_Z_T_div.denominator().is_one()
        f_over_Z_T = f_over_Z_T_div.numerator()
        assert f_over_Z_T == f_over_Z_T_prime

        return polynomial_at_secret(f_over_Z_T, srs), f_over_Z_T

    def create_evaluation_witness(
            self,
            f_sets,
            z_s,
            evaluations,
            srs,
            gamma,
            W,
            f_over_T,
            z):

        assert len(f_sets) == len(z_s)
        assert len(evaluations) == len(z_s)

        Z_T = compute_Z_polynomial(z_s)
        Z_T_at_z = Z_T(z)
        L = -Z_T_at_z * f_over_T

        i = 0

        for j in range(len(f_sets)):
            f_set = f_sets[j]
            evals = evaluations[j]
            assert len(f_set) == len(evals)
            T_minus_S_i = [z_s[k] for k in range(len(f_sets)) if k != j]
            Z_T_minus_S_i = compute_Z_polynomial(T_minus_S_i)
            Z_T_minus_S_i_at_z = Z_T_minus_S_i(z)

            L_j = Frx(0)

            for k in range(len(f_set)):
                gamma_pow_i = pow(gamma, i)
                poly_k = f_set[k]
                r_k = evals[k]
                L_j_k = gamma_pow_i * Z_T_minus_S_i_at_z * (poly_k - r_k)
                L_j = L_j + L_j_k

                i = i + 1

            L = L + L_j

        assert L(z) == 0
        L_over_x_minus_z_div = L / (x - z)
        assert L_over_x_minus_z_div.denominator().is_one()
        L_over_x_minus_z = L_over_x_minus_z_div.numerator()
        W_prime = polynomial_at_secret(L_over_x_minus_z, srs)

        return W, W_prime, L, L_over_x_minus_z


def bdfg21_test_data():

    secret = 7
    max_degree = 8

    f_sets = [
        [
             1 +  2*x +  3*x^2 +  4*x^3 +  5*x^4 +  6*x^5 +  7*x^6 +  8*x^7,
            11 + 12*x + 13*x^2 + 14*x^3 + 15*x^4 + 16*x^5 + 17*x^6 + 18*x^7,
            21 + 22*x + 23*x^2 + 24*x^3 + 25*x^4 + 26*x^5 + 27*x^6 + 28*x^7,
            31 + 32*x + 33*x^2 + 34*x^3 + 35*x^4 + 36*x^5 + 37*x^6 + 38*x^7,
        ],
        [
            71 + 72*x + 73*x^2 + 74*x^3 + 75*x^4 + 76*x^5 + 77*x^6 + 78*x^7,
            81 + 82*x + 83*x^2 + 84*x^3 + 85*x^4 + 86*x^5 + 87*x^6 + 88*x^7,
            91 + 92*x + 93*x^2 + 94*x^3 + 95*x^4 + 96*x^5 + 97*x^6 + 98*x^7,
        ],
        [
            41 + 42*x + 43*x^2 + 44*x^3 + 45*x^4 + 46*x^5 + 47*x^6 + 48*x^7,
            51 + 52*x + 53*x^2 + 54*x^3 + 55*x^4 + 56*x^5 + 57*x^6 + 58*x^7,
            61 + 62*x + 63*x^2 + 64*x^3 + 65*x^4 + 66*x^5 + 67*x^6 + 68*x^7,
        ],
    ]

    z_s = [ Fr(123), Fr(456), Fr(789) ]

    evaluations = evaluate_polynomials(f_sets, z_s)
    print(f"evaluations:\n  {evaluations}")

    bdfg21 = BDFG21()

    srs = bdfg21.setup_from_secret(max_degree, secret)
    gamma = Fr(54321)

    W, f_over_T = bdfg21.create_evaluation_witness_phase1(
        f_sets, z_s, evaluations, srs, gamma)
    print(f"  W = {W}")

    z = Fr(98765)
    W, W_prime, L, L_over_x_minus_z = bdfg21.create_evaluation_witness(
        f_sets, z_s, evaluations, srs, gamma, W, f_over_T, z)
    print(f"  W_prime = {W_prime}")
