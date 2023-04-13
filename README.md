# FIDO-AC

This repository provides FIDO-AC, an unoptimized proof-of-concept implementation prototype for the framework described in the Fast IDentity Online with Anonymous Credentials (FIDO-AC) paper.
Listed below are summaries of each implemented component and simple flowchart depicting the typical flow of the FIDO-AC system.

Description of Different Components
1) fidoac_android : Android Application implementing the BAC/PACE, the ZK-Proof of the eID attribute and the local mediator.
2) fidoac_app_server: Relying Party's Server, verify FIDO.
3) fidoac_server: Verification of additional components (ZKProof and mediator).

![fidoac_simplified_impl_dia drawio](https://user-images.githubusercontent.com/13492945/231697175-393710eb-9f4e-4c00-8a42-e8c137df4cff.svg)
