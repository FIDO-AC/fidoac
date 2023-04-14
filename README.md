# FIDO-AC

This repository provides FIDO-AC, an unoptimized proof-of-concept implementation prototype for the framework described in the Fast IDentity Online with Anonymous Credentials (FIDO-AC) paper.
Listed below are summaries of each implemented component and simple flowchart depicting the typical flow of the FIDO-AC system.

Description of Different Components
1) fidoac_androidapp : Android Application implementing the BAC/PACE, the ZK-Proof of the eID attribute and the local mediator.
2) fidoac_app_server: Relying Party's Server, verify FIDO.
3) fidoac_server: Verification of additional components (ZKProof and mediator).

![fidoac_simplified_impl_dia drawio](https://user-images.githubusercontent.com/13492945/232001458-3bf2bbe5-2738-4a8a-b926-86ec6210a9cc.svg)

