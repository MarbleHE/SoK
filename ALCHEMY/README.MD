# ALCHEMY
A Language and Compiler for Homomorphic Encryption Made easY (ALCHEMY) was proposed by Crockett et al. in 2017 [1]. 
Input programs are specified in a special DomainSpecific Language (DSL) implemented in Haskell and executed as arithmetic circuits using a custom BGV implementation using the Λ◦λ lattice crypto library [2].
While it supports SIMD batching, it does not offer an encoding/decoding API, making it difficult to use. 
ALCHEMY automatically selects suitable parameters by statically tracking the upper bound of the ciphertext error but requires user-supplied modulus candidates.
However, this approach, based on type-level arithmetic, leads to excessively long compilation times and makes ALCHEMY impractical for complex programs.

## Experiments
While opensource[3], the minimal examples are insufficient to allow non Haskell-experts to use the library, and it is therefore excluded from our experimental evaluation.
We include a ready-to-use docker image here for those that want to experiment with Alchemy, but do not provide implementations of our benchmark applications.

## References
[1] Crockett, E. et al. 2018. ALCHEMY: A Language and Compiler for Homomorphic Encryption Made easY. Proceedings of the 2018 ACM SIGSAC Conference on Computer and Communications Security (Oct. 2018), 1020–1037.
[2] Crockett, E. and Peikert, C. 2016. Λολ: Functional Lattice Cryptography. Proceedings of the 2016 ACM SIGSAC Conference on Computer and Communications Security (New York, NY, USA, Oct. 2016), 993–1005.
[3] https://github.com/cpeikert/ALCHEMY
