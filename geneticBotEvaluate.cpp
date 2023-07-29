#include <fstream>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <algorithm>
#include <random>
#define AI_MAX_PLY 3
#define POPULATION_SIZE (24)
// Must be less then population_size/4
#include "include/ai.h"
#include "include/geneticBot.h"
using namespace chess;
using namespace chess::ai;
using namespace std;

int main(){
    srand(time(nullptr));
    std::vector<botWeights> populationWeights;
	std::vector<bot> population;
    std::vector<gameResult> gameResults;
    int generationNumber = 0;

    char reset;
	std::cout << "Reset population or use previous population? ";
	std::cin >> reset;
	if(reset == 'y'){
        // Set up the default population
        for (int i = 0; i < POPULATION_SIZE/2; i++){
			populationWeights.push_back(randomWeights());
			population.push_back({ populationWeights.back() });
            gameResults.push_back({ .score = 0, .leafs = 0 });
        }
        for (int i = 0; i < POPULATION_SIZE/2; i++){
			populationWeights.push_back(nomalahCustomDesignedWeights);
			population.push_back({ populationWeights.back() });
            gameResults.push_back({ .score = 0, .leafs = 0 });
        }
	}else{
        std::ifstream populationFile { "population.bin", ios_base::in | ios_base::binary };
        if(!populationFile.good()){
            std::cout << "File could not be opened!\n";
			return -1;
		}

		char generationMessage[19];
        populationFile.read(generationMessage, 19);
        populationFile.read(reinterpret_cast<char*>(&generationNumber), sizeof(int));
		std::cout << generationMessage << generationNumber;

		// Set up the default population
		for (int i = 0; i < POPULATION_SIZE; i++){
            populationWeights.push_back(nomalahCustomDesignedWeights);
			populationFile.read(reinterpret_cast<char*>(&populationWeights.back()), sizeof(botWeights));
			population.push_back({ populationWeights.back() });
			gameResults.push_back({ .score = 0, .leafs = 0, .nodes = 0});
        }
        
        populationFile.close();
    }

    for(int generation = 0; generation < 300; generation++){
        std::cout << "Running new population\n";
        std::vector<thread> games;
        // Double round robin
        for (int i = 0; i < POPULATION_SIZE; i++){
			for (int j = i+1; j < POPULATION_SIZE; j++) {
				thread startThread { evaluateMatchBetweenBotsOnThread, ref(population[i]), ref(population[j]), ref(gameResults[i]), ref(gameResults[j]), i * POPULATION_SIZE };
				games.push_back(std::move(startThread));
			}
        }

		// Is there a way to continually have multiple games running instead of waiting for stragglers?
		for (int i = 0; i < POPULATION_SIZE * POPULATION_SIZE - POPULATION_SIZE; i++) {
			games[i].join();
		}

		std::cout << "Sorting population based on results\n";
        sortPopulation(populationWeights, gameResults);
		std::cout << "All game results\n";
		printResults(gameResults);
		std::cerr << "Best of generation " << generation + generationNumber << '\n';
        printWeights(populationWeights[0]);
        std::cout << "Saving sorted population\n";
        sendPopulationToFile(populationWeights, generation + generationNumber);
        std::cout << "Breeding population\n";
        breedPopulation(populationWeights, gameResults, [](const std::vector<chess::ai::botWeights>& populationWeights, const std::vector<gameResult>& gameResults, std::vector<chess::ai::botWeights>& matingPool) {
            for (int i = 0; i < gameResults.size(); i++) {
                for (int j = 0; j <= gameResults[i].score; j++) {
                    matingPool.push_back(populationWeights[i]);
                }
            }
        });
        std::cout << "Mutating whole population\n";
        mutatePopulation(populationWeights, mutateEval);
        std::cout << "Shuffling population\n";
        std::random_device rd;
        std::mt19937 g(rd());
		std::shuffle(populationWeights.begin(), populationWeights.end(), g);

        // Reset scores and make new population
        for (int i = 0; i < POPULATION_SIZE; i++){
		    population[i] = { populationWeights[i] };
            gameResults[i] = { .score = 0, .leafs = 0, .nodes = 0 };
	    }
    }
}