#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <utility>
#include <random>
#include <algorithm>
#include <cstdlib>
#define POPULATION_SIZE 100
#include "include/geneticBot.h"
#define BATCH_SIZE 3
void evaluateFen(const chess::ai::bot& botToTrain, gameResult& botGameResult, const std::vector<std::string>& fens){
	for (auto& fen : fens) {
		chess::game gameFromFen { fen };
        auto bestContinuation = chess::ai::bestMove(gameFromFen, botToTrain);
		botGameResult.leafs += bestContinuation.leafs;
		botGameResult.nodes += bestContinuation.nodes;
	}
}

int main() {
	srand(time(nullptr));
	const std::vector<std::string> fens { {
#include "fenlist.csv"
	} };

	std::vector<chess::ai::botWeights> populationWeights;
	std::vector<chess::ai::bot> population;
	std::vector<gameResult> gameResults;

	std::ifstream populationFile { "population.bin", std::ios_base::in | std::ios_base::binary };
	if (!populationFile.good()) {
		std::cout << "File could not be opened!\n";
		return -1;
	}

	char generationMessage[19];
	int generationNumber;
	populationFile.read(generationMessage, 19);
	populationFile.read(reinterpret_cast<char*>(&generationNumber), sizeof(int));
	std::cout << generationMessage << generationNumber;
	
    populationWeights.push_back(nomalahCustomDesignedWeights);
	populationFile.read(reinterpret_cast<char*>(&populationWeights.back()), sizeof(chess::ai::botWeights)); // get best bot value

	// Set up the default population
	for (int i = 0; i < POPULATION_SIZE; i++) {
		populationWeights.push_back(populationWeights.back());
		population.push_back({ populationWeights.back() });
		gameResults.push_back({ .score = 0, .leafs = 0, .nodes = 0 });
	}
	populationWeights.pop_back();

	for (int generation = 0; generation < 10000; generation++) {
		std::cout << "Running new population\n";
		std::vector<std::thread> fenTesting;
		std::vector<std::string> fenBatch;
		for (int i = 0; i < BATCH_SIZE; i++){
			fenBatch.push_back(pickRandom(fens));
		}
		for (int i = 0; i < POPULATION_SIZE; i++) {
            std::thread startThread { evaluateFen, std::ref(population[i]), std::ref(gameResults[i]), std::ref(fenBatch) };
            fenTesting.push_back(std::move(startThread));
        }

		// Is there a way to continually have multiple games running instead of waiting for stragglers?
		for (int i = 0; i < POPULATION_SIZE; i++) {
			fenTesting[i].join();
		}
		std::cout << "Population Testing Complete\n";

		std::cout << "Sorting population based on results\n";
		sortPopulation(populationWeights, gameResults);
		std::cout << "All game results of generation " << generation + generationNumber << '\n';
		//printResults(gameResults);
		//std::cerr << "Best of generation " << generation + generationNumber << '\n';
		printWeights(populationWeights[0]);
		std::cout << "Saving sorted population\n";
		sendPopulationToFile(populationWeights, generation + generationNumber);
		std::cout << "Breeding population\n";
		breedPopulation(populationWeights, gameResults, [&](const std::vector<chess::ai::botWeights>& populationWeights, const std::vector<gameResult>& gameResults, std::vector<chess::ai::botWeights>& matingPool) {
			std::cerr << "All game results of generation " << generation + generationNumber << '\n';
			auto min = std::min_element(gameResults.begin(), gameResults.end(), [](const gameResult a, const gameResult b) { return a.nodes < b.nodes; })->nodes;
            auto max = std::max_element(gameResults.begin(), gameResults.end(), [](const gameResult a, const gameResult b) { return a.nodes < b.nodes; })->nodes;
			auto oldRange = max - min;
			auto newRange = 100 - 0;
            std::cout << "Min: " << min << " Max: " << max << " Old Range: " << oldRange << " New Range: " << newRange << '\n';
			for (int i = 0; i < gameResults.size(); i++) {
				auto weightedValue = 0;
				if (oldRange != 0) {
				    weightedValue = 100 - (((gameResults[i].nodes - min) * newRange) / oldRange) + 0;
				}
				std::cerr << "Bot: " << i << " result: " << gameResults[i].score << " evaluated: " << gameResults[i].leafs << " searched: " << gameResults[i].nodes << " ratio: " << gameResults[i].ratio() << " weighted: " << weightedValue << '\n';
				for (int j = 0; j <= weightedValue; j++) {
					matingPool.push_back(populationWeights[i]);
				}
			}
		});
		std::cout << "Mutating whole population\n";
		mutatePopulation(populationWeights, mutateMoveOrdering);
		std::cout << "Shuffling population\n";
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(populationWeights.begin(), populationWeights.end(), g);

		// Reset scores and make new population
		for (int i = 0; i < POPULATION_SIZE; i++) {
			population[i]  = { populationWeights[i] };
			gameResults[i] = { .score = 0, .leafs = 0, .nodes = 0 };
		}
	}
}