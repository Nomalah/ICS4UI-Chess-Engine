#include <iostream>

#include "../include/chess.h"

[[nodiscard]] chess::staticVector<chess::moveData> chess::game::moves() const noexcept {
	if (this->threeFoldRep() || this->currentPosition().halfMoveClock >= 50) {
		return staticVector<chess::moveData>();
	}
	// Continue with normal move generation
	return this->currentPosition().moves();
}

[[nodiscard]] chess::staticVector<chess::moveData> chess::position::moves() const noexcept {
	using namespace chess;
	using namespace chess::util;
	using namespace chess::util::constants;

	boardAnnotations currentTurnColor { this->turn() };
	u64 notCurrentColor { ~this->bitboards[currentTurnColor] };
	staticVector<moveData> pseudoLegalMoves;    // chess::move = 4 bytes * 254 + 8 byte pointer = 1KB

	const auto importAttacksToMoves = [&](u64 attackBoard, const u8 pieceFrom, const u16 flagModifier) -> void {
		attackBoard &= notCurrentColor;    // Clear blockers that were the same colour as the moving side
		while (attackBoard) {
			const u8 pieceTo { ctz64(attackBoard) };
			const boardAnnotations capturedPiece { this->pieceAtIndex[pieceTo] };
			pseudoLegalMoves.append({ .originIndex      = pieceFrom,
			                          .destinationIndex = pieceTo,
			                          .flags            = static_cast<u16>(flagModifier | isPiece(capturedPiece) | ((this->pieceAtIndex[pieceFrom]) << 4) | capturedPiece) });
			attackBoard ^= bitboardFromIndex(pieceTo);
		}
	};

	// Find bishop moves
	for (u64 remainingBishops { this->bitboards[boardAnnotations::blackBishop | currentTurnColor] | this->bitboards[boardAnnotations::blackQueen | currentTurnColor] }; remainingBishops;) {
		const u8 squareFrom { ctz64(remainingBishops) };
		importAttacksToMoves(bishopAttacks(squareFrom), squareFrom, 0x0);
		remainingBishops ^= bitboardFromIndex(squareFrom);
	}

	// Find rook moves
	for (u64 remainingRooks { this->bitboards[boardAnnotations::blackRook | currentTurnColor] | this->bitboards[boardAnnotations::blackQueen | currentTurnColor] }; remainingRooks;) {
		const u8 squareFrom { ctz64(remainingRooks) };
		importAttacksToMoves(rookAttacks(squareFrom), squareFrom, 0x0);
		remainingRooks ^= bitboardFromIndex(squareFrom);
	}

	// Find knight moves
	for (u64 remainingKnights { this->bitboards[boardAnnotations::blackKnight | currentTurnColor] }; remainingKnights;) {
		const u8 squareFrom { ctz64(remainingKnights) };
		importAttacksToMoves(knightAttacks(squareFrom), squareFrom, 0x0);
		remainingKnights ^= bitboardFromIndex(squareFrom);
	}

	if (currentTurnColor) {
		// Find king moves
		{
			u8 squareFrom { ctz64(this->bitboards[boardAnnotations::whiteKing]) };
			importAttacksToMoves(kingAttacks(squareFrom), squareFrom, 0x0);
			u64 attackBoard { this->attacks(boardAnnotations::black) };
			if (this->castleWK()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::g1) | bitboardFromIndex(squareAnnotations::f1))) &&
				    !(attackBoard & (bitboardFromIndex(squareAnnotations::e1) | bitboardFromIndex(squareAnnotations::f1) | bitboardFromIndex(squareAnnotations::g1)))) {
					pseudoLegalMoves.append({ .originIndex      = squareFrom,
					                          .destinationIndex = squareAnnotations::g1,
					                          .flags            = static_cast<u16>(0x2000 | (boardAnnotations::whiteKing << 4)) });
				}
			}
			if (this->castleWQ()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::b1) | bitboardFromIndex(squareAnnotations::c1) | bitboardFromIndex(squareAnnotations::d1))) &&
				    !(attackBoard & (bitboardFromIndex(squareAnnotations::e1) | bitboardFromIndex(squareAnnotations::d1) | bitboardFromIndex(squareAnnotations::c1)))) {
					pseudoLegalMoves.append({ .originIndex      = squareFrom,
					                          .destinationIndex = squareAnnotations::c1,
					                          .flags            = static_cast<u16>(0x3000 | (boardAnnotations::whiteKing << 4)) });
				}
			}
		}

		// Find pawn moves
		for (u64 remainingWhitePawns { this->bitboards[boardAnnotations::whitePawn] }; remainingWhitePawns;) {
			u8 squareFrom { ctz64(remainingWhitePawns) };
			std::array<u64, 2> attackRaw { pawnAttacks(squareFrom) };
			importAttacksToMoves(attackRaw[0] & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			importAttacksToMoves(attackRaw[1] & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x8000);
			if (attackRaw[0] & this->enPassantTargetSquare) {
				pseudoLegalMoves.append({ .originIndex      = squareFrom,
				                          .destinationIndex = ctz64(this->enPassantTargetSquare),
				                          .flags            = static_cast<u16>(0x6000 | (whitePawn << 4) | blackPawn) });
			}

			for (u64 promotionSquares { attackRaw[0] & 0xFF00000000000000ULL }; promotionSquares;) {
				u8 pieceTo { ctz64(promotionSquares) };
				auto pieceCaptured { this->pieceAtIndex[pieceTo] };
				auto pieceCapturedFlag { isPiece(pieceCaptured) };
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteKnight << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured) });
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteBishop << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured) });
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteRook << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured) });
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteQueen << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured) });
				promotionSquares ^= bitboardFromIndex(pieceTo);
			}

			remainingWhitePawns ^= bitboardFromIndex(squareFrom);
		}

		staticVector<moveData> legalMoves;

		for (auto pseudoLegalMove : pseudoLegalMoves) {
			position movedBoard { this->move(pseudoLegalMove) };
			if (!movedBoard.squareAttacked(static_cast<squareAnnotations>(ctz64(movedBoard.bitboards[boardAnnotations::whiteKing])))) {
				legalMoves.append(pseudoLegalMove);
			}
		}
		return legalMoves;
	} else {
		// Find king moves
		{
			u8 squareFrom { ctz64(this->bitboards[boardAnnotations::blackKing]) };
			importAttacksToMoves(kingAttacks(squareFrom), squareFrom, 0x0);
			u64 attackBoard { this->attacks(boardAnnotations::white) };
			if (this->castleBK()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::g8) | bitboardFromIndex(squareAnnotations::f8))) &&
				    !(attackBoard & (bitboardFromIndex(squareAnnotations::e8) | bitboardFromIndex(squareAnnotations::f8) | bitboardFromIndex(squareAnnotations::g8)))) {
					pseudoLegalMoves.append({ .originIndex      = squareFrom,
					                          .destinationIndex = squareAnnotations::g8,
					                          .flags            = static_cast<u16>(0x4000 | (boardAnnotations::blackKing << 4)) });
				}
			}
			if (this->castleBQ()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::b8) | bitboardFromIndex(squareAnnotations::c8) | bitboardFromIndex(squareAnnotations::d8))) &&
				    !(attackBoard & (bitboardFromIndex(squareAnnotations::e8) | bitboardFromIndex(squareAnnotations::d8) | bitboardFromIndex(squareAnnotations::c8)))) {
					pseudoLegalMoves.append({ .originIndex      = squareFrom,
					                          .destinationIndex = squareAnnotations::c8,
					                          .flags            = static_cast<u16>(0x5000 | (boardAnnotations::blackKing << 4)) });
				}
			}
		}

		// Find pawn moves
		for (u64 remainingBlackPawns { this->bitboards[boardAnnotations::blackPawn] }; remainingBlackPawns;) {
			u8 squareFrom { ctz64(remainingBlackPawns) };
			std::array<u64, 2> attackRaw { pawnAttacks(squareFrom) };
			importAttacksToMoves(attackRaw[0] & ~0xFFULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			importAttacksToMoves(attackRaw[1] & ~0xFFULL & ~this->enPassantTargetSquare, squareFrom, 0x9000);
			if (attackRaw[0] & this->enPassantTargetSquare) {
				pseudoLegalMoves.append({ .originIndex      = squareFrom,
				                          .destinationIndex = ctz64(this->enPassantTargetSquare),
				                          .flags            = static_cast<u16>(0x7000 | (blackPawn << 4) | whitePawn) });
			}

			for (u64 promotionSquares = attackRaw[0] & 0xFFULL; promotionSquares;) {
				u8 pieceTo { ctz64(promotionSquares) };
				auto pieceCaptured { this->pieceAtIndex[pieceTo] };
				auto pieceCapturedFlag { isPiece(pieceCaptured) };
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackBishop << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured) });
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackKnight << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured) });
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackRook << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured) });
				pseudoLegalMoves.append({ squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackQueen << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured) });
				promotionSquares ^= bitboardFromIndex(pieceTo);
			}

			remainingBlackPawns ^= bitboardFromIndex(squareFrom);
		}

		// Filter out Illegal moves
		staticVector<moveData> legalMoves;
		for (auto pseudoLegalMove : pseudoLegalMoves) {
			position movedBoard { this->move(pseudoLegalMove) };
			if (!movedBoard.squareAttacked(static_cast<squareAnnotations>(ctz64(movedBoard.bitboards[boardAnnotations::blackKing])))) {
				legalMoves.append(pseudoLegalMove);
			}
		}
		return legalMoves;
	}
}

// https://www.chessprogramming.org/Classical_Approach
[[nodiscard]] inline chess::u64 chess::position::bishopAttacks(const u8 squareFrom) const noexcept {
	return positiveRayAttacks<chess::northWest>(squareFrom) | positiveRayAttacks<chess::northEast>(squareFrom) | negativeRayAttacks<chess::southWest>(squareFrom) | negativeRayAttacks<chess::southEast>(squareFrom);
}

// https://www.chessprogramming.org/Classical_Approach
[[nodiscard]] inline chess::u64 chess::position::rookAttacks(const u8 squareFrom) const noexcept {
	return positiveRayAttacks<chess::north>(squareFrom) | positiveRayAttacks<chess::west>(squareFrom) | negativeRayAttacks<chess::south>(squareFrom) | negativeRayAttacks<chess::east>(squareFrom);
}

[[nodiscard]] inline chess::u64 chess::position::knightAttacks(const u8 squareFrom) const noexcept {
	return chess::util::constants::knightJumps[squareFrom];
}

[[nodiscard]] inline chess::u64 chess::position::kingAttacks(const u8 squareFrom) const noexcept {
	return chess::util::constants::kingAttacks[squareFrom];
}

[[nodiscard]] inline std::array<chess::u64, 2> chess::position::pawnAttacks(const u8 squareFrom) const noexcept {
	// Pawn push
	chess::u64 singlePawnPush = this->turn() ? ((1ULL << squareFrom) << 8) & this->empty() : ((1ULL << squareFrom) >> 8) & this->empty();
	chess::u64 doublePawnPush = this->turn() ? (singlePawnPush << 8) & this->empty() & 0xFF000000ULL : (singlePawnPush >> 8) & this->empty() & 0xFF00000000ULL;
	// Pawn capture
	chess::u64 pawnCapture = this->turn() ? chess::util::constants::pawnAttacks[1][squareFrom] & (this->bitboards[chess::boardAnnotations::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][squareFrom] & (this->bitboards[chess::boardAnnotations::white] | this->enPassantTargetSquare);
	return { singlePawnPush | pawnCapture, doublePawnPush };
}

[[nodiscard]] chess::u64 chess::position::attacks(const chess::boardAnnotations turn) const noexcept {
	chess::boardAnnotations pawn   = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackPawn | turn);
	chess::boardAnnotations knight = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKnight | turn);
	chess::boardAnnotations bishop = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackBishop | turn);
	chess::boardAnnotations rook   = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackRook | turn);
	chess::boardAnnotations queen  = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackQueen | turn);
	chess::boardAnnotations king   = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKing | turn);

	chess::u64 resultAttackBoard { 0 };

	// Find bishop moves
	for (chess::u64 remainingBishops { this->bitboards[bishop] | this->bitboards[queen] }; remainingBishops;) {
		chess::u8 squareFrom { chess::util::ctz64(remainingBishops) };
		resultAttackBoard |= bishopAttacks(squareFrom);
		remainingBishops ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find rook moves
	for (chess::u64 remainingRooks { this->bitboards[rook] | this->bitboards[queen] }; remainingRooks;) {
		chess::u8 squareFrom { chess::util::ctz64(remainingRooks) };
		resultAttackBoard |= rookAttacks(squareFrom);
		remainingRooks ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find knight moves
	for (chess::u64 remainingKnights { this->bitboards[knight] }; remainingKnights;) {
		chess::u8 squareFrom { chess::util::ctz64(remainingKnights) };
		resultAttackBoard |= knightAttacks(squareFrom);
		remainingKnights ^= chess::util::bitboardFromIndex(squareFrom);
	}

	resultAttackBoard |= chess::util::constants::kingAttacks[chess::util::ctz64(this->bitboards[king])];

	for (chess::u64 remainingPawns { this->bitboards[pawn] }; remainingPawns;) {
		chess::u8 squareFrom { chess::util::ctz64(remainingPawns) };
		resultAttackBoard |= turn ? chess::util::constants::pawnAttacks[1][squareFrom] : chess::util::constants::pawnAttacks[0][squareFrom];
		remainingPawns ^= chess::util::bitboardFromIndex(squareFrom);
	}

	return resultAttackBoard;
}

[[nodiscard]] bool chess::position::squareAttacked(const chess::squareAnnotations square) const noexcept {    // checking white king
	//Attackers
	auto turn { this->turn() };                                                                                          // blacks turn
	auto reverseTurn { chess::util::oppositeColorOf(turn) };                                                             // white
	chess::boardAnnotations pawn { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackPawn | turn) };    // whites pieces
	chess::boardAnnotations knight { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKnight | turn) };
	chess::boardAnnotations bishop { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackBishop | turn) };
	chess::boardAnnotations rook { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackRook | turn) };
	chess::boardAnnotations queen { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackQueen | turn) };
	chess::boardAnnotations king { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKing | turn) };

	return (bishopAttacks(square) & (this->bitboards[bishop] | this->bitboards[queen])) ||
	       (rookAttacks(square) & (this->bitboards[rook] | this->bitboards[queen])) ||
	       (knightAttacks(square) & this->bitboards[knight]) ||
	       (chess::util::constants::kingAttacks[square] & this->bitboards[king]) ||
	       ((reverseTurn ? chess::util::constants::pawnAttacks[1][square] : chess::util::constants::pawnAttacks[0][square]) & this->bitboards[pawn]);
}