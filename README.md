# Simple File System

Implementation of a simple file system with indexed allocation

## Contents

- simplefs.c
- simplefs.h
- Makefile
- test.c (contains the experiments for the tests)
- report.pdf (contains experimental results)

## How to Run

- cd to the project directory.
- compilation and linking done by:

```
$ make
```

##### Recompile

```
$ make clean
$ make
```

##### Running the program

```
$ ./create_format <FILENAME> <SIZE>
$ ./app <FILENAME>
```

##### Running the test (experimentation)

```
$ ./test
```