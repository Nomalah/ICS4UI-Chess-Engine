#include <iostream>

#include "../include/chess.h"

chess::moveList chess::game::moves() {
	// Check for 3-fold Repetition
	/*if(this->gameHistory.size() >= 5){
		if (chess::position::pieceEqual(this->gameHistory[this->gameHistory.size() - 3], this->currentPosition()) && chess::position::pieceEqual(this->gameHistory[this->gameHistory.size() - 5], this->currentPosition())){
			return {};
		}
	}*/
	// Continue with normal move generation
	return this->currentPosition().moves();
}

chess::moveList chess::position::moves() {
    using namespace chess;
    using namespace chess::util;
    using namespace chess::util::constants;

    boardAnnotations currentTurnColor = this->turn();
    u64 notCurrentColor = (currentTurnColor ? ~this->bitboards[chess::util::constants::boardAnnotations::white] : ~this->bitboards[chess::util::constants::boardAnnotations::black]);
    moveList pseudoLegalMoves; // chess::move = 4 bytes * 254 + 8 byte pointer = 1KB
	const auto importAttacksToMoves = [&](u64 attackBoard, chess::u8 pieceFrom, chess::u16 flagModifier) {
        attackBoard &= notCurrentColor;
		while (attackBoard) {
			chess::u8 pieceTo = chess::util::ctz64(attackBoard);
			boardAnnotations capturedPiece = this->pieceAtIndex[pieceTo];
			pseudoLegalMoves.append({pieceFrom, pieceTo, static_cast<u16>(flagModifier | chess::util::isPiece(capturedPiece) | ((this->pieceAtIndex[pieceFrom]) << 4) | capturedPiece)});
			attackBoard ^= chess::util::bitboardFromIndex(pieceTo);
		}
	};

    u64 remainingBishops = this->bitboards[boardAnnotations::blackBishop | currentTurnColor] | this->bitboards[boardAnnotations::blackQueen | currentTurnColor]; // Queen is a rook & bishop
    u64 remainingRooks = this->bitboards[boardAnnotations::blackRook | currentTurnColor] | this->bitboards[boardAnnotations::blackQueen | currentTurnColor]; // Queen is a rook & bishop
    u64 remainingKnights = this->bitboards[boardAnnotations::blackKnight | currentTurnColor];

    // Find bishop moves
    while (remainingBishops) {
        chess::u8 squareFrom = chess::util::ctz64(remainingBishops);
        importAttacksToMoves(bishopAttacks(squareFrom, currentTurnColor), squareFrom, 0x0);
        remainingBishops ^= chess::util::bitboardFromIndex(squareFrom);
    }

    // Find rook moves
    while (remainingRooks) {
        chess::u8 squareFrom = chess::util::ctz64(remainingRooks);
        importAttacksToMoves(rookAttacks(squareFrom, currentTurnColor), squareFrom, 0x0);
        remainingRooks ^= chess::util::bitboardFromIndex(squareFrom);
    }

    // Find knight moves
    while (remainingKnights) {
        chess::u8 squareFrom = chess::util::ctz64(remainingKnights);
        importAttacksToMoves(knightAttacks(squareFrom, currentTurnColor), squareFrom, 0x0);
        remainingKnights ^= chess::util::bitboardFromIndex(squareFrom);
    }

	if (currentTurnColor) {
		// Find king moves
		{
		    chess::u8 squareFrom = chess::util::ctz64(this->bitboards[boardAnnotations::whiteKing]);
		    importAttacksToMoves(kingAttacks(squareFrom, currentTurnColor), squareFrom, 0x0);
			chess::u64 attackBoard = this->attacks(static_cast<boardAnnotations>((currentTurnColor ^ 0x08)));
			if (this->castleWK()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::g1) | bitboardFromIndex(squareAnnotations::f1))) &&
					!(attackBoard & (bitboardFromIndex(squareAnnotations::e1) | bitboardFromIndex(squareAnnotations::f1) | bitboardFromIndex(squareAnnotations::g1)))) {
					pseudoLegalMoves.append({squareFrom, squareAnnotations::g1, static_cast<u16>(0x2000 | (boardAnnotations::whiteKing << 4))});
				}
			}
			if (this->castleWQ()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::b1) | bitboardFromIndex(squareAnnotations::c1) | bitboardFromIndex(squareAnnotations::d1))) &&
					!(attackBoard & (bitboardFromIndex(squareAnnotations::e1) | bitboardFromIndex(squareAnnotations::d1) | bitboardFromIndex(squareAnnotations::c1)))) {
					pseudoLegalMoves.append({squareFrom, squareAnnotations::c1, static_cast<u16>(0x3000 | (boardAnnotations::whiteKing << 4))});
				}
			}
		}

		// Find pawn moves
		chess::u64 remainingWhitePawns = this->bitboards[boardAnnotations::whitePawn];
		while (remainingWhitePawns) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhitePawns);
			std::array<chess::u64, 2> attackRaw = pawnAttacks(squareFrom);
			importAttacksToMoves(attackRaw[0] & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			importAttacksToMoves(attackRaw[1] & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x8000);
			if (attackRaw[0] & this->enPassantTargetSquare) {
				pseudoLegalMoves.append({squareFrom, chess::util::ctz64(this->enPassantTargetSquare), 0x6000 | (whitePawn << 4) | blackPawn});
			}
			chess::u64 promotionSquares = attackRaw[0] & 0xFF00000000000000ULL;
			while (promotionSquares) {
				chess::u8 pieceTo = chess::util::ctz64(promotionSquares);
				auto pieceCaptured = this->pieceAtIndex[pieceTo];
				auto pieceCapturedFlag = chess::util::isPiece(pieceCaptured);
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteKnight << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured)});
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteBishop << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured)});
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteRook << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured)});
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::whiteQueen << 8) | (boardAnnotations::whitePawn << 4) | pieceCaptured)});
				promotionSquares ^= chess::util::bitboardFromIndex(pieceTo);
			}

			remainingWhitePawns ^= chess::util::bitboardFromIndex(squareFrom);
		}
		
        moveList legalMoves;

		for (auto pseudoLegalMove : pseudoLegalMoves) {
			chess::position movedBoard = this->move(pseudoLegalMove);
			if (!movedBoard.squareAttacked(static_cast<squareAnnotations>(ctz64(movedBoard.bitboards[boardAnnotations::whiteKing])))) {
				legalMoves.append(pseudoLegalMove);
			}
		}
		return legalMoves;
	} else {
        // Find king moves
		{
		    chess::u8 squareFrom = chess::util::ctz64(this->bitboards[boardAnnotations::blackKing]);
		    importAttacksToMoves(kingAttacks(squareFrom, currentTurnColor), squareFrom, 0x0);
			chess::u64 attackBoard = this->attacks(static_cast<boardAnnotations>(currentTurnColor ^ 0x08));
			if (this->castleBK()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::g8) | bitboardFromIndex(squareAnnotations::f8))) && 
                    !(attackBoard & (bitboardFromIndex(squareAnnotations::e8) | bitboardFromIndex(squareAnnotations::f8) | bitboardFromIndex(squareAnnotations::g8)))){
					pseudoLegalMoves.append({squareFrom, squareAnnotations::g8, static_cast<u16>(0x4000 | (boardAnnotations::blackKing << 4))});
                }
			}
			if (this->castleBQ()) {
				if (!(this->occupied() & (bitboardFromIndex(squareAnnotations::b8) | bitboardFromIndex(squareAnnotations::c8) | bitboardFromIndex(squareAnnotations::d8))) &&
					!(attackBoard & (bitboardFromIndex(squareAnnotations::e8) | bitboardFromIndex(squareAnnotations::d8) | bitboardFromIndex(squareAnnotations::c8)))) {
					pseudoLegalMoves.append({squareFrom, squareAnnotations::c8, static_cast<u16>(0x5000 | (boardAnnotations::blackKing << 4))});
				}
			}
		}

		// Find pawn moves
		chess::u64 remainingBlackPawns = this->bitboards[boardAnnotations::blackPawn];
		while (remainingBlackPawns) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackPawns);
			std::array<chess::u64, 2> attackRaw = pawnAttacks(squareFrom);
			importAttacksToMoves(attackRaw[0] & ~0xFFULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			importAttacksToMoves(attackRaw[1] & ~0xFFULL & ~this->enPassantTargetSquare, squareFrom, 0x9000);
			if (attackRaw[0] & this->enPassantTargetSquare) {
				pseudoLegalMoves.append({squareFrom, chess::util::ctz64(this->enPassantTargetSquare), 0x7000 | (blackPawn << 4) | whitePawn});
			}
			chess::u64 promotionSquares = attackRaw[0] & 0xFFULL;
			while (promotionSquares) {
				chess::u8 pieceTo = chess::util::ctz64(promotionSquares);
				auto pieceCaptured = this->pieceAtIndex[pieceTo];
				auto pieceCapturedFlag = chess::util::isPiece(pieceCaptured);
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackBishop << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured)});
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackKnight << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured)});
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackRook << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured)});
				pseudoLegalMoves.append({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (boardAnnotations::blackQueen << 8) | (boardAnnotations::blackPawn << 4) | pieceCaptured)});
				promotionSquares ^= chess::util::bitboardFromIndex(pieceTo);
			}

			remainingBlackPawns ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Filter out Illegal moves
		moveList legalMoves;
		for (auto pseudoLegalMove : pseudoLegalMoves) {
			chess::position movedBoard = this->move(pseudoLegalMove);
			if (!movedBoard.squareAttacked(static_cast<squareAnnotations>(ctz64(movedBoard.bitboards[boardAnnotations::blackKing])))) {
				legalMoves.append(pseudoLegalMove);
			}
		}
		return legalMoves;
	}
}

// https://www.chessprogramming.org/Classical_Approach
inline chess::u64 chess::position::bishopAttacks(u8 squareFrom, chess::util::constants::boardAnnotations turn) const noexcept {
	return positiveRayAttacks<chess::util::constants::northWest>(squareFrom, turn) | positiveRayAttacks<chess::util::constants::northEast>(squareFrom, turn) | negativeRayAttacks<chess::util::constants::southWest>(squareFrom, turn) | negativeRayAttacks<chess::util::constants::southEast>(squareFrom, turn);
}

// https://www.chessprogramming.org/Classical_Approach
inline chess::u64 chess::position::rookAttacks(u8 squareFrom, chess::util::constants::boardAnnotations turn) const noexcept {
	return positiveRayAttacks<chess::util::constants::north>(squareFrom, turn) | positiveRayAttacks<chess::util::constants::west>(squareFrom, turn) | negativeRayAttacks<chess::util::constants::south>(squareFrom, turn) | negativeRayAttacks<chess::util::constants::east>(squareFrom, turn);
}

inline chess::u64 chess::position::knightAttacks(u8 squareFrom, chess::util::constants::boardAnnotations turn) const noexcept {
	return chess::util::constants::knightJumps[squareFrom];
}

inline chess::u64 chess::position::kingAttacks(u8 squareFrom, chess::util::constants::boardAnnotations turn) const noexcept {
	return chess::util::constants::kingAttacks[squareFrom];
}

inline std::array<chess::u64, 2> chess::position::pawnAttacks(u8 squareFrom) const noexcept {
	// Pawn push
	chess::u64 singlePawnPush = this->turn() ? ((1ULL << squareFrom) << 8) & this->empty() : ((1ULL << squareFrom) >> 8) & this->empty();
	chess::u64 doublePawnPush = this->turn() ? (singlePawnPush << 8) & this->empty() & 0xFF000000ULL : (singlePawnPush >> 8) & this->empty() & 0xFF00000000ULL;
	// Pawn capture
	chess::u64 pawnCapture = this->turn() ? chess::util::constants::pawnAttacks[1][squareFrom] & (this->bitboards[chess::util::constants::boardAnnotations::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][squareFrom] & (this->bitboards[chess::util::constants::boardAnnotations::white] | this->enPassantTargetSquare);
	return {singlePawnPush | pawnCapture, doublePawnPush};
}

chess::u64 chess::position::attacks(chess::util::constants::boardAnnotations turn) {
	chess::util::constants::boardAnnotations pawn = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackPawn | turn);
	chess::util::constants::boardAnnotations knight = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackKnight | turn);
	chess::util::constants::boardAnnotations bishop = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackBishop | turn);
	chess::util::constants::boardAnnotations rook = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackRook | turn);
	chess::util::constants::boardAnnotations queen = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackQueen | turn);
	chess::util::constants::boardAnnotations king = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackKing | turn);

	chess::u64 resultAttackBoard = 0;
	chess::u64 remainingBishops = this->bitboards[bishop] | this->bitboards[queen];

	while (remainingBishops) {
		chess::u8 squareFrom = chess::util::ctz64(remainingBishops);
		resultAttackBoard |= bishopAttacks(squareFrom, turn);
		remainingBishops ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find rook moves
	chess::u64 remainingRooks = this->bitboards[rook] | this->bitboards[queen];
	while (remainingRooks) {
		chess::u8 squareFrom = chess::util::ctz64(remainingRooks);
		resultAttackBoard |= rookAttacks(squareFrom, turn);
		remainingRooks ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find knight moves
	chess::u64 remainingKnights = this->bitboards[knight];
	while (remainingKnights) {
		chess::u8 squareFrom = chess::util::ctz64(remainingKnights);
		resultAttackBoard |= knightAttacks(squareFrom, turn);
		remainingKnights ^= chess::util::bitboardFromIndex(squareFrom);
	}

	resultAttackBoard |= chess::util::constants::kingAttacks[chess::util::ctz64(this->bitboards[king])];

	chess::u64 remainingPawns = this->bitboards[pawn];
	while (remainingPawns) {
		chess::u8 squareFrom = chess::util::ctz64(remainingPawns);
		resultAttackBoard |= turn ? chess::util::constants::pawnAttacks[1][squareFrom] : chess::util::constants::pawnAttacks[0][squareFrom];
		remainingPawns ^= chess::util::bitboardFromIndex(squareFrom);
	}

	return resultAttackBoard;
}

bool chess::position::squareAttacked(chess::util::constants::squareAnnotations square) { // checking white king
    //Attackers
    auto turn = this->turn(); // blacks turn
    auto reverseTurn = static_cast<chess::util::constants::boardAnnotations>(turn ^ 0x08); // white
	chess::util::constants::boardAnnotations pawn = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackPawn | turn); // whites pieces
	chess::util::constants::boardAnnotations knight = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackKnight | turn);
	chess::util::constants::boardAnnotations bishop = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackBishop | turn);
	chess::util::constants::boardAnnotations rook = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackRook | turn);
	chess::util::constants::boardAnnotations queen = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackQueen | turn);
	chess::util::constants::boardAnnotations king = static_cast<chess::util::constants::boardAnnotations>(chess::util::constants::boardAnnotations::blackKing | turn);

	return (bishopAttacks(square, reverseTurn) & (this->bitboards[bishop] | this->bitboards[queen])) || 
        (rookAttacks(square, reverseTurn) & (this->bitboards[rook] | this->bitboards[queen])) || 
        (knightAttacks(square, reverseTurn) & this->bitboards[knight]) || 
        (chess::util::constants::kingAttacks[square] & this->bitboards[king]) ||
        ((reverseTurn ? chess::util::constants::pawnAttacks[1][square] : chess::util::constants::pawnAttacks[0][square]) & this->bitboards[pawn]);
}