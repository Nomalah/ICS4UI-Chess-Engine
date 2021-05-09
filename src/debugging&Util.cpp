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

std::string chess::util::squareToAlgebraic(chess::u8 square) {
	std::string file[] = {"h", "g", "f", "e", "d", "c", "b", "a"};
	std::string rank[] = {"1", "2", "3", "4", "5", "6", "7", "8"};
	return file[square % 8] + rank[square / 8];
}

chess::u8 chess::util::ctz64(u64 bitboard) {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_ctzll(bitboard);
#else
	static_assert(-1);
#endif
}

chess::u8 chess::util::clz64(u64 bitboard) {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_clzll(bitboard);
#else
	static_assert(-1);
#endif
}

// Ignoring for the time being, as it is not necessary to either the AI or engine
bool chess::position::validateFen(std::string fen) {
	return true;
}

char chess::util::constants::pieceToChar(chess::util::constants::boardModifiers piece) {
	constexpr char conversionList[] = {'*', 'p', 'n', 'b', 'r', 'q', 'k', '*', '*', 'P', 'N', 'B', 'R', 'Q', 'K', '*'};
	return conversionList[piece];
}

chess::util::constants::boardModifiers chess::util::colorOf(chess::util::constants::boardModifiers piece){
	return static_cast<chess::util::constants::boardModifiers>(piece & 0x08);
}

chess::util::constants::boardModifiers chess::util::nullOf(chess::util::constants::boardModifiers piece){
	return static_cast<chess::util::constants::boardModifiers>(piece | 0x07);
}
