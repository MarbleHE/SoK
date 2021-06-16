# Cingulata

Cingulata (previously [Armadillo](https://dl.acm.org/doi/10.1145/2732516.2732520) ([eprint](https://eprint.iacr.org/2014/988)) was proposed in 2015 by Carpov et al. [1].
The compiler takes C++ code as input and generates a corresponding Boolean circuit. 
Cingulata implements the BFV scheme directly, using the Flint and Sage libraries for operations on polynomials. 
We refer to this built-in BFV implementation as CinguBFV. 
Cingulata also supports the CGGI scheme via the TFHE library, but advanced optimizations are not supported in this mode. 
Recent versions include [CinguParam](https://eprint.iacr.org/2019/1402) [2], which automatically determines parameters for BFV.
Cingulata inserts relinearization operations naively but tries to reduce the circuit’s multiplicative depth using the circuit optimization tool [ABC](https://people.eecs.berkeley.edu/~alanmi/abc/) [3],
which was originally designed for hardware synthesis.
However, followup work has introduced novel FHE-specific depth-reduction heuristics [4-6]. 
Cingulata’s [source code](https://github.com/CEA-LIST/Cingulata) is available under the CeCILL license and includes many examples.


## Compilation Process (BFV)

The following describes how programs for BFV are compiled by Cingulata.
The whole compilation process is fully automated using cmake/make files, and as such the following steps are enough to build Cingulata including its examples:

```
git clone https://github.com/CEA-LIST/Cingulata.git
cd Cingulata
mkdir build; cd "$_"
cmake ..
make
``` 

As an example for the following explanation, we consider the `cardio` test program that is part of Cingulata and consists of two files:

- `cardio.cxx` - The program's logic. It uses the `BitTracker` class to record operations to be transformed into their equivalent circuit at run-time.
- `run.sh.in` - A script template that automates key generation, input data encryption, execution, and decryption.

During the `cmake` step, the Makefiles are created that contain the commands required for building and optimizing the circuit. Also, the `run.sh.in` script template is configured, i.e., placeholder variables (e.g., paths) that are formatted as `@KEY_NAME@` are replaced by concrete values, resulting in the executable file `run.sh`.

Executing the `make` command starts the actual build process. Essential part of it is building the helper utilities (`apps/` directory) for encoding data into binary numbers (`helper`), en- and decryption (`encrypt`/`decrypt`), key generation (`generate_keys`), and homomorphic evaluation (`dyn_omp`). 
Furthermore, the program `cardio.cxx` is built and automatically executed, resulting in a circuit file `bfv-cardio.blif`. Thereafter, a Python script is executed that invokes the ABC circuit optimizer on that circuit, resulting in an optimized circuit `bfv-cardio-opt.blif`. A helper script (`selectParams.sh`) then uses CinguParam to derive suitable parameters for the circuit. These are stored in the `fhe_params.xml` file.

After the build process finished, the user can execute the `run.sh` file to run the program:
This first generates the required FHE keys – a public key (`fhe_key.pk`), a private key (`fhe_key.sk`), and an evaluation key (`fhe_key.evk`).
Then, the input values that are hard-coded in the script (`run.sh`) are encoded as binary numbers and encrypted by bitwise-XOR with the hard-coded Kreyvium keystream. 
A clear-text representation of that is written into `clear_data.data` for debugging purposes.
The ciphertexts that serve as the circuit inputs are written into the `input/` directory.
This directory contains a ciphertext file for each bit of an input parameter, e.g., `i:ks_X_Y` denotes the Y-th bit of the X-th input parameter.
In the cardio example, each input value is encrypted using a 8 bit binary encoding.
Afterward, the program is homomorphically executed over the encrypted inputs and the encrypted result is written into the `output/` directory in a bit-encoded style, similar as the input.
Finally, the execution's result is decrypted and printed to the screen.

### Selecting Circuits for Arithmetic Operations

There exist multiple circuits for the arithmetic operations (e.g., addition, multiplication).
In the circuit generation file it is possible to choose between a multiplicative depth-optimized circuit (`IntOpGenDepth`) or a low-sized circuit (`IntOpGenSize`).

For example, `IntOpGenDepth` uses the following circuits for the primitive operations:

```
namespace cingulata
{
  class IntOpGenDepth : public IIntOpGen {
  ...
  private:
    int_ops::SklanskyAdder      m_add;
    int_ops::Negate             m_neg;
    int_ops::WallaceMultiplier  m_mul;
    int_ops::EqualDepth         m_equal;
    int_ops::LowerCompDepth     m_lower;
  };
}
```

In the circuit generation file (e.g., `cardio.cxx`), the desired optimization strategy must be chosen by passing the strategy to the `CiContext`:

```
CiContext::set_config(
  make_shared<BitTracker>(), 
  make_shared<IntOpGenDepth>());  <---
```

### Improved CMake Integration

We have created a `FindCingulata.cmake` file that provides the necessary boilerplate to integrate Cingulata into a traditional cmake based project. 
The system searches for Cingulata in `/cingu` and exposes the necessary libraries and include files to allow successful compilation of the program logic file (e.g. `cardio.cxx`). 
Support for generating actual circuits and running them is currently missing.
Note: `FindCingulata.cmake` is not usd in the current benchmarking code, but available in commit [6631f06c](https://github.com/MarbleHE/SoK/commit/6631f06c9aab17a790e889e3a3a67bd02da38ec2) for those wanting to extend it further.


## Benchmarks
We evaluate Cingulata on the Cardio and Chi-Squared tasks, using pre-compiled circuits targeting CinguBFV.
We compare the original Cingulata ABC optimizations against versions using Lobster [4] and MultiStart Heuristics [5]. 
Since the code for the latter isn't available, we use the implementation of the technique from the [Lobster codebase](https://github.com/ropas/PLDI2020_242_artifact_publication).
We also evaluate versions of those optimized circuits using CinguParam generated parameters instead of the Cingulata defaults.

### Cardio
Summarized, our benchmark considers the following configurations:

| Folder Name                      | Description                                                                                                                    | Num. Circuits |
| -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ | ------------- |
| cardio-cingulata                 | The Cardio circuit generated by Cingulata and the ABC-optimized circuit therefrom. Both use the original Cingulata parameters. | 2             |
| cardio-lobster-baseline          | The Cardio circuit used by Lobster and the ABC-optimized circuit therefrom. Both use the original Cingulata parameters.        | 2             |
| cardio-lobster                   | The Lobster-optimized circuit with the original Cingulata parameters.                                                          | 1             |
| cardio-lobster-optimal-params    | The Lobster-optimized circuit with custom parameters determined by CinguParam.                                                 | 1             |
| cardio-multistart                | The Multi-Start-optimized circuit with the original Cingulata parameters.                                                      | 1             |
| cardio-multistart-optimal-params | The Multi-Start-optimized circuit with custom parameters determined by CinguParam.                                             | 1             |

#### Cingulata Version

The implementation in Cingulata uses an illegal transformation of the initial rule that is described in the paper ([*Practical privacy-preserving medical diagnosis using homomorphic encryption*](https://doi.org/10.1109/CLOUD.2016.0084)):

```
Original rule:
+1 if weight > height-90

--— is not equal to ----

Cingulata's Cardio implementation:
+1 if height+10 < weight
```

Also, the addition (`+10`) did not work properly as `10` must be declared as a `CiInt` to make use of the overloaded addition.
The error was fixed by changing the rule to `+1 if height < (weight+90)` and declaring `90` as `CiInt`.
This change is also considered in the implementation of cardio in all other tools.

Cingulata encodes the inputs as 7*8 bit = 56 bits. The 1-bit flags (`sex`, `antecedents`, `smoker`, `diabetes`, and `high_blood_pressure`) are encoded in a single 8-bit ciphertext.

At the end of the test run, a file `fhe_parameters.txt` is created containing the FHE parameters that looks as follow:

```
== FHE parameters ====
n: 16384
q: {40,50,56,60,60,60} (326 bit)
T: 2
```

The parameters are extracted from the file `fhe_params.xml`, created by CinguParam, and is also included in our SoK repository.
The `fhe_parameters.txt` file is uploaded to the respective folder in the S3 bucket for documentation purposes.

#### Lobster Baseline

For comparison, we executed the original Lobster circuit and the Multi-Start circuit using the Lobster code base. 

The following results show the Lobster benchmarks executed on the same AWS machine as our benchmarks use (m5n.large):

```
root@dfcf63f5b942:~/Lobster# ./gen_table_rewriting.sh

Optimization logs will be saved in paper_result directory

  Name      Old Depth            Carpov.el.al                       Lobster
                           New Depth      Opt Time(s)      New Depth      Opt Time(s)
cardio.eqn         10              9              20s              8              32s
dsort.eqn           9              8           1m 25s              8           2m 18s
hd01.eqn            6              6               2s              6              22s
hd02.eqn            6              6               3s              6              33s
hd03.eqn            5              5               1s              5               5s
hd04.eqn           10              9               3s              8              16s
hd05.eqn            7              7               9s              7           3m  5s
hd06.eqn            7              7               9s              7           4m 35s
hd07.eqn            5              5               0s              3               4s
hd08.eqn            6              5               1s              5               3s
hd09.eqn           14             12               6s             12           2m 23s
hd10.eqn            6              5               1s              5               5s
hd11.eqn           18             17               9s             15           1m 12s
hd12.eqn           16             15               9s             15              42s
bar.eqn            12             12              57s       ... aborted because would take too long!
```

For the circuit evaluation, we obtained the following results:

```
root@dfcf63f5b942:~/Lobster# ./gen_table_eval_time.sh

Evaluation result will be saved in paper_result directory

 Name                             Eval.Time
                   Original     Baseline-opted   Lobster-opted
cardio.eqn          14m 54s            10m 42s          7m 12s
dsort.eqn           11m 26s             8m 51s          8m 54s
hd01.eqn                  -                  -               -
hd02.eqn                  -                  -               -
hd03.eqn                  -                  -               -
hd04.eqn             8m 28s             5m 25s          3m 24s
hd05.eqn                  -                  -               -
hd06.eqn                  -                  -               -
hd07.eqn             1m  1s                  -             24s
hd08.eqn             2m 24s             1m  3s          1m  2s
hd09.eqn            13m 33s             9m 58s          9m 30s
hd10.eqn             3m  3s             1m 30s          1m 28s
... aborted manually.
```

Notably, the execution time of the Cardio circuit is much higher than using Cingulata. Although Lobster uses an outdated version of HElib from 2017 (derived from license in header files), we do not expect such a large difference in the circuit's evaluation time. However, as the circuits in Lobster are specified on a very low-level and thus not easily comparable with the Cingulata circuit, we cannot verify whether this circuit is correct or if there are any other issues.

### Chi-Squared
As the input circuit is already multiplicative depth-optimized, Cingulata's optimizations cannot improve the circuit.
Instead, the *optimized circuit* has a larger multiplicative depth (38 instead of 16), although also significantly less
expensive AND gates (277 instead of 1698).

```
Input circuit
	x depth 16, x gates 796, + gates 1698               <-- Original circuit
Executing optimization command ""
	x depth 15, x gates 576, + gates 1286 *
Executing optimization command "resyn2"
	x depth 40, x gates 894, + gates 326
Executing optimization command "resyn2rs"
	x depth 35, x gates 903, + gates 297
Executing optimization command "recadd3;resyn2rs"
	x depth 38, x gates 2770, + gates 277               <-- optimized circuit
```


## References
[1] Carpov, S. et al. 2015. Armadillo: A Compilation Chain for Privacy Preserving Applications. Proceedings of the 3rd International Workshop on Security in Cloud Computing (New York, NY, USA, 2015), 13–19.  
[2] Herbert, V. 2019. Automatize parameter tuning in Ring-Learning-With-Errors-based leveled homomorphic cryptosystem implementations.  
[3] A. Mishchenko, 2018. ABC: System for sequential logic synthesis and formal verification. Online https://github.com/berkeleyabc/abc  
[4] Lee, D. et al. 2020. Optimizing homomorphic evaluation circuits by program synthesis and term rewriting. Proceedings of the 41st ACM SIGPLAN Conference on Programming Language Design and Implementation (New York, NY, USA, Jun. 2020), 503–518.  
[5] Carpov, S. et al. 2018. A Multi-start Heuristic for Multiplicative Depth Minimization of Boolean Circuits. Combinatorial Algorithms (2018), 275–286.  
[6] Aubry, P. et al. 2019. Faster Homomorphic Encryption is not Enough: Improved Heuristic for Multiplicative Depth Minimization of Boolean Circuits. Cryptology ePrint Archive, Report 2019/963.  
