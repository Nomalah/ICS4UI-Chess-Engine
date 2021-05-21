#!/bin/bash

echo "Building perft"
clang++ -std=c++2a src/*.cpp perft.cpp -o perft -O3
if [ "$?" -eq 0 ]; then perftSuccess="Success"; else perftSuccess="Failure"; fi
echo "Building self-play ai"
clang++ -std=c++2a src/*.cpp ai.cpp -o ai -O3
if [ "$?" -eq 0 ]; then selfPlayAISuccess="Success"; else selfPlayAISuccess="Failure"; fi
echo "Building user v ai"
clang++ -std=c++2a src/*.cpp aiVsPlayer.cpp -o aiVsPlayer -O3
if [ "$?" -eq 0 ]; then userVsAiSuccess="Success"; else userVsAiSuccess="Failure"; fi
echo "Building self-play user"
clang++ -std=c++2a src/*.cpp engine.cpp -o engine -O3
if [ "$?" -eq 0 ]; then selfPlayUserSuccess="Success"; else selfPlayUserSuccess="Failure"; fi

echo "Compile Results:"
echo "perft: $perftSuccess"
echo "self-play ai: $selfPlayAISuccess"
echo "user v ai: $userVsAiSuccess"
echo "self-play user: $selfPlayUserSuccess"