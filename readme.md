# ICS4UI Chess Engine
The project has multiple sections, with the examples for each shown:
* [Chess Engine](/blob/main/engine.cpp)
* [Perft](/blob/main/perft.cpp)
* [Chess AI](/blob/main/ai.cpp)
* [Python Integration](/blob/main/engine.py)

#
## Installing and running
### - MacOS (10.13.6)
```sh
clang++ -std=c++2a -O3 /src/*.cpp perft.cpp -o perft
clang++ -std=c++2a -O3 /src/*.cpp engine.cpp -o engine
clang++ -std=c++2a -O3 /src/*.cpp ai.cpp -o ai
```