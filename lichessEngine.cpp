#include <iostream>

#define AI_MAX_PLY 4

#include "include/ai.h"

int main(int argc, const char* argv[]) {
	const chess::ai::bot nomalahCustomDesignedBot {
        chess::ai::botWeights{
            .moveOrdering = {
                .pieceValues = {0, 62, 295, 323, 448, 851, 0, 0, 0, 62, 295, 323, 448, 851, 0, 0 },
                .hashMove = 9942,
                .notHashMove = 7,
                .promotionMultiplier = 5,
                .captureMultiplier = 854,
                .kingsideCastling = 45,
                .queensideCastling = 29,
                .enPassant = -61,
                .pawnDoublePush = 7,
                .defaultMove = 265
            },
            .evaluate = {
                .pieceValues = {0, 100, 300, 320, 500, 900, 0, 0, 0, 100, 300, 320, 500, 900, 0, 0 }
            }
        }
    };

	// Only supports standard mode
	chess::game chessGame = chess::defaultGame();
	for (int moveNumber = 1; moveNumber < argc; moveNumber++) {
		chessGame.move(std::string { argv[moveNumber] });
	}
	std::cerr << chessGame.currentPosition().ascii() << '\n';
	auto botMove { chess::ai::bestMove(chessGame, nomalahCustomDesignedBot).bestMove };
	chessGame.move(botMove);
	std::cerr << chessGame.currentPosition().ascii() << '\n';

	std::cout << botMove.toString() << std::flush;
	return 0;
}
