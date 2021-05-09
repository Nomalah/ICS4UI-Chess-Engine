#include "../include/chess.h"

std::vector<chess::move> chess::game::moves() {
	// Check for 3-fold Repetition
	/*if(this->gameHistory.size() >= 5){
		if (chess::position::pieceEqual(this->gameHistory[this->gameHistory.size() - 3], this->currentPosition()) && chess::position::pieceEqual(this->gameHistory[this->gameHistory.size() - 5], this->currentPosition())){
			return {};
		}
	}*/
	// Continue with normal move generation
	return this->currentPosition().moves();
}

std::vector<chess::move> chess::position::moves() {
	std::vector<chess::move> result;
	const auto importAttacksToMoves = [&](chess::u64 attackBoard, chess::u8 pieceFrom, chess::u16 flagModifier) {
		while (attackBoard) {
			chess::u8 pieceTo = chess::util::ctz64(attackBoard);
			chess::util::constants::boardModifiers capturedPiece = pieceAtIndex(pieceTo);
			result.push_back({pieceFrom, pieceTo, static_cast<u16>(flagModifier | chess::util::isPiece(capturedPiece) | (pieceAtIndex(pieceFrom) << 4) | capturedPiece)});
			attackBoard ^= chess::util::bitboardFromIndex(pieceTo);
		}
	};

	if (this->turn()) {
		// Find bishop moves
		chess::u64 remainingWhiteBishops = this->bitboards[chess::util::constants::boardModifiers::whiteBishop];
		while (remainingWhiteBishops) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteBishops);
			importAttacksToMoves(bishopAttacks(squareFrom), squareFrom, 0x0);
			remainingWhiteBishops ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find rook moves
		chess::u64 remainingWhiteRooks = this->bitboards[chess::util::constants::boardModifiers::whiteRook];
		while (remainingWhiteRooks) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteRooks);
			importAttacksToMoves(rookAttacks(squareFrom), squareFrom, 0x0);
			remainingWhiteRooks ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find queen moves
		chess::u64 remainingWhiteQueens = this->bitboards[chess::util::constants::boardModifiers::whiteQueen];
		while (remainingWhiteQueens) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteQueens);
			importAttacksToMoves(bishopAttacks(squareFrom) | rookAttacks(squareFrom), squareFrom, 0x0);

			remainingWhiteQueens ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find knight moves
		chess::u64 remainingWhiteKnights = this->bitboards[chess::util::constants::boardModifiers::whiteKnight];
		while (remainingWhiteKnights) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteKnights);
			importAttacksToMoves(knightAttacks(squareFrom), squareFrom, 0x0);
			remainingWhiteKnights ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find king moves
		chess::u8 squareFrom = chess::util::ctz64(this->bitboards[chess::util::constants::boardModifiers::whiteKing]);
		importAttacksToMoves(kingAttacks(squareFrom), squareFrom, 0x0);
		if (this->castleBK()) {
			if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("g1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("f1"))))){
				result.push_back({squareFrom, chess::util::algebraicToSquare("g1"), static_cast<u16>(0x2000 | (chess::util::constants::boardModifiers::whiteKing << 4))});
            }
		}
		if (this->castleBQ()) {
			if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("b1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("c1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("d1"))))){
				result.push_back({squareFrom, chess::util::algebraicToSquare("c1"), static_cast<u16>(0x3000 | (chess::util::constants::boardModifiers::whiteKing << 4))});
            }
		}

		// Find pawn moves
		chess::u64 remainingWhitePawns = this->bitboards[chess::util::constants::boardModifiers::whitePawn];
		while (remainingWhitePawns) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhitePawns);
			chess::u64 attackRaw = pawnAttacks(squareFrom);
			importAttacksToMoves(attackRaw & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			if (attackRaw & this->enPassantTargetSquare) {
				result.push_back({squareFrom, chess::util::ctz64(this->enPassantTargetSquare), (chess::util::constants::blackPawn << 4) | chess::util::constants::whitePawn});
			}
			chess::u64 promotionSquares = attackRaw & 0xFF00000000000000ULL;
			while (promotionSquares) {
				chess::u8 pieceTo = chess::util::ctz64(promotionSquares);
				auto pieceCaptured = pieceAtIndex(pieceTo);
				auto pieceCapturedFlag = chess::util::isPiece(pieceCaptured);
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteKnight << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceAtIndex(pieceTo))});
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteBishop << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceAtIndex(pieceTo))});
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteRook << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceAtIndex(pieceTo))});
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteQueen << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceAtIndex(pieceTo))});
				promotionSquares ^= chess::util::bitboardFromIndex(pieceTo);
			}

			remainingWhitePawns ^= chess::util::bitboardFromIndex(squareFrom);
		}

		return result;
	} else {
		// Generate Black's Moves
		// Find bishop moves
		chess::u64 remainingBlackBishops = this->bitboards[chess::util::constants::boardModifiers::blackBishop];
		while (remainingBlackBishops) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackBishops);
			importAttacksToMoves(bishopAttacks(squareFrom), squareFrom, 0x0);
			remainingBlackBishops ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find rook moves
		chess::u64 remainingBlackRooks = this->bitboards[chess::util::constants::boardModifiers::blackRook];
		while (remainingBlackRooks) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackRooks);
			importAttacksToMoves(rookAttacks(squareFrom), squareFrom, 0x0);
			remainingBlackRooks ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find queen moves
		chess::u64 remainingBlackQueens = this->bitboards[chess::util::constants::boardModifiers::blackQueen];
		while (remainingBlackQueens) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackQueens);
			importAttacksToMoves(bishopAttacks(squareFrom) | rookAttacks(squareFrom), squareFrom, 0x0);
			remainingBlackQueens ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find knight moves
		chess::u64 remainingBlackKnights = this->bitboards[chess::util::constants::boardModifiers::blackKnight];
		while (remainingBlackKnights) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackKnights);
			importAttacksToMoves(knightAttacks(squareFrom), squareFrom, 0x0);
			remainingBlackKnights ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find king moves
        chess::u8 squareFrom = chess::util::ctz64(this->bitboards[chess::util::constants::boardModifiers::blackKing]);
        importAttacksToMoves(kingAttacks(squareFrom), squareFrom, 0x0);
        if (this->castleBK()){
			if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("g8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("f8"))))){
				result.push_back({squareFrom, chess::util::algebraicToSquare("g8"), static_cast<u16>(0x4000 | (chess::util::constants::boardModifiers::blackKing << 4))});
            }
		}
		if(this->castleBQ()){
			if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("b8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("c8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("d8"))))){
				result.push_back({squareFrom, chess::util::algebraicToSquare("c8"), static_cast<u16>(0x5000 | (chess::util::constants::boardModifiers::blackKing << 4))});
            }
		}
		

		// Find pawn moves
		chess::u64 remainingBlackPawns = this->bitboards[chess::util::constants::boardModifiers::blackPawn];
		while (remainingBlackPawns) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackPawns);
			chess::u64 attackRaw = pawnAttacks(squareFrom);
			importAttacksToMoves(attackRaw & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			if (attackRaw & this->enPassantTargetSquare) {
				result.push_back({squareFrom, chess::util::ctz64(this->enPassantTargetSquare), (chess::util::constants::blackPawn << 4) | chess::util::constants::whitePawn});
			}
			chess::u64 promotionSquares = attackRaw & 0xFF00000000000000ULL;
			while (promotionSquares) {
				chess::u8 pieceTo = chess::util::ctz64(promotionSquares);
				auto pieceCaptured = pieceAtIndex(pieceTo);
				auto pieceCapturedFlag = chess::util::isPiece(pieceCaptured);
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackBishop << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackKnight << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackRook << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				result.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackQueen << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				promotionSquares ^= chess::util::bitboardFromIndex(pieceTo);
			}

			remainingBlackPawns ^= chess::util::bitboardFromIndex(squareFrom);
		}
		return result;
	}
}

// https://www.chessprogramming.org/Classical_Approach
inline chess::u64 chess::position::bishopAttacks(u8 squareFrom) const noexcept {
	return positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::northWest) | positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::northEast) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::southEast) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::southWest);
}

// https://www.chessprogramming.org/Classical_Approach
inline chess::u64 chess::position::rookAttacks(u8 squareFrom) const noexcept {
	return positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::north) | positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::west) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::south) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::east);
}

inline chess::u64 chess::position::knightAttacks(u8 squareFrom) const noexcept {
	return chess::util::constants::knightJumps[squareFrom] & (this->turn() ? this->bitboards[chess::util::constants::boardModifiers::black] | ~this->bitboards[chess::util::constants::boardModifiers::white] : this->bitboards[chess::util::constants::boardModifiers::white] | ~this->bitboards[chess::util::constants::boardModifiers::black]);
}

inline chess::u64 chess::position::kingAttacks(u8 squareFrom) const noexcept {
	return (chess::util::constants::kingAttacks[squareFrom] & (this->turn() ? ~this->bitboards[chess::util::constants::boardModifiers::white] : ~this->bitboards[chess::util::constants::boardModifiers::black]));
}

inline chess::u64 chess::position::pawnAttacks(u8 squareFrom) const noexcept {
	// Pawn push
	chess::u64 singlePawnPush = this->turn() ? ((1ULL << squareFrom) << 8) & this->empty() : ((1ULL << squareFrom) >> 8) & this->empty();
	chess::u64 doublePawnPush = this->turn() ? (singlePawnPush << 8) & this->empty() & 0xFF000000ULL : (singlePawnPush >> 8) & this->empty() & 0xFF00000000ULL;
	// Pawn capture
	chess::u64 pawnCapture = this->turn() ? chess::util::constants::pawnAttacks[1][squareFrom] & (this->bitboards[chess::util::constants::boardModifiers::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][squareFrom] & (this->bitboards[chess::util::constants::boardModifiers::white] | this->enPassantTargetSquare);
	/*std::cout << "Single Push:\n"
			  << chess::debugging::convert(singlePawnPush) << "Double Push:\n"
			  << chess::debugging::convert(doublePawnPush) << "Captures:\n"
			  << chess::debugging::convert(pawnCapture) << std::endl;*/
	return singlePawnPush | doublePawnPush | pawnCapture;
}

// https://www.chessprogramming.org/Classical_Approach
chess::u64 chess::position::positiveRayAttacks(u8 squareFrom, util::constants::attackRayDirection direction) const noexcept {
	chess::u64 attacks = chess::util::constants::attackRays[direction][squareFrom];
	chess::u64 blocker = attacks & this->occupied();
	if (blocker) {
		squareFrom = chess::util::ctz64(blocker);
		attacks ^= chess::util::constants::attackRays[direction][squareFrom];
		attacks &= this->turn() ? ~this->bitboards[chess::util::constants::boardModifiers::white] : ~this->bitboards[chess::util::constants::boardModifiers::black];
	}
	return attacks;
}

// https://www.chessprogramming.org/Classical_Approach
chess::u64 chess::position::negativeRayAttacks(u8 squareFrom, util::constants::attackRayDirection direction) const noexcept {
	chess::u64 attacks = chess::util::constants::attackRays[direction][squareFrom];
	chess::u64 blocker = attacks & this->occupied();
	if (blocker) {
		squareFrom = 63U - chess::util::clz64(blocker);
		attacks ^= chess::util::constants::attackRays[direction][squareFrom];
		attacks &= this->turn() ? ~this->bitboards[chess::util::constants::boardModifiers::white] : ~this->bitboards[chess::util::constants::boardModifiers::black];
	}
	return attacks;
}