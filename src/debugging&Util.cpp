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

std::string chess::util::squareToAlgebraic(const chess::squareAnnotations square) {
	// clang-format off
	static constexpr const char* mapping[] {
		"h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
		"h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
        "h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
        "h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
        "h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
        "h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
        "h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
        "h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8",
	};
	// clang-format on
	return mapping[square];
}

// Ignoring for the time being, as it is not necessary to either the AI or engine
[[nodiscard]] bool chess::position::validateFen([[maybe_unused]] const std::string& fen) noexcept {
	return true;
}
