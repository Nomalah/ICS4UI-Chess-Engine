#include <iostream>
#include <vector>

#include "include/chess.h"
#include "include/ai.h"

int main() {
        // AI self play
	chess::game chessGame = chess::defaultGame();
	std::vector<chess::moveData> moveHistory;
	std::string resultStr[] = { "Black Wins!", "Draw!", "White Wins!" };

	while (!chessGame.finished()) {
		std::cout << chessGame.currentPosition().ascii() << std::endl;
		auto bestContinuation = chess::ai::bestMove(chessGame);
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