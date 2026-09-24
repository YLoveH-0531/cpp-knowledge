# Random

The standard library random number facilities are based on two fundamental notions:
- **Engines**: a function object that generates a uniformly distributed sequence of *unsigned integer* values.
- **Distributions**: a function object that maps a sequence of values from an engine (via a mathematical formula) onto a target statistical distribution.

## Engine

### Engine Interface

| Member | Meaning |
|---|---|
| `operator()` | generate the next value |
| `min()` / `max()` | static, closed range `[min(), max()]` of generated values |
| `seed(s)` | reseed (not on `random_device`) |
| `discard(z)` | advance the state `z` times without extracting a value (not on `random_device`) |
| `operator==` / `!=` | compare full internal state (not on `random_device`) |
| `operator<<` / `>>` | serialize/deserialize state to/from a stream (not on `random_device`) |

### Constructors

```cpp
std::mt19937 e1;              // default-seeded
std::mt19937 e2(seed);        // seeded with a single unsigned integer
std::mt19937 e3(seed_seq);    // seeded with a seed_seq (recommended for large-state engines)
e1.seed(seed);                // reseed an existing engine

std::random_device rd;        // no seed() — draws from a hardware/OS entropy source instead
```

### Notes

Each compiler designates one of these engines as the `default_random_engine` type. This type is intended to be the engine with the most generally useful properties — but the exact algorithm behind it is implementation-defined, so don't rely on it when quality or a specific sequence matters; name a concrete engine (e.g. `mt19937`) instead.

## Distributions

### Distribution Interface

| Member | Meaning |
|---|---|
| `operator()(g)` | draw one value using the stored parameters |
| `operator()(g, params)` | draw one value using one-off parameters, without altering the stored ones |
| `param()` / `param(p)` | get/set the stored parameter set |
| `min()` / `max()` | bounds of the distribution's output |
| `reset()` | clear any cached internal state (e.g. `normal_distribution`'s Box-Muller cache) |

### Constructors

```cpp
std::uniform_int_distribution<> u(0, 9);
std::uniform_real_distribution<> u(0, 1);
std::normal_distribution<> n(4, 1.5);
std::bernoulli_distribution b;         // defaults to p = 0.5
std::bernoulli_distribution b(.55);
```

## Worked Example

```cpp
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

int main()
{
    std::random_device rd;              // entropy source (slow, "true" random)
    std::mt19937 gen(rd());             // fast PRNG, seeded once

    std::uniform_int_distribution<> die(1, 6);
    std::cout << die(gen) << '\n';      // dice roll

    std::bernoulli_distribution coin(0.5);
    std::cout << coin(gen) << '\n';     // coin flip

    std::normal_distribution<> height(170.0, 8.0);
    std::cout << height(gen) << '\n';   // ~N(170, 8)

    std::vector<int> deck{1,2,3,4,5,6,7,8,9,10};
    std::shuffle(deck.begin(), deck.end(), gen);  // Fisher-Yates, gen as the bit source
}
```

## Mental Model

```text
random_device (entropy source, slow, used once)
        │
        ▼ seeds
     Engine (deterministic PRNG once seeded, e.g. mt19937)
        │  operator() → raw uniformly-distributed unsigned integers
        ▼
  Distribution (mathematical mapping, e.g. uniform_int_distribution)
        │  operator()(engine) → value shaped to the target law
        ▼
     Result (int / double / bool / ...)
```
