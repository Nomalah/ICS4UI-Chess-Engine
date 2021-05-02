#include <string>

#include "../include/chess.h"

std::string chess::debugging::convert(u64 bitBoard) {
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

chess::u8 chess::util::algebraicToSquare(std::string algebraic) {
	if (algebraic.length() == 2) {
		u8 total = 0;
		if (algebraic[0] >= 'a' && algebraic[0] <= 'h') {
			total += 7 - algebraic[0] + 'a';
			if (algebraic[1] >= '1' && algebraic[1] <= '8') {
				total += (algebraic[1] - '1') * 8;
				return 1ULL << total;
			}
		}
	}
	return 0x0ULL;
}

bool chess::position::pieceEqual(chess::position a, chess::position b) {
	return std::memcmp(&a, &b, sizeof(chess::u64) * 14);
}

// Ignoring for the time being, as it is not necessary to either the AI or engine
bool chess::position::validateFen(std::string fen) {
	return true;
}
