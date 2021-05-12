#include <iostream>

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
	std::vector<chess::move> pseudoLegalMoves;
	const auto importAttacksToMoves = [&](chess::u64 attackBoard, chess::u8 pieceFrom, chess::u16 flagModifier) {
		while (attackBoard) {
			chess::u8 pieceTo = chess::util::ctz64(attackBoard);
			chess::util::constants::boardModifiers capturedPiece = pieceAtIndex(pieceTo);
			pseudoLegalMoves.push_back({pieceFrom, pieceTo, static_cast<u16>(flagModifier | chess::util::isPiece(capturedPiece) | (pieceAtIndex(pieceFrom) << 4) | capturedPiece)});
			attackBoard ^= chess::util::bitboardFromIndex(pieceTo);
		}
	};

	if (this->turn()) {
		// Find bishop moves
		chess::u64 remainingWhiteBishops = this->bitboards[chess::util::constants::boardModifiers::whiteBishop];
		while (remainingWhiteBishops) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteBishops);
			importAttacksToMoves(bishopAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingWhiteBishops ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find rook moves
		chess::u64 remainingWhiteRooks = this->bitboards[chess::util::constants::boardModifiers::whiteRook];
		while (remainingWhiteRooks) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteRooks);
			importAttacksToMoves(rookAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingWhiteRooks ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find queen moves
		chess::u64 remainingWhiteQueens = this->bitboards[chess::util::constants::boardModifiers::whiteQueen];
		while (remainingWhiteQueens) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteQueens);
			importAttacksToMoves(bishopAttacks(squareFrom, this->turn()) | rookAttacks(squareFrom, this->turn()), squareFrom, 0x0);

			remainingWhiteQueens ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find knight moves
		chess::u64 remainingWhiteKnights = this->bitboards[chess::util::constants::boardModifiers::whiteKnight];
		while (remainingWhiteKnights) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhiteKnights);
			importAttacksToMoves(knightAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingWhiteKnights ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find king moves
		{
		    chess::u8 squareFrom = chess::util::ctz64(this->bitboards[chess::util::constants::boardModifiers::whiteKing]);
		    importAttacksToMoves(kingAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			chess::u64 attackBoard = this->attacks(static_cast<chess::util::constants::boardModifiers>((this->turn() ^ 0x08)));
			if (this->castleWK()) {
				if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("g1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("f1")))) &&
					!(attackBoard & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("e1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("f1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("g1"))))) {
					pseudoLegalMoves.push_back({squareFrom, chess::util::algebraicToSquare("g1"), static_cast<u16>(0x2000 | (chess::util::constants::boardModifiers::whiteKing << 4))});
				}
			}
			if (this->castleWQ()) {
				if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("b1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("c1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("d1")))) &&
					!(attackBoard & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("e1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("d1")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("c1"))))) {
					pseudoLegalMoves.push_back({squareFrom, chess::util::algebraicToSquare("c1"), static_cast<u16>(0x3000 | (chess::util::constants::boardModifiers::whiteKing << 4))});
				}
			}
		}

		// Find pawn moves
		chess::u64 remainingWhitePawns = this->bitboards[chess::util::constants::boardModifiers::whitePawn];
		while (remainingWhitePawns) {
			chess::u8 squareFrom = chess::util::ctz64(remainingWhitePawns);
			std::array<chess::u64, 2> attackRaw = pawnAttacks(squareFrom);
			importAttacksToMoves(attackRaw[0] & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			importAttacksToMoves(attackRaw[1] & ~0xFF00000000000000ULL & ~this->enPassantTargetSquare, squareFrom, 0x8000);
			if (attackRaw[0] & this->enPassantTargetSquare) {
				pseudoLegalMoves.push_back({squareFrom, chess::util::ctz64(this->enPassantTargetSquare), 0x6000 | (chess::util::constants::whitePawn << 4) | chess::util::constants::blackPawn});
			}
			chess::u64 promotionSquares = attackRaw[0] & 0xFF00000000000000ULL;
			while (promotionSquares) {
				chess::u8 pieceTo = chess::util::ctz64(promotionSquares);
				auto pieceCaptured = pieceAtIndex(pieceTo);
				auto pieceCapturedFlag = chess::util::isPiece(pieceCaptured);
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteKnight << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceCaptured)});
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteBishop << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceCaptured)});
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteRook << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceCaptured)});
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::whiteQueen << 8) | (chess::util::constants::boardModifiers::whitePawn << 4) | pieceCaptured)});
				promotionSquares ^= chess::util::bitboardFromIndex(pieceTo);
			}

			remainingWhitePawns ^= chess::util::bitboardFromIndex(squareFrom);
		}
		std::vector<chess::move> legalMoves;

		for (auto pseudoLegalMove : pseudoLegalMoves) {
			
				//std::cout << "testing move " << pseudoLegalMove.toString() << std::endl;
				chess::position movedBoard = this->move(pseudoLegalMove);
			chess::u64 attackBoard = movedBoard.attacks(static_cast<chess::util::constants::boardModifiers>(movedBoard.turn()));
			if ((attackBoard & movedBoard.bitboards[chess::util::constants::boardModifiers::whiteKing]) == 0) {
				legalMoves.push_back(pseudoLegalMove);
			}
		}
		return legalMoves;
	} else {
		// Generate Black's Moves
		// Find bishop moves
		chess::u64 remainingBlackBishops = this->bitboards[chess::util::constants::boardModifiers::blackBishop];
		while (remainingBlackBishops) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackBishops);
			importAttacksToMoves(bishopAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingBlackBishops ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find rook moves
		chess::u64 remainingBlackRooks = this->bitboards[chess::util::constants::boardModifiers::blackRook];
		while (remainingBlackRooks) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackRooks);
			importAttacksToMoves(rookAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingBlackRooks ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find queen moves
		chess::u64 remainingBlackQueens = this->bitboards[chess::util::constants::boardModifiers::blackQueen];
		while (remainingBlackQueens) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackQueens);
			importAttacksToMoves(bishopAttacks(squareFrom, this->turn()) | rookAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingBlackQueens ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Find knight moves
		chess::u64 remainingBlackKnights = this->bitboards[chess::util::constants::boardModifiers::blackKnight];
		while (remainingBlackKnights) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackKnights);
			importAttacksToMoves(knightAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			remainingBlackKnights ^= chess::util::bitboardFromIndex(squareFrom);
		}

        // Find king moves
		{
		    chess::u8 squareFrom = chess::util::ctz64(this->bitboards[chess::util::constants::boardModifiers::blackKing]);
		    importAttacksToMoves(kingAttacks(squareFrom, this->turn()), squareFrom, 0x0);
			chess::u64 attackBoard = this->attacks(static_cast<chess::util::constants::boardModifiers>(this->turn() ^ 0x08));
			if (this->castleBK()) {
				if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("g8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("f8")))) && 
                    !(attackBoard & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("e8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("f8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("g8"))))){
					pseudoLegalMoves.push_back({squareFrom, chess::util::algebraicToSquare("g8"), static_cast<u16>(0x4000 | (chess::util::constants::boardModifiers::blackKing << 4))});
                }
			}
			if (this->castleBQ()) {
				if (!(this->occupied() & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("b8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("c8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("d8")))) &&
					!(attackBoard & (chess::util::bitboardFromIndex(chess::util::algebraicToSquare("e8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("d8")) | chess::util::bitboardFromIndex(chess::util::algebraicToSquare("c8"))))) {
					pseudoLegalMoves.push_back({squareFrom, chess::util::algebraicToSquare("c8"), static_cast<u16>(0x5000 | (chess::util::constants::boardModifiers::blackKing << 4))});
				}
			}
		}

		// Find pawn moves
		chess::u64 remainingBlackPawns = this->bitboards[chess::util::constants::boardModifiers::blackPawn];
		while (remainingBlackPawns) {
			chess::u8 squareFrom = chess::util::ctz64(remainingBlackPawns);
			std::array<chess::u64, 2> attackRaw = pawnAttacks(squareFrom);
			importAttacksToMoves(attackRaw[0] & ~0xFFULL & ~this->enPassantTargetSquare, squareFrom, 0x0);
			importAttacksToMoves(attackRaw[1] & ~0xFFULL & ~this->enPassantTargetSquare, squareFrom, 0x9000);
			if (attackRaw[0] & this->enPassantTargetSquare) {
				pseudoLegalMoves.push_back({squareFrom, chess::util::ctz64(this->enPassantTargetSquare), 0x7000 | (chess::util::constants::blackPawn << 4) | chess::util::constants::whitePawn});
			}
			chess::u64 promotionSquares = attackRaw[0] & 0xFFULL;
			while (promotionSquares) {
				chess::u8 pieceTo = chess::util::ctz64(promotionSquares);
				auto pieceCaptured = pieceAtIndex(pieceTo);
				auto pieceCapturedFlag = chess::util::isPiece(pieceCaptured);
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackBishop << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackKnight << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackRook << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				pseudoLegalMoves.push_back({squareFrom, pieceTo, static_cast<u16>(pieceCapturedFlag | (chess::util::constants::boardModifiers::blackQueen << 8) | (chess::util::constants::boardModifiers::blackPawn << 4) | pieceCaptured)});
				promotionSquares ^= chess::util::bitboardFromIndex(pieceTo);
			}

			remainingBlackPawns ^= chess::util::bitboardFromIndex(squareFrom);
		}

		// Filter out Illegal moves
		std::vector<chess::move> legalMoves;
		for (auto pseudoLegalMove : pseudoLegalMoves) {
			//std::cout << "testing move " << pseudoLegalMove.toString() << std::endl;
			chess::position movedBoard = this->move(pseudoLegalMove);
			chess::u64 attackBoard = movedBoard.attacks(static_cast<chess::util::constants::boardModifiers>(movedBoard.turn()));
			//std::cout << chess::debugging::convert(attackBoard) << std::flush;
			if ((attackBoard & movedBoard.bitboards[chess::util::constants::boardModifiers::blackKing]) == 0) {
				legalMoves.push_back(pseudoLegalMove);
			}
		}
		return legalMoves;
	}
}

// https://www.chessprogramming.org/Classical_Approach
inline chess::u64 chess::position::bishopAttacks(u8 squareFrom, chess::util::constants::boardModifiers turn) const noexcept {
	return positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::northWest, turn) | positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::northEast, turn) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::southEast, turn) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::southWest, turn);
}

// https://www.chessprogramming.org/Classical_Approach
inline chess::u64 chess::position::rookAttacks(u8 squareFrom, chess::util::constants::boardModifiers turn) const noexcept {
	return positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::north, turn) | positiveRayAttacks(squareFrom, chess::util::constants::attackRayDirection::west, turn) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::south, turn) | negativeRayAttacks(squareFrom, chess::util::constants::attackRayDirection::east, turn);
}

inline chess::u64 chess::position::knightAttacks(u8 squareFrom, chess::util::constants::boardModifiers turn) const noexcept {
	return chess::util::constants::knightJumps[squareFrom] & (turn ? this->bitboards[chess::util::constants::boardModifiers::black] | ~this->bitboards[chess::util::constants::boardModifiers::white] : this->bitboards[chess::util::constants::boardModifiers::white] | ~this->bitboards[chess::util::constants::boardModifiers::black]);
}

inline chess::u64 chess::position::kingAttacks(u8 squareFrom, chess::util::constants::boardModifiers turn) const noexcept {
	return (chess::util::constants::kingAttacks[squareFrom] & (turn ? ~this->bitboards[chess::util::constants::boardModifiers::white] : ~this->bitboards[chess::util::constants::boardModifiers::black]));
}

inline std::array<chess::u64, 2> chess::position::pawnAttacks(u8 squareFrom) const noexcept {
	// Pawn push
	chess::u64 singlePawnPush = this->turn() ? ((1ULL << squareFrom) << 8) & this->empty() : ((1ULL << squareFrom) >> 8) & this->empty();
	chess::u64 doublePawnPush = this->turn() ? (singlePawnPush << 8) & this->empty() & 0xFF000000ULL : (singlePawnPush >> 8) & this->empty() & 0xFF00000000ULL;
	// Pawn capture
	chess::u64 pawnCapture = this->turn() ? chess::util::constants::pawnAttacks[1][squareFrom] & (this->bitboards[chess::util::constants::boardModifiers::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][squareFrom] & (this->bitboards[chess::util::constants::boardModifiers::white] | this->enPassantTargetSquare);
	return {singlePawnPush | pawnCapture, doublePawnPush};
}

// https://www.chessprogramming.org/Classical_Approach
chess::u64 chess::position::positiveRayAttacks(u8 squareFrom, util::constants::attackRayDirection direction, chess::util::constants::boardModifiers turn) const noexcept {
	chess::u64 attacks = chess::util::constants::attackRays[direction][squareFrom];
	chess::u64 blocker = attacks & this->occupied();
	if (blocker) {
		squareFrom = chess::util::ctz64(blocker);
		attacks ^= chess::util::constants::attackRays[direction][squareFrom];
		attacks &= turn ? ~this->bitboards[chess::util::constants::boardModifiers::white] : ~this->bitboards[chess::util::constants::boardModifiers::black];
	}
	return attacks;
}

// https://www.chessprogramming.org/Classical_Approach
chess::u64 chess::position::negativeRayAttacks(u8 squareFrom, util::constants::attackRayDirection direction, chess::util::constants::boardModifiers turn) const noexcept {
	chess::u64 attacks = chess::util::constants::attackRays[direction][squareFrom];
	chess::u64 blocker = attacks & this->occupied();
	if (blocker) {
		squareFrom = 63U - chess::util::clz64(blocker);
		attacks ^= chess::util::constants::attackRays[direction][squareFrom];
		attacks &= turn ? ~this->bitboards[chess::util::constants::boardModifiers::white] : ~this->bitboards[chess::util::constants::boardModifiers::black];
	}
	return attacks;
}

chess::u64 chess::position::attacks(chess::util::constants::boardModifiers turn) {
	chess::util::constants::boardModifiers pawn = static_cast<chess::util::constants::boardModifiers>(chess::util::constants::boardModifiers::blackPawn | turn);
	chess::util::constants::boardModifiers knight = static_cast<chess::util::constants::boardModifiers>(chess::util::constants::boardModifiers::blackKnight | turn);
	chess::util::constants::boardModifiers bishop = static_cast<chess::util::constants::boardModifiers>(chess::util::constants::boardModifiers::blackBishop | turn);
	chess::util::constants::boardModifiers rook = static_cast<chess::util::constants::boardModifiers>(chess::util::constants::boardModifiers::blackRook | turn);
	chess::util::constants::boardModifiers queen = static_cast<chess::util::constants::boardModifiers>(chess::util::constants::boardModifiers::blackQueen | turn);
	chess::util::constants::boardModifiers king = static_cast<chess::util::constants::boardModifiers>(chess::util::constants::boardModifiers::blackKing | turn);

	chess::u64 resultAttackBoard = 0;
	chess::u64 remainingBishops = this->bitboards[bishop] | this->bitboards[queen];

	while (remainingBishops) {
		chess::u8 squareFrom = chess::util::ctz64(remainingBishops);
		auto at = bishopAttacks(squareFrom, turn);
		resultAttackBoard |= at;
		remainingBishops ^= chess::util::bitboardFromIndex(squareFrom);
	}
	//std::cout << "ba:\n" << chess::debugging::convert(resultAttackBoard) << std::endl;

	// Find rook moves
	chess::u64 remainingRooks = this->bitboards[rook] | this->bitboards[queen];
	while (remainingRooks) {
		chess::u8 squareFrom = chess::util::ctz64(remainingRooks);
		resultAttackBoard |= rookAttacks(squareFrom, turn);
		remainingRooks ^= chess::util::bitboardFromIndex(squareFrom);
	}
	//std::cout << "ra:\n" << chess::debugging::convert(resultAttackBoard) << std::endl;

	// Find knight moves
	chess::u64 remainingKnights = this->bitboards[knight];
	while (remainingKnights) {
		chess::u8 squareFrom = chess::util::ctz64(remainingKnights);
		resultAttackBoard |= knightAttacks(squareFrom, turn);
		remainingKnights ^= chess::util::bitboardFromIndex(squareFrom);
	}
	//std::cout << "ka:\n"<< chess::debugging::convert(resultAttackBoard) << std::endl;

	resultAttackBoard |= chess::util::constants::kingAttacks[chess::util::ctz64(this->bitboards[king])];

	chess::u64 remainingPawns = this->bitboards[pawn];
	while (remainingPawns) {
		chess::u8 squareFrom = chess::util::ctz64(remainingPawns);
		resultAttackBoard |= turn ? chess::util::constants::pawnAttacks[1][squareFrom] : chess::util::constants::pawnAttacks[0][squareFrom];
		remainingPawns ^= chess::util::bitboardFromIndex(squareFrom);
	}

	return resultAttackBoard;
}