# ICS4UI Chess Engine
The project has multiple sections, with the examples for each shown:
* [Chess Engine](engine.cpp)
* [Perft](perft.cpp)
* [Chess AI](ai.cpp)
* [Python Integration](engine.py)

#
## Installing and running
### - MacOS (10.13.6)
```sh
clang++ -std=c++2a -O3 /src/*.cpp perft.cpp -o perft
clang++ -std=c++2a -O3 /src/*.cpp engine.cpp -o engine
clang++ -std=c++2a -O3 /src/*.cpp ai.cpp -o ai
```