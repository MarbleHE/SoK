# Encrypt-Everything-Everywhere

The [Encrypt-Everything-Everywhere (E3)](https://ieeexplore.ieee.org/document/9133489) framework was proposed by Chielle et al. [1] in 2018.
E3 uses C++ as its input language and supports both arithmetic and boolean circuits in BFV, BGV, and CGGI. 
E3 supports SIMD operations but does not expose rotation operations, severely limiting the expressiveness.
Users must provide parameters as part of the configuration, and ciphertext maintenance operations are inserted naively. 
It uses the Synopsys Design Compiler, a proprietary tool for hardware design, to try to reduce the circuit’s multiplicative depth.
Internally, it supports a varietyof libraries, including TFHE, FHEW, HElib, and SEAL. 
E3’s [source code](https://github.com/momalab/e3) is available online and includes both examplesand documentation.

## Benchmarks
We implemented the Cardio and Chi-Squared task in E3. Switching between targeting BFV and TFHE was nearly seamless.
The Chi-Squared task doesn't benefit from SIMD operations, but Cardio can when targeting BFV.
However, our batched solution is actually incomplete: since rotations aren't supported, we had to omit the final "fold" (summing up elements from all ciphertext slots).

## References
[1] Chielle, E. et al. 2020. Encrypt-Everything-Everywhere: ISA Extensions for Private Computation. IEEE Transactions on Dependable and Secure Computing. (2020), 1–1.  
