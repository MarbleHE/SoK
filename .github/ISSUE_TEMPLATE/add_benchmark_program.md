---
name: add_benchmark_program
about: Describes the steps required for the implementation of a new combination of
  (benchmark program, tool).
title: Implement <benchmark-name> for <tool-name>
labels: ''
assignees: ''

---

# Preparation
- [ ] Describe the program to be implemented in the [Wiki](https://github.com/MarbleHE/SoK/wiki/) (see "Benchmark Programs" in Wiki Sidebar):
    - Pseudocode of program 
    - Inputs and outputs, e.g., datasets involved
    - High-level idea of what computation does

# Implementation

- [ ] Implement the program
  - Use the Docker image from the tool's sample program
  - Commit the files in the SoK repository
- [ ] Describe gained insights about the tool on the tool's Wiki page, see "Compilers & Optimizations" in Wiki Sidebar 
- [ ] Describe implementation-specific details on the program's Wiki page, including:
  - changes/approximations made to accommodate program to tool

# Automation

- [ ] Adapt Dockerfile to enable automatic benchmark runs
  - If present, remove the sample program - we don't need that it to be executed in each benchmark run
  - Test the Docker image locally, for that there exists the automation script [execute_benchmark_locally.sh](https://github.com/MarbleHE/SoK/blob/master/execute_benchmark_locally.sh)
- [ ] If this is the first implementation of the application: Write plotting code
  - See the Wiki's page [Visualization](https://github.com/MarbleHE/SoK/wiki/Visualization) for detail
- [ ] Run `run-benchmarks` workflow and check results
  - [ ] Raw output files (CSV) present?
  - [ ] Data included in generated plot?
