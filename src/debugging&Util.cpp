#include <string>

#include "../include/chess.h"

std::string chess::debugging::bitboardToString(u64 bitBoard) {
	std::string result;
	u64 cmp = 1ULL << 63;
	for (std::size_t y = 8; y > 0; y--) {
		for (std::size_t x = 8; x > 0; x--) {
			result += bitBoard & cmp ? '1' : '0';
			cmp >>= 1;
		}
		result += '\n';
	}
	return result;
}

std::string chess::util::squareToAlgebraic(chess::u8 square) {
	std::string file[] = {"h", "g", "f", "e", "d", "c", "b", "a"};
	std::string rank[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
	return file[square % 8] + rank[square / 8];
}

// Ignoring for the time being, as it is not necessary to either the AI or engine
bool chess::position::validateFen(std::string fen) {
	return true;
}

char chess::util::constants::pieceToChar(chess::util::constants::boardAnnotations piece) {
	constexpr char conversionList[] = {'*', 'p', 'n', 'b', 'r', 'q', 'k', '*', '*', 'P', 'N', 'B', 'R', 'Q', 'K', '*'};
	return conversionList[piece];
}
