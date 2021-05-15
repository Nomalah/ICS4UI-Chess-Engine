#include <iostream>
#include <vector>

#include "include/chess.h"

int main() {
    // Desired 1 player Program
	chess::game chessGame = chess::defaultGame();
	std::string resultStr[] = {"Black Wins!", "Draw!", "White Wins!"};
    
	while (!chessGame.finished()) {
		std::vector<chess::move> validMoves = chessGame.moves();

		std::cout << chessGame.currentPosition().ascii() << std::endl;

		std::size_t moveNum = 0;
		for (auto move : validMoves) {
			std::cout << "[" << moveNum++ << "] " << move.toString() << std::endl;
		}

		std::size_t desiredMoveNum;
		std::cout << "Enter Your Desired Move" << std::endl;
		std::cin >> desiredMoveNum;
		std::cout << "[" << desiredMoveNum << "] " << validMoves[desiredMoveNum].toString() << std::endl;

		chessGame.move(validMoves[desiredMoveNum]);
	}
	std::cout << "Result: " << resultStr[chessGame.result()] << std::endl;
	return 0;
}