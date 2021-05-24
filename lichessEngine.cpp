#include <iostream>

#define AI_MAX_PLY 5

#include "include/ai.h"

int main(int argc, const char* argv[]) {
	// Only supports standard mode
	chess::game chessGame = chess::defaultGame();
	for (int moveNumber = 1; moveNumber < argc; moveNumber++) {
		chessGame.move(std::string { argv[moveNumber] });
	}
	std::string uciMove { chess::ai::bestMove(chessGame).toString() };
	std::cout << (uciMove[4] == '*' ? uciMove.substr(0, 4) : uciMove);
	return 0;
}
