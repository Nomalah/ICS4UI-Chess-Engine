#ifndef NMLH_CHESS_UTILS_HPP
#define NMLH_CHESS_UTILS_HPP

#include <string>
#include <bit>

#include "chess_types.hpp"

namespace chess::util {
	[[nodiscard]] constexpr u8 algebraicToSquare(const std::string& algebraic) noexcept {
		if (algebraic.length() == 2) {
			u8 total { 0 };
			if (algebraic[0] >= 'a' && algebraic[0] <= 'h') {
				total += 7 - algebraic[0] + 'a';
				if (algebraic[1] >= '1' && algebraic[1] <= '8') {
					total += (algebraic[1] - '1') * 8;
					return total;
				}
			}
		}
		return 0x0ULL;
	};
	[[nodiscard]] std::string squareToAlgebraic(const chess::square targetSquare);
	[[nodiscard]] constexpr char pieceToChar(const chess::piece targetPiece) {
		return "*pnbrqk**PNBRQK*"[targetPiece];
	}
	[[nodiscard]] constexpr u64 bitboardFromIndex(const u8 index) { return 1ULL << index; };

	// Intrinsic wrapper functions
	// Count trailing zeros (of a number's binary representation)
	[[nodiscard]] inline chess::square ctz64(const u64 bitboard) noexcept {
        return static_cast<chess::square>(std::countr_zero(bitboard));
	}

	// Count leading zeros (of a number's binary representation)
	[[nodiscard]] inline chess::square clz64(const u64 bitboard) noexcept { 
        return static_cast<chess::square>(std::countl_zero(bitboard));
	}

	// Count the number of ones (in a number's binary representation)
	[[nodiscard]] inline auto popcnt64(const u64 bitboard) noexcept {
        return static_cast<chess::square>(std::popcount(bitboard));
	}

	// Expand to intergral types
	inline constexpr u64 zeroLSB(u64& bitboard) noexcept {
		return bitboard &= bitboard - 1;    // Should become the BLSR instruction on x86
	}

	// Functions to modify board/piece annotations
	[[nodiscard]] constexpr chess::piece colorOf(const chess::piece targetPiece) noexcept {
		return static_cast<chess::piece>(targetPiece & 0x08);
	}
	[[nodiscard]] constexpr chess::piece getPieceOf(const chess::piece targetPiece) noexcept {
		return static_cast<chess::piece>(targetPiece & 0x07);
	}
	[[nodiscard]] constexpr chess::piece operator~(const chess::piece targetPiece) noexcept {
		return static_cast<chess::piece>(targetPiece ^ chess::piece::white);
	}
	[[nodiscard]] constexpr chess::attackRayDirection operator~(const chess::attackRayDirection direction) noexcept {
		return static_cast<chess::attackRayDirection>(direction ^ chess::attackRayDirection::south);
	}

	[[nodiscard]] constexpr chess::moveFlags getCaptureFlag(chess::piece targetPiece) { return targetPiece != chess::piece::empty ? chess::moveFlags::capture : chess::moveFlags::quiet; };

	[[nodiscard]] constexpr chess::piece constructPiece(const chess::piece targetPiece, const chess::piece color) {
		return static_cast<chess::piece>(targetPiece | color);
	}

	template <chess::attackRayDirection direction>
	[[nodiscard]] constexpr chess::u64 positiveRayAttacks(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
		chess::u64 attacks { chess::constants::attackRays[direction][squareFrom] };
		chess::u64 blocker { (attacks & occupiedSquares) | 0x8000000000000000 };
		attacks ^= chess::constants::attackRays[direction][chess::util::ctz64(blocker)];
		return attacks;
	}

	template <chess::attackRayDirection direction>
	[[nodiscard]] constexpr chess::u64 negativeRayAttacks(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
		chess::u64 attacks { chess::constants::attackRays[direction][squareFrom] };
		chess::u64 blocker { (attacks & occupiedSquares) | 0x1 };
		attacks ^= chess::constants::attackRays[direction][63U - chess::util::clz64(blocker)];
		return attacks;
	}

	template <chess::attackRayDirection direction>
	[[nodiscard]] constexpr chess::u64 rayAttack(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
		if constexpr (chess::attackRayDirection::north == direction || chess::attackRayDirection::northWest == direction || chess::attackRayDirection::west == direction || chess::attackRayDirection::northEast == direction) {
			return positiveRayAttacks<direction>(squareFrom, occupiedSquares);
		} else {
			return negativeRayAttacks<direction>(squareFrom, occupiedSquares);
		}
	}

    [[nodiscard]] constexpr chess::piece nullOf(chess::piece) noexcept {
        return chess::piece::empty;
    }
}    // namespace util

#endif    // NMLH_CHESS_UTILS_HPP