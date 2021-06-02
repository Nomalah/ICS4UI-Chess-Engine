#include <iostream>

#define AI_MAX_PLY 5

#include "include/ai.h"

int main(int argc, const char* argv[]) {
	// Only supports standard mode
	chess::game chessGame = chess::defaultGame();
	for (int moveNumber = 1; moveNumber < argc; moveNumber++) {
		chessGame.move(std::string { argv[moveNumber] });
	}
	std::cerr << chessGame.currentPosition().ascii() << '\n';
	auto botMove { chess::ai::bestMove(chessGame) };
	chessGame.move(botMove);
	std::cerr << chessGame.currentPosition().ascii() << '\n';

	std::string uciMove { botMove.toString() };
	std::cout << (uciMove[4] == '*' ? uciMove.substr(0, 4) : uciMove);
	return 0;
}
