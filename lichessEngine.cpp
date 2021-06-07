#include <iostream>

#define AI_MAX_PLY 5

#include "include/ai.h"

int main(int argc, const char* argv[]) {
	const chess::ai::bot nomalahCustomDesignedBot {
        chess::ai::botWeights{
            .moveOrdering = {
                .pieceValues = {0, 100, 300, 320, 500, 900, 0, 0, 0, 100, 300, 320, 500, 900, 0 },
                .hashMove = 10000,
                .notHashMove = 0,
                .promotionMultiplier = 5,
                .captureMultiplier = 2,
                .kingsideCastling = 100,
                .queensideCastling = 100,
                .enPassant = 0,
                .pawnDoublePush = 0,
                .defaultMove = -100
            },
            .evaluate = {
                .pieceValues = {0, 100, 300, 320, 500, 900, 0, 0, 0, 100, 300, 320, 500, 900, 0 }
            }
        }
    };

	// Only supports standard mode
	chess::game chessGame = chess::defaultGame();
	for (int moveNumber = 1; moveNumber < argc; moveNumber++) {
		chessGame.move(std::string { argv[moveNumber] });
	}
	std::cerr << chessGame.currentPosition().ascii() << '\n';
	auto botMove { chess::ai::bestMove(chessGame, nomalahCustomDesignedBot) };
	chessGame.move(botMove);
	std::cerr << chessGame.currentPosition().ascii() << '\n';

	std::string uciMove { botMove.toString() };
	std::cout << (uciMove[4] == '*' ? uciMove.substr(0, 4) : uciMove);
	return 0;
}
