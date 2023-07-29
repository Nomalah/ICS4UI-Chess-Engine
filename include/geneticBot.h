#include <iostream>
#include <fstream>
#include <cstdlib>
#include "ai.h"

#ifndef POPULATION_SIZE
#define POPULATION_SIZE 2
#endif
		  
#define WEIGHT_RANGE 1000
int weightRand() {
	return rand() % WEIGHT_RANGE;
}

chess::ai::botWeights randomWeights() {
	return {
		.moveOrdering = {
			.pieceValues         = { 0, weightRand(), weightRand(), weightRand(), weightRand(), weightRand(), 0, 0, 0, weightRand(), weightRand(), weightRand(), weightRand(), weightRand(), 0, 0 },
			.hashMove            = weightRand(),
			.notHashMove         = weightRand(),
			.promotionMultiplier = weightRand(),
			.captureMultiplier   = weightRand(),
			.kingsideCastling    = weightRand(),
			.queensideCastling   = weightRand(),
			.enPassant           = weightRand(),
			.pawnDoublePush      = weightRand(),
			.defaultMove         = weightRand() },
		.evaluate = { .pieceValues = { weightRand(), weightRand(), weightRand(), weightRand(), weightRand(), weightRand(), 0, 0, weightRand(), weightRand(), weightRand(), weightRand(), weightRand(), weightRand(), 0, 0 } }
	};
}

struct gameResult {
	int score;
	size_t leafs;
	size_t nodes;
	double ratio() const noexcept {
		return static_cast<double>(leafs) / static_cast<double>(nodes);
	}
};

std::array<gameResult, 2> runGameBetweenBots(const chess::ai::bot& botOne, const chess::ai::bot& botTwo) {
	std::array<gameResult, 2> result {};
	chess::game chessGame = chess::defaultGame();

	while (!chessGame.finished()) {
		auto bestContinuation = chessGame.currentPosition().turn() ? chess::ai::bestMove(chessGame, botOne) : chess::ai::bestMove(chessGame, botTwo);
		(chessGame.currentPosition().turn() ? result[0].nodes : result[1].nodes) += bestContinuation.nodes;
		(chessGame.currentPosition().turn() ? result[0].leafs : result[1].leafs) += bestContinuation.leafs;
		chessGame.move(bestContinuation.bestMove);
	}
	std::cout << chessGame.currentPosition().ascii() << '\n';

	result[0].score = 1 + chessGame.result();    // 2 for a white win, 1 for a draw, 0 for a black win
	result[1].score = 1 - chessGame.result();    // 0 for a white win, 1 for a draw, 2 for a black win
	return result;
}

void evaluateMatchBetweenBotsOnThread(const chess::ai::bot& botOne, const chess::ai::bot& botTwo, gameResult& botOneResult, gameResult& botTwoResult, int gameNumber) {
	const std::string resultStr[] = { "Black Wins!", "Draw!", "White Wins!" };
	{
		auto [whiteResult, blackResult] = runGameBetweenBots(botOne, botTwo);
		std::cout << "Game :" << gameNumber << ": Round :One: " << resultStr[whiteResult.score] << '\n';
		botOneResult.score += whiteResult.score;
		botOneResult.leafs += whiteResult.leafs;
		botOneResult.nodes += whiteResult.nodes;
		botTwoResult.score += blackResult.score;
		botTwoResult.leafs += blackResult.leafs;
		botTwoResult.nodes += blackResult.nodes;
	}
	{
		auto [whiteResult, blackResult] = runGameBetweenBots(botTwo, botOne);
		std::cout << "Game :" << gameNumber << ": Round :Two: " << resultStr[whiteResult.score] << '\n';
		botOneResult.score += whiteResult.score;
		botOneResult.leafs += whiteResult.leafs;
		botOneResult.nodes += whiteResult.nodes;
		botTwoResult.score += blackResult.score;
		botTwoResult.leafs += blackResult.leafs;
		botTwoResult.nodes += blackResult.nodes;
	}
}

void sortPopulation(std::vector<chess::ai::botWeights>& populationWeights, std::vector<gameResult>& gameResults) {
	for (int i = 0; i < POPULATION_SIZE; i++) {
		for (int j = i + 1; j < POPULATION_SIZE; j++) {
			if (gameResults[i].score < gameResults[j].score) {    // Highest score to lowest as primary rule
				SWAP(populationWeights[i], populationWeights[j]);
				SWAP(gameResults[i], gameResults[j]);
			} else if (gameResults[i].score == gameResults[j].score) {
				if (gameResults[i].nodes > gameResults[j].nodes) {    // Lowest number of nodes to highest as secondary rule
					SWAP(populationWeights[i], populationWeights[j]);
					SWAP(gameResults[i], gameResults[j]);
				}
			}
		}
	}
}

auto pickRandom(const auto& weights) {
	return weights[rand() % weights.size()];
}

chess::ai::botWeights breedWeights(const chess::ai::botWeights& parentA, const chess::ai::botWeights& parentB) {
	chess::ai::botWeights child;
	// botWeights is composed entirely of ints, so we're gonna have some unsafe fun.
	constexpr auto numberOfWeights { sizeof(chess::ai::botWeights) / sizeof(int) };

	for (int i = 0; i < numberOfWeights; ++i) {
		if (rand() % 2) {
			(reinterpret_cast<int*>(&child))[i] = (reinterpret_cast<const int* const>(&parentA))[i];
		} else {
			(reinterpret_cast<int*>(&child))[i] = (reinterpret_cast<const int* const>(&parentB))[i];
		}
	}

	return child;
}

void mutateMoveOrdering(chess::ai::botWeights& capita) {
	// botWeights is composed entirely of ints, so we're gonna have some unsafe fun.
	constexpr auto numberOfWeights { (sizeof(chess::ai::botWeights) - sizeof(decltype(capita.evaluate))) / sizeof(int) };
	constexpr auto mutationPercent = 2;    // two percent
	constexpr auto mutationMax     = WEIGHT_RANGE / 40;
	for (int i = 0; i < numberOfWeights; ++i) {
		if (rand() % (100 / mutationPercent) == 0) {
			(reinterpret_cast<int*>(&capita))[i] += rand() % mutationMax - mutationMax / 2;
		}
	}
	capita.moveOrdering.pieceValues[chess::black]       = 0;
	capita.moveOrdering.pieceValues[chess::white]       = 0;
	capita.moveOrdering.pieceValues[chess::whitePawn]   = capita.moveOrdering.pieceValues[chess::blackPawn];
	capita.moveOrdering.pieceValues[chess::whiteKnight] = capita.moveOrdering.pieceValues[chess::blackKnight];
	capita.moveOrdering.pieceValues[chess::whiteBishop] = capita.moveOrdering.pieceValues[chess::blackBishop];
	capita.moveOrdering.pieceValues[chess::whiteRook]   = capita.moveOrdering.pieceValues[chess::blackRook];
	capita.moveOrdering.pieceValues[chess::whiteQueen]  = capita.moveOrdering.pieceValues[chess::blackQueen];
	capita.moveOrdering.pieceValues[chess::blackKing]   = 0;
	capita.moveOrdering.pieceValues[chess::blackNull]   = 0;
	capita.moveOrdering.pieceValues[chess::whiteKing]   = 0;
	capita.moveOrdering.pieceValues[chess::whiteNull]   = 0;
}

void mutateEval(chess::ai::botWeights& capita) {
	// botWeights is composed entirely of ints, so we're gonna have some unsafe fun.
	constexpr auto numberOfWeights { sizeof(decltype(capita.evaluate)) / sizeof(int) };
	constexpr auto mutationPercent = 4;    // two percent
	constexpr auto mutationMax     = WEIGHT_RANGE / 40;
	for (int i = 0; i < numberOfWeights; ++i) {
		if (rand() % (100 / mutationPercent) == 0) {
			(reinterpret_cast<int*>(&capita.evaluate))[i] += rand() % mutationMax - mutationMax / 2;
		}
	}
	capita.evaluate.pieceValues[chess::blackKing]       = 0;
	capita.evaluate.pieceValues[chess::blackNull]       = 0;
	capita.evaluate.pieceValues[chess::whiteKing]       = 0;
	capita.evaluate.pieceValues[chess::whiteNull]       = 0;
	capita.evaluate.pieceValues[chess::black]           = 0;
	capita.evaluate.pieceValues[chess::white]           = 0;
	capita.evaluate.pieceValues[chess::whitePawn]       = capita.evaluate.pieceValues[chess::blackPawn];
	capita.evaluate.pieceValues[chess::whiteKnight]     = capita.evaluate.pieceValues[chess::blackKnight];
	capita.evaluate.pieceValues[chess::whiteBishop]     = capita.evaluate.pieceValues[chess::blackBishop];
	capita.evaluate.pieceValues[chess::whiteRook]       = capita.evaluate.pieceValues[chess::blackRook];
	capita.evaluate.pieceValues[chess::whiteQueen]      = capita.evaluate.pieceValues[chess::blackQueen];
}

void mutatePopulation(std::vector<chess::ai::botWeights>& populationWeights, decltype(&mutateEval) mutate) {
	for (int i = 0; i < POPULATION_SIZE; i++) {
		mutate(populationWeights[i]);
	}
}

void breedPopulation(std::vector<chess::ai::botWeights>& populationWeights, const std::vector<gameResult>& gameResults, const auto matingPoolDescriminator) {
	std::vector<chess::ai::botWeights> matingPool;
	populationWeights.clear();
	matingPoolDescriminator(populationWeights, gameResults, matingPool);
	for (int i = 0; i < POPULATION_SIZE; i++) {
		populationWeights.push_back(breedWeights(pickRandom(matingPool), pickRandom(matingPool)));
	}
}

void printWeights(const chess::ai::botWeights& weight) {
	std::cout << "Move Ordering:\n\tPiece Values:\n";
	std::string pieceNames[] = { "Black", "Black Pawn", "Black Knight", "Black Bishop", "Black Rook", "Black Queen", "Black King", "Black Null", "White", "White Pawn", "White Knight", "White Bishop", "White Rook", "White Queen", "White King", "White Null" };
	for (int i = 0; i < 16; i++) {
		std::cout << "\t\t" << pieceNames[i] << ':' << weight.moveOrdering.pieceValues[i] << '\n';
	}
	std::cout << "\tHash Move:" << weight.moveOrdering.hashMove << '\n';
	std::cout << "\tNot Hash Move:" << weight.moveOrdering.notHashMove << '\n';
	std::cout << "\tPromotion Multiplier:" << weight.moveOrdering.promotionMultiplier << '\n';
	std::cout << "\tCapture Multiplier:" << weight.moveOrdering.captureMultiplier << '\n';
	std::cout << "\tKingside Castling:" << weight.moveOrdering.kingsideCastling << '\n';
	std::cout << "\tQueenside Castling:" << weight.moveOrdering.queensideCastling << '\n';
	std::cout << "\tEn Passant:" << weight.moveOrdering.enPassant << '\n';
	std::cout << "\tDouble Pawn Push:" << weight.moveOrdering.pawnDoublePush << '\n';
	std::cout << "\tDefault Move:" << weight.moveOrdering.defaultMove << '\n';
	std::cout << "Evaluation:\n\tPiece Values:\n";
	for (int i = 0; i < 16; i++) {
		std::cout << "\t\t" << pieceNames[i] << ':' << weight.evaluate.pieceValues[i] << '\n';
	}
}

void printResults(const std::vector<gameResult>& gameResults) {
	for (int i = 0; auto& result : gameResults) {
		std::cerr << "Bot: " << i << " result: " << result.score << " evaluated: " << result.leafs << " searched: " << result.nodes << " ratio: " << result.ratio() << '\n';
		i++;
	}
}

void sendPopulationToFile(const std::vector<chess::ai::botWeights>& populationWeights, const int generationNumber) {
	std::ofstream outputFile("population.bin", std::ios_base::out | std::ios_base::binary);
	outputFile.write("Generation Number:", 19);
	outputFile.write(reinterpret_cast<const char* const>(&generationNumber), sizeof(decltype(generationNumber)));
	outputFile.write(reinterpret_cast<const char* const>(populationWeights.data()), sizeof(chess::ai::botWeights) * POPULATION_SIZE);
	outputFile.close();
}

const chess::ai::botWeights nomalahCustomDesignedWeights {
	.moveOrdering = {
		.pieceValues         = { 0, 100, 300, 320, 500, 900, 0, 0, 0, 100, 300, 320, 500, 900, 0 },
		.hashMove            = 10000,
		.notHashMove         = 0,
		.promotionMultiplier = 5,
		.captureMultiplier   = 2,
		.kingsideCastling    = 100,
		.queensideCastling   = 100,
		.enPassant           = 0,
		.pawnDoublePush      = 0,
		.defaultMove         = -100 },
	.evaluate = { .pieceValues = { 0, 100, 300, 320, 500, 900, 0, 0, 0, 100, 300, 320, 500, 900, 0 } }
};
