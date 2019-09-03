# checkers

An implementation of a checkers artificial intelligence and game environment.

## Description

This is an implementation of a checkers artificial intelligence and game environment.  The artificial intelligence uses the minimax algorithm with alpha-beta pruning to efficiently look ahead at potential future game states.  Those game states are evaluated using a basic heuristic function to help the AI make its decisions.

The user can play against the computer or another human.  If the user chooses to play against the computer, a time limit can be set for the computer, ranging from 3 seconds to 60 seconds.  The user can even force the computer to play against itself!  Game states can be saved to and loaded from the disk as a simple text file.

## Dependencies

* gcc (with C++11 support)

## Build

```
make
```

## Usage

```
make run
```
