#include <iostream>
#include <vector>

#include "include/chess.hpp"

int main() {
	// One player game
	chess::game chessGame = chess::defaultGame();
	std::vector<chess::moveData> moveHistory;
	std::string resultStr[] = { "Black Wins!", "Draw!", "White Wins!" };

	while (!chessGame.finished()) {
		std::cout << chessGame.currentPosition().ascii() << std::endl;

		auto legalMoves { chessGame.moves() };
		for (int i { -1 }; auto legalMove : legalMoves) {
			std::cout << ++i << ": " << legalMove.toString() << std::endl;
		}
		int desiredIndex { 0 };
		std::cout << "Desired Move: ";
		std::cin >> desiredIndex;

		chessGame.move(legalMoves[desiredIndex]);
		moveHistory.push_back(legalMoves[desiredIndex]);
	}

	// Print the result of the game
	std::cout << "Result: " << resultStr[chessGame.result()] << std::endl;

	// Print out the game in a PGN friendly format
	for (auto it { moveHistory.begin() }; it != moveHistory.end(); it++) {
		std::cout << (it - moveHistory.begin() + 1) << ". " << it->toString() << " ";
		it++;
		if (it != moveHistory.end()) {
			std::cout << it->toString() << std::endl;
		}
	}
	return 0;
}