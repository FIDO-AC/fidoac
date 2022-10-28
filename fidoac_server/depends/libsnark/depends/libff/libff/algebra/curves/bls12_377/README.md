# Implementation of the BLS12-377 pairing group

Warning: This implementation is pretty much WIP and has not received any scrutiny. Use at your own risk.

## Run the sage script to generate the curve parameters

1. Make sure that you have [SageMath](https://www.sagemath.org/) installed

2. Run:
```bash
sage bls12_377.sage
```

## References:

- [Zexe: Enabling Decentralized Private Computation](https://eprint.iacr.org/2018/962.pdf)
- [Rust Implementation](https://github.com/scipr-lab/zexe/tree/master/algebra/src/bls12_377)