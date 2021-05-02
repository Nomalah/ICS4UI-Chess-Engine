#include <iostream>
#include <vector>

#include "include/chess.h"

int main() {
	chess::game testGame = chess::defaultGame();
	std::cout << testGame.currentPosition().ascii() << std::endl;

    // Desired 1 player Program
	/*chess::game chessGame = chess::defaultGame();
	std::string resultStr[] = {"Black Wins!", "Draw!", "White Wins!"};
	while (!chessGame.finished()) {
		std::vector<chess::move> validMoves = chessGame.moves();

		std::cout << chessGame.position().ascii() << std::endl;

		chess::move desiredUserMove;
        do {
            do {
                std::string desiredUserMoveStr;
                std::cout << "What is the move you would like to make? ";
                std::cin >> desiredUserMoveStr;
                desiredUserMove = chess::strToMove(desiredUserMoveStr);
            } while (!desiredUserMove.isValid());
		} while (std::find(validMoves.begin(), validMoves.end(), desiredUserMove) != validMoves.end());

		chessGame.move(desiredUserMove);
	}
	std::cout << "Result: " << resultStr[chessGame.result()] << std::endl;
    */
}