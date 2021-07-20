#ifndef NMLH_CHESS_CONSTANTS_HPP
#define NMLH_CHESS_CONSTANTS_HPP

#include "chess_types.hpp"

namespace chess::constants {
	constexpr chess::u64 bitboardFull { 0xFFFFFFFFFFFFFFFFULL };
	constexpr chess::u64 bitboardIter { 1ULL << 63 };
    constexpr chess::u64 maxMoves { 218 }; // Maximum number of moves reachable in a position

	namespace {
		constexpr std::array<std::array<chess::u64, 64>, 8> generateAttackRays() {
			std::array<std::array<chess::u64, 64>, 8> resultRays {};
			constexpr chess::u64 horizontal { 0x00000000000000FFULL };
			constexpr chess::u64 vertical { 0x0101010101010101ULL };
			constexpr chess::u64 diagonal { 0x0102040810204080ULL };
			constexpr chess::u64 antiDiagonal { 0x8040201008040201ULL };
			for (std::size_t y { 0 }; y < 8; y++) {
				for (std::size_t x { 0 }; x < 8; x++) {
					resultRays[chess::attackRayDirection::north][y * 8 + x]     = ((vertical << x) ^ (1ULL << (y * 8 + x))) & (chess::constants::bitboardFull << (y * 8));
					resultRays[chess::attackRayDirection::south][y * 8 + x]     = ((vertical << x) ^ (1ULL << (y * 8 + x))) & ~(chess::constants::bitboardFull << (y * 8));
					resultRays[chess::attackRayDirection::east][y * 8 + x]      = ((horizontal << y * 8) ^ (1ULL << (y * 8 + x))) & ~(chess::constants::bitboardFull << (y * 8 + x));
					resultRays[chess::attackRayDirection::west][y * 8 + x]      = ((horizontal << y * 8) ^ (1ULL << (y * 8 + x))) & (chess::constants::bitboardFull << (y * 8 + x));
					resultRays[chess::attackRayDirection::northEast][y * 8 + x] = ((x + y > 6 ? diagonal << (8 * (x + y - 7)) : diagonal >> (8 * (7 - x - y))) ^ (1ULL << (y * 8 + x))) & (chess::constants::bitboardFull << (y * 8));
					resultRays[chess::attackRayDirection::southWest][y * 8 + x] = ((x + y > 6 ? diagonal << (8 * (x + y - 7)) : diagonal >> (8 * (7 - x - y))) ^ (1ULL << (y * 8 + x))) & ~(chess::constants::bitboardFull << (y * 8));
					resultRays[chess::attackRayDirection::southEast][y * 8 + x] = ((y > x ? antiDiagonal << (8 * (y - x)) : antiDiagonal >> (8 * (x - y))) ^ (1ULL << (y * 8 + x))) & ~(chess::constants::bitboardFull << (y * 8 + x));
					resultRays[chess::attackRayDirection::northWest][y * 8 + x] = ((y > x ? antiDiagonal << (8 * (y - x)) : antiDiagonal >> (8 * (x - y))) ^ (1ULL << (y * 8 + x))) & (chess::constants::bitboardFull << (y * 8 + x));
				}
			}

			return resultRays;
		}

		constexpr std::array<chess::u64, 64> generateKnightJumps() {
			constexpr std::array<chess::u64, 8> fileMask { { 0xC0C0C0C0C0C0C0C0ULL, 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0101010101010101ULL, 0x0303030303030303ULL } };
			std::array<chess::u64, 64> resultJumps {};
			chess::u64 defaultJump { 0x0000142200221400ULL };    // Attacks from e4
			for (chess::square i { chess::square::h1 }; i <= chess::square::f4; ++i) {
				resultJumps[i] = (defaultJump >> (chess::square::e4 - i)) & ~fileMask[i % 8];
			}

			for (chess::square i { chess::square::e4 }; i <= chess::square::a8; ++i) {
				resultJumps[i] = (defaultJump << (i - e4)) & ~fileMask[i % 8];
			}

			return resultJumps;
		}

		constexpr std::array<chess::u64, 64> generateKingAttacks() {
			constexpr std::array<chess::u64, 8> fileMask { { 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL } };
			std::array<chess::u64, 64> resultJumps {};
			chess::u64 defaultAttack { 0x0000001C141C0000ULL };    // Attacks from e4
			for (chess::square i { chess::square::h1 }; i <= chess::square::f4; ++i) {
				resultJumps[i] = (defaultAttack >> (chess::square::e4 - i)) & ~fileMask[i % 8];
			}

			for (chess::square i { chess::square::e4 }; i <= chess::square::a8; ++i) {
				resultJumps[i] = (defaultAttack << (i - e4)) & ~fileMask[i % 8];
			}

			return resultJumps;
		}

		constexpr std::array<std::array<chess::u64, 64>, 2> generatePawnAttacks() {
			constexpr std::array<chess::u64, 8> fileMask { { 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL } };
			std::array<std::array<chess::u64, 64>, 2> resultAttacks {};
			constexpr chess::u64 defaultAttackWhite { 0x0000000000000280ULL };
			for (chess::square i { chess::square::h1 }; i <= chess::square::a8; ++i) {
				resultAttacks[1][i] = (defaultAttackWhite << i) & ~fileMask[i % 8];
			}

			constexpr chess::u64 defaultAttackBlack { 0x0140000000000000ULL };
			for (chess::square i { chess::square::h1 }; i <= chess::square::a8; ++i) {
				resultAttacks[0][i] = (defaultAttackBlack >> (chess::square::a8 - i)) & ~fileMask[i % 8];
			}

			return resultAttacks;
		}

		constexpr std::array<std::array<chess::u64, 16>, 64> generateZobristBitStrings() {
			constexpr auto time_from_string { [](const char* str, int offset) {
				return static_cast<std::uint32_t>(str[offset] - '0') * 10 + static_cast<std::uint32_t>(str[offset + 1] - '0');
			} };

			std::array<std::array<chess::u64, 16>, 64> result {};
			// Seed the random number generation
			auto previous = [&time_from_string]() -> chess::u64 {
				return time_from_string(__TIME__, 0) * 360  /* hours */
				       + time_from_string(__TIME__, 3) * 60 /* minutes */
				       + time_from_string(__TIME__, 6) /* seconds */;
			}();
			for (auto& targetSquare : result) {
				for (auto& targetPiece : targetSquare) {
					targetPiece = previous;
					previous    = ((137 * previous + 457) % 922372036854775808ULL);
					targetPiece ^= previous << 16;
					previous = ((137 * previous + 457) % 922372036854775808ULL);
					targetPiece ^= previous << 32;
					previous = ((137 * previous + 457) % 922372036854775808ULL);
				}
			}
			return result;
		}
	}    // namespace

	constexpr std::array<std::array<chess::u64, 64>, 8> attackRays { generateAttackRays() };
	constexpr std::array<chess::u64, 64> kingAttacks { generateKingAttacks() };
	constexpr std::array<chess::u64, 64> knightJumps { generateKnightJumps() };
	constexpr std::array<std::array<chess::u64, 64>, 2> pawnAttacks { generatePawnAttacks() };

	constexpr std::array<std::array<chess::u64, 16>, 64> zobristBitStrings { generateZobristBitStrings() };
}    // namespace chess::constants

#endif    // NMLH_CHESS_CONSTANTS_HPP