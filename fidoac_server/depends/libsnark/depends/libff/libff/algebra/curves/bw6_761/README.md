# Implementation of curve BW6_761

This implementation of `BW6_761` is adapted from Youssef El Housni's implementation (https://github.com/EYBlockchain/zk-swap-libff/tree/ey/libff/algebra/curves/bw6_761).

BW6_761 is an alternative curve to Zexe's SW6 which is faster.
This curve aims to be used along with BLS12_377 in order to build a pairing-friendly amicable chain of curves allowing for one level of recursive proof/snark composition.

## Run the sage script to generate the curve parameters

1. Make sure that you have [SageMath](https://www.sagemath.org/) installed

2. Run:
```bash
sage bw6_761.sage
```

## References:

- [Optimized and secure pairing-friendly elliptic curves suitable for one layer proof composition](https://eprint.iacr.org/2020/351.pdf)
- [Sage Implementation](https://gitlab.inria.fr/zk-curves/bw6-761/-/blob/master/sage/pairing.py)
