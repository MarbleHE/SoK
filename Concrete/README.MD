# Concrete
[Concrete](https://homomorphicencryption.org/wp-content/uploads/2020/12/wahc20_demo_damien.pdf) was presented in 2020 by Chillotti et al. [1].
It implements an improved version of TFHE, working over integers rather than bits and introducing [programmable boostrapping](https://eprint.iacr.org/2021/091) 
that can evalaute univariate functions using (small) look-up tables [2].
Concret is implemented in Rust and the [source code](https://github.com/zama-ai/concrete) is available.

## Benchmarks
Since Concrete was released after the SoK was originally submitted, we did not implement the benchmarking applications.
However, it should be possiblet to use programmable bootstrapping to implement the cardio task efficiently, without binary emulation.

## References 
[1] Chillotti, I. et al. 2020. CONCRETE: Concrete operates oN ciphertexts rapidly by extending TfhE. WAHC 2020 – 8th Workshop on Encrypted Computing & Applied Homomorphic Cryptography (Dec. 2020).  
[2] Chillotti, I. et al. 2021. Programmable Bootstrapping Enables Efficient Homomorphic Inference of Deep Neural Networks.
