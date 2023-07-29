#include <iostream>
#include <vector>

#include "include/chess.hpp"
#include "include/ai.hpp"

int main() {
	// AI self play
	const chess::ai::bot nomalahCustomDesignedBot {
		chess::ai::botWeights {
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
			.evaluate = { .pieceValues = { 0, 100, 300, 320, 500, 900, 0, 0, 0, 100, 300, 320, 500, 900, 0 } } }
	};
	chess::game chessGame = chess::defaultGame();
	std::vector<chess::moveData> moveHistory;
	std::string resultStr[] = { "Black Wins!", "Draw!", "White Wins!" };

	while (!chessGame.finished()) {
		std::cout << chessGame.currentPosition().ascii() << std::endl;
		auto bestContinuation = chess::ai::bestMove(chessGame, nomalahCustomDesignedBot);
		std::cout << "Computer Move: " << bestContinuation.toString() << std::endl;
		chessGame.move(bestContinuation);
		moveHistory.push_back(bestContinuation);
	}

	// Print the result of the game
	std::cout << "Result: " << resultStr[chessGame.result()] << std::endl;

	// Print out the game in a PGN friendly format
	for (auto it { moveHistory.begin() }; it != moveHistory.end(); it++) {
		std::cout << (it - moveHistory.begin() + 2) / 2 << ". " << it->toString() << " ";
		it++;
		if (it != moveHistory.end()) {
			std::cout << it->toString() << std::endl;
		}
	}

	return 0;
}