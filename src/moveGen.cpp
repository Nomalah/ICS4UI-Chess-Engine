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
	typedef u64 (position::*positionRayAttackFunction)(const u8 squareFrom) const noexcept;
#define CALL_RAY_ATTACK_FUNCTION(object, ptrToMember) ((object).*(ptrToMember))
	const boardAnnotations allyColor { this->turn() };
	const boardAnnotations allyPawn { constructPiece(pawn, allyColor) };
	const boardAnnotations allyKnight { constructPiece(knight, allyColor) };
	const boardAnnotations allyBishop { constructPiece(bishop, allyColor) };
	const boardAnnotations allyRook { constructPiece(rook, allyColor) };
	const boardAnnotations allyQueen { constructPiece(queen, allyColor) };
	const boardAnnotations allyKing { constructPiece(king, allyColor) };

	const boardAnnotations opponentColor { oppositeColorOf(allyColor) };
	const boardAnnotations opponentPawn { constructPiece(pawn, opponentColor) };
	const boardAnnotations opponentKnight { constructPiece(knight, opponentColor) };
	const boardAnnotations opponentBishop { constructPiece(bishop, opponentColor) };
	const boardAnnotations opponentRook { constructPiece(rook, opponentColor) };
	const boardAnnotations opponentQueen { constructPiece(queen, opponentColor) };
	const boardAnnotations opponentKing { constructPiece(king, opponentColor) };

	const u64 notAlly { ~this->bitboards[allyColor] };
	staticVector<moveData> legalMoves;    // chess::move = 4 bytes * 254 + 8 byte pointer = 1KB

	const u8 allyKingLocation { ctz64(this->bitboards[allyKing]) };
	u64 attackersOfAllyKing { this->attackers(static_cast<squareAnnotations>(allyKingLocation), opponentColor) };
	u64 opponentAttackBoard { this->attacks(opponentColor) };

	if (auto numberOfAttackers { popcnt64(attackersOfAllyKing) }; numberOfAttackers == 0) {    // Not in check
		// Calculate all pinned pieces
		u64 pinnedPieces { 0 };

		auto calculateVerticalPin = [&](positionRayAttackFunction rayAttackFunction, auto findBlockerFunction) {
			if (auto rayFromKingToBlocker = CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(allyKingLocation); rayFromKingToBlocker & this->bitboards[allyColor]) {
				u8 blockerLocation          = findBlockerFunction(rayFromKingToBlocker);
				auto blockerBitboard        = bitboardFromIndex(blockerLocation);
				auto rayFromBlockerToPinner = CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(blockerLocation);
				if (auto pinningPiece = rayFromBlockerToPinner & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]); pinningPiece) {
					// The blocker is pinned
					pinnedPieces ^= blockerBitboard;    // Add the pinned pieces to the mask of pinned pieces
					auto movementMask { (rayFromBlockerToPinner | rayFromKingToBlocker) ^ blockerBitboard };
					if (this->pieceAtIndex[blockerLocation] == allyRook || this->pieceAtIndex[blockerLocation] == allyQueen) {
						while (movementMask) {
							auto moveSpot      = ctz64(movementMask);
							auto capturedPiece = this->pieceAtIndex[moveSpot];
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(isPiece(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece) });
							movementMask ^= bitboardFromIndex(moveSpot);
						}
					} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
						auto [regularPushAndCapture, doublePush] = this->pawnAttacks(blockerLocation);
						regularPushAndCapture &= movementMask;
						while (regularPushAndCapture) {    // not double pawn push
							auto moveSpot { ctz64(regularPushAndCapture) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(allyPawn << 4) });
							regularPushAndCapture ^= bitboardFromIndex(moveSpot);
						}

						doublePush &= movementMask;    // Operation may not be neccisary - look into
						if (doublePush) {              // double pawn push (only one possible per pawn)
							u8 moveSpot { ctz64(doublePush) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)) });
						}
					}
				}
			}
		};

		auto calculateHorizontalPin = [&](positionRayAttackFunction rayAttackFunction, auto findBlockerFunction) {
			if (auto rayFromKingToBlocker = CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(allyKingLocation); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				u8 blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				auto blockerBitboard { bitboardFromIndex(blockerLocation) };
				auto rayFromBlockerToPinner { CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(blockerLocation) };
				if (auto pinningPiece { rayFromBlockerToPinner & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]) }; pinningPiece) {
					pinnedPieces ^= blockerBitboard;
					auto movementMask { (rayFromBlockerToPinner | rayFromKingToBlocker) ^ blockerBitboard };
					if (this->pieceAtIndex[blockerLocation] == allyRook || this->pieceAtIndex[blockerLocation] == allyQueen) {
						while (movementMask) {
							auto moveSpot { ctz64(movementMask) };
							auto capturedPiece { this->pieceAtIndex[moveSpot] };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(isPiece(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece) });
							movementMask ^= bitboardFromIndex(moveSpot);
						}
					}
				}
			}
		};

		auto calculateDiagonalPin = [&](positionRayAttackFunction rayAttackFunction, auto findBlockerFunction) {
			if (auto rayFromKingToBlocker = CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(allyKingLocation); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				u8 blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				auto blockerBitboard { bitboardFromIndex(blockerLocation) };
				auto rayFromBlocker { CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(blockerLocation) };
				if (auto pinningPiece { rayFromBlocker & (this->bitboards[opponentBishop] | this->bitboards[opponentQueen]) }; pinningPiece) {    // Bishops and Queens can pin on a diagonal
					pinnedPieces ^= blockerBitboard;                                                                                              // Remove the piece from the rest of move generation
					auto movementMask { (rayFromBlocker | rayFromKingToBlocker) ^ blockerBitboard };                                              // The ray includes the blocker, but not the attacker, which means our blocker must be removed from our movement mask
					if (this->pieceAtIndex[blockerLocation] == allyBishop || this->pieceAtIndex[blockerLocation] == allyQueen) {
						while (movementMask) {
							auto moveSpot { ctz64(movementMask) };
							auto capturedPiece { this->pieceAtIndex[moveSpot] };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(isPiece(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece) });
							movementMask ^= bitboardFromIndex(moveSpot);
						}
					} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
						auto [regularPushAndCapture, doublePawnPush] = pawnAttacks(blockerLocation);
						regularPushAndCapture &= movementMask;
						// Only one capture is allowed when pinned
						if (regularPushAndCapture & this->enPassantTargetSquare) {
							// enPassant is possible if pinned
							auto moveSpot { ctz64(this->enPassantTargetSquare) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | (allyPawn << 4)) });
						} else if (regularPushAndCapture) {
							auto moveSpot { ctz64(regularPushAndCapture) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
						}
					}
				}
			}
		};
		calculateVerticalPin(&position::positiveRayAttacks<north>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateVerticalPin(&position::negativeRayAttacks<south>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		calculateHorizontalPin(&position::positiveRayAttacks<west>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateHorizontalPin(&position::negativeRayAttacks<east>, [](auto bitboard) -> u8 { return ctz64(bitboard); });

		calculateDiagonalPin(&position::positiveRayAttacks<northWest>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateDiagonalPin(&position::positiveRayAttacks<northEast>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateDiagonalPin(&position::negativeRayAttacks<southWest>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		calculateDiagonalPin(&position::negativeRayAttacks<southEast>, [](auto bitboard) -> u8 { return ctz64(bitboard); });

		u64 allyKnights { this->bitboards[allyKnight] & ~pinnedPieces };    // Knights that are not pinned
		while (allyKnights) {
			u8 currentAllyKnightIndex { ctz64(allyKnights) };
			u64 allyKnightMoves { this->knightAttacks(currentAllyKnightIndex) & notAlly };
			while (allyKnightMoves) {
				auto destinationSquare { ctz64(allyKnightMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyKnightIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKnight << 4) | destinationPiece) });
				allyKnightMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyKnights ^= bitboardFromIndex(currentAllyKnightIndex);
		}

		u64 allyBishops { this->bitboards[allyBishop] & ~pinnedPieces };    // Knights that are not pinned
		while (allyBishops) {
			u8 currentAllyBishopIndex { ctz64(allyBishops) };
			u64 allyBishopMoves { this->bishopAttacks(static_cast<squareAnnotations>(currentAllyBishopIndex)) & notAlly };
			while (allyBishopMoves) {
				auto destinationSquare { ctz64(allyBishopMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyBishopIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyBishop << 4) | destinationPiece) });
				allyBishopMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyBishops ^= bitboardFromIndex(currentAllyBishopIndex);
		}

		u64 allyRooks { this->bitboards[allyRook] & ~pinnedPieces };    // Rooks that are not pinned
		while (allyRooks) {
			auto currentAllyRookIndex { ctz64(allyRooks) };
			u64 allyRookMoves { this->rookAttacks(currentAllyRookIndex) & notAlly };
			while (allyRookMoves) {
				auto destinationSquare { ctz64(allyRookMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyRookIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyRook << 4) | destinationPiece) });
				allyRookMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyRooks ^= bitboardFromIndex(currentAllyRookIndex);
		}

		u64 allyQueens { this->bitboards[allyQueen] & ~pinnedPieces };    // Knights that are not pinned
		while (allyQueens) {
			u8 currentAllyQueenIndex { ctz64(allyQueens) };
			u64 allyQueenMoves { (this->bishopAttacks(static_cast<squareAnnotations>(currentAllyQueenIndex)) | this->rookAttacks(static_cast<squareAnnotations>(currentAllyQueenIndex))) & notAlly };
			while (allyQueenMoves) {
				auto destinationSquare { ctz64(allyQueenMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyQueenIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyQueen << 4) | destinationPiece) });
				allyQueenMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyQueens ^= bitboardFromIndex(currentAllyQueenIndex);
		}

		u64 allyKingMoves { this->kingAttacks(allyKingLocation) & ~opponentAttackBoard & notAlly };
		while (allyKingMoves) {
			u8 destinationSquare { ctz64(allyKingMoves) };
			auto destinationPiece { this->pieceAtIndex[destinationSquare] };
			legalMoves.append({ .originIndex      = allyKingLocation,
			                    .destinationIndex = destinationSquare,
			                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKing << 4) | destinationPiece) });
			allyKingMoves ^= bitboardFromIndex(destinationSquare);
		}

		if (allyColor == white) {
			if (this->castleWK()) {
				if (!(this->occupied() & (bitboardFromIndex(g1) | bitboardFromIndex(f1))) && !(opponentAttackBoard & (bitboardFromIndex(f1) | bitboardFromIndex(g1)))) {
					legalMoves.append({ .originIndex      = e1,
					                    .destinationIndex = g1,
					                    .flags            = static_cast<u16>(0x2000 | (whiteKing << 4)) });
				}
			}
			if (this->castleWQ()) {
				if (!(this->occupied() & (bitboardFromIndex(d1) | bitboardFromIndex(c1) | bitboardFromIndex(b1))) && !(opponentAttackBoard & (bitboardFromIndex(d1) | bitboardFromIndex(c1)))) {
					legalMoves.append({ .originIndex      = e1,
					                    .destinationIndex = c1,
					                    .flags            = static_cast<u16>(0x3000 | (whiteKing << 4)) });
				}
			}
		} else {
			if (this->castleBK()) {
				if (!(this->occupied() & (bitboardFromIndex(g8) | bitboardFromIndex(f8))) && !(opponentAttackBoard & (bitboardFromIndex(f8) | bitboardFromIndex(g8)))) {
					legalMoves.append({ .originIndex      = e8,
					                    .destinationIndex = g8,
					                    .flags            = static_cast<u16>(0x4000 | (blackKing << 4)) });
				}
			}
			if (this->castleBQ()) {
				if (!(this->occupied() & (bitboardFromIndex(d8) | bitboardFromIndex(c8) | bitboardFromIndex(b8))) && !(opponentAttackBoard & (bitboardFromIndex(d8) | bitboardFromIndex(c8)))) {
					legalMoves.append({ .originIndex      = e8,
					                    .destinationIndex = c8,
					                    .flags            = static_cast<u16>(0x5000 | (blackKing << 4)) });
				}
			}
		}

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };

		while (allyPawns) {
			auto currentAllyPawnIndex { ctz64(allyPawns) };
			auto [regularPushAndCapture, doublePush] = this->pawnAttacks(currentAllyPawnIndex);
			auto enPassant { regularPushAndCapture & this->enPassantTargetSquare };
			auto notEnPassant { regularPushAndCapture ^ enPassant };
			auto promotions { notEnPassant & (allyColor == white ? 0xFF00000000000000 : 0xFF) };

			notEnPassant ^= promotions;
			while (notEnPassant) {    // not double pawn push
				u8 moveSpot { static_cast<u8>(ctz64(notEnPassant)) };
				auto destinationPiece { this->pieceAtIndex[moveSpot] };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyPawn << 4) | destinationPiece) });
				notEnPassant ^= bitboardFromIndex(moveSpot);
			}

			while (promotions) {    // not double pawn push
				u8 moveSpot { static_cast<u8>(ctz64(promotions)) };
				auto destinationPiece { this->pieceAtIndex[moveSpot] };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKnight << 8) | (allyPawn << 4) | destinationPiece) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyBishop << 8) | (allyPawn << 4) | destinationPiece) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyRook << 8) | (allyPawn << 4) | destinationPiece) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyQueen << 8) | (allyPawn << 4) | destinationPiece) });
				promotions ^= bitboardFromIndex(moveSpot);
			}

			if (enPassant) {
				u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				auto newPos { this->move({ .originIndex      = currentAllyPawnIndex,
					                       .destinationIndex = moveSpot,
					                       .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) }) };
				if (!newPos.attackers(static_cast<squareAnnotations>(allyKingLocation), opponentColor)) {
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) });
				}
			}

			if (doublePush) {    // double pawn push (only one possible per pawn)
				u8 moveSpot { static_cast<u8>(ctz64(doublePush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)) });
			}
			allyPawns ^= bitboardFromIndex(currentAllyPawnIndex);
		}
	} else if (numberOfAttackers == 1) {    // King is in check - no castling - only blocking captures or running
		// All attacks must either block or capture the attacker
		// attackersOfAllyKing <- mask for capturing attacking pieces
		// blockingOfAllyKing <- mask for blocking attacking pieces
		u64 captureMask { attackersOfAllyKing };
		u64 blockMask { 0 };
		auto opponentAttackerLocation { ctz64(attackersOfAllyKing) };
		if (attackersOfAllyKing & this->bitboards[opponentBishop]) {
			blockMask = this->bishopAttacks(opponentAttackerLocation) & this->bishopAttacks(allyKingLocation);
		} else if (attackersOfAllyKing & this->bitboards[opponentRook]) {
			blockMask = this->rookAttacks(opponentAttackerLocation) & this->rookAttacks(allyKingLocation);
		} else if (attackersOfAllyKing & this->bitboards[opponentQueen]) {
			if (auto rookAttackFromKing { this->rookAttacks(allyKingLocation) }; rookAttackFromKing & this->bitboards[opponentQueen]) {    // The check is along a file or rank
				blockMask = this->rookAttacks(opponentAttackerLocation) & rookAttackFromKing;
			} else {    // The check is along a diagonal
				blockMask = this->bishopAttacks(opponentAttackerLocation) & this->bishopAttacks(allyKingLocation);
			}
		}

		// Mark all pinned pieces - there is no position where a pinned piece can capture the checker
		u64 pinnedPieces { 0 };
		auto markPinnedPerpendicular = [&](positionRayAttackFunction rayAttackFunction, auto findBlockerFunction) {
			if (auto rayFromKingToBlocker = CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(allyKingLocation); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				auto blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				auto rayFromBlockerToPinner { CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(blockerLocation) };
				if (rayFromBlockerToPinner & (this->bitboards[opponentRook] | this->bitboards[opponentQueen])) {
					pinnedPieces |= bitboardFromIndex(blockerLocation);
				}
			}
		};

		auto markPinnedDiagonal = [&](positionRayAttackFunction rayAttackFunction, auto findBlockerFunction) {
			if (auto rayFromKingToBlocker = CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(allyKingLocation); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				auto blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				auto rayFromBlockerToPinner { CALL_RAY_ATTACK_FUNCTION(*this, rayAttackFunction)(blockerLocation) };
				if (rayFromBlockerToPinner & (this->bitboards[opponentBishop] | this->bitboards[opponentQueen])) {
					pinnedPieces |= bitboardFromIndex(blockerLocation);
				}
			}
		};
		markPinnedPerpendicular(&position::positiveRayAttacks<north>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		markPinnedPerpendicular(&position::negativeRayAttacks<south>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		markPinnedPerpendicular(&position::positiveRayAttacks<west>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		markPinnedPerpendicular(&position::negativeRayAttacks<east>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		markPinnedDiagonal(&position::positiveRayAttacks<northWest>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		markPinnedDiagonal(&position::negativeRayAttacks<southWest>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		markPinnedDiagonal(&position::positiveRayAttacks<northEast>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		markPinnedDiagonal(&position::negativeRayAttacks<southEast>, [](auto bitboard) -> u8 { return ctz64(bitboard); });

		u64 allyKnights { this->bitboards[allyKnight] & ~pinnedPieces };    // Knights that are not pinned
		while (allyKnights) {
			u8 currentAllyKnightIndex { ctz64(allyKnights) };
			u64 allyKnightMoves { this->knightAttacks(currentAllyKnightIndex) & notAlly & (captureMask | blockMask) };
			while (allyKnightMoves) {
				u8 destinationSquare { ctz64(allyKnightMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyKnightIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKnight << 4) | destinationPiece) });
				allyKnightMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyKnights ^= bitboardFromIndex(currentAllyKnightIndex);
		}

		u64 allyBishops { this->bitboards[allyBishop] & ~pinnedPieces };    // Knights that are not pinned
		while (allyBishops) {
			u8 currentAllyBishopIndex { ctz64(allyBishops) };
			u64 allyBishopMoves { this->bishopAttacks(currentAllyBishopIndex) & notAlly & (captureMask | blockMask) };
			while (allyBishopMoves) {
				u8 destinationSquare { ctz64(allyBishopMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyBishopIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyBishop << 4) | destinationPiece) });
				allyBishopMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyBishops ^= bitboardFromIndex(currentAllyBishopIndex);
		}

		u64 allyRooks { this->bitboards[allyRook] & ~pinnedPieces };    // Knights that are not pinned
		while (allyRooks) {
			u8 currentAllyRookIndex { ctz64(allyRooks) };
			u64 allyRookMoves { this->rookAttacks(currentAllyRookIndex) & notAlly & (captureMask | blockMask) };
			while (allyRookMoves) {
				u8 destinationSquare { ctz64(allyRookMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyRookIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyRook << 4) | destinationPiece) });
				allyRookMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyRooks ^= bitboardFromIndex(currentAllyRookIndex);
		}

		u64 allyQueens { this->bitboards[allyQueen] & ~pinnedPieces };    // Knights that are not pinned
		while (allyQueens) {
			u8 currentAllyQueenIndex { ctz64(allyQueens) };
			u64 allyQueenMoves { (this->bishopAttacks(static_cast<squareAnnotations>(currentAllyQueenIndex)) | this->rookAttacks(static_cast<squareAnnotations>(currentAllyQueenIndex))) & notAlly & (captureMask | blockMask) };
			while (allyQueenMoves) {
				auto destinationSquare { ctz64(allyQueenMoves) };
				auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyQueenIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyQueen << 4) | destinationPiece) });
				allyQueenMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyQueens ^= bitboardFromIndex(currentAllyQueenIndex);
		}

		// King is in check, no castling
		u64 allyKingMoves { this->kingAttacks(allyKingLocation) & ~opponentAttackBoard & notAlly };
		while (allyKingMoves) {
			u8 destinationSquare { ctz64(allyKingMoves) };
			auto destinationPiece { this->pieceAtIndex[destinationSquare] };
			legalMoves.append({ .originIndex      = allyKingLocation,
			                    .destinationIndex = destinationSquare,
			                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKing << 4) | destinationPiece) });
			allyKingMoves ^= bitboardFromIndex(destinationSquare);
		}

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		u64 enPassantTargetSquare_local { this->pieceAtIndex[opponentAttackerLocation] != opponentPawn ? 0 : this->enPassantTargetSquare };
		while (allyPawns) {
			auto currentAllyPawnIndex { ctz64(allyPawns) };
			auto [regularPushAndCapture, doublePush] = this->pawnAttacks(currentAllyPawnIndex);
			regularPushAndCapture &= (captureMask | blockMask | enPassantTargetSquare_local);
			auto enPassant { regularPushAndCapture & enPassantTargetSquare_local };
			auto notEnPassant { regularPushAndCapture ^ enPassant };
			auto promotions { notEnPassant & (allyColor == white ? 0xFF00000000000000 : 0xFF) };

			notEnPassant ^= promotions;
			while (notEnPassant) {    // not double pawn push
				u8 moveSpot { static_cast<u8>(ctz64(notEnPassant)) };
				auto destinationPiece { this->pieceAtIndex[moveSpot] };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyPawn << 4) | destinationPiece) });
				notEnPassant ^= bitboardFromIndex(moveSpot);
			}

			while (promotions) {    // not double pawn push
				u8 moveSpot { static_cast<u8>(ctz64(promotions)) };
				auto destinationPiece { this->pieceAtIndex[moveSpot] };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKnight << 8) | (allyPawn << 4) | destinationPiece) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyBishop << 8) | (allyPawn << 4) | destinationPiece) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyRook << 8) | (allyPawn << 4) | destinationPiece) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyQueen << 8) | (allyPawn << 4) | destinationPiece) });
				promotions ^= bitboardFromIndex(moveSpot);
			}

			if (enPassant) {
				u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				auto newPos { this->move({ .originIndex      = currentAllyPawnIndex,
					                       .destinationIndex = moveSpot,
					                       .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) }) };
				if (!newPos.attackers(static_cast<squareAnnotations>(allyKingLocation), opponentColor)) {
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) });
				}
			}

			doublePush &= blockMask;    // Operation may not be neccisary - look into
			if (doublePush) {           // double pawn push (only one possible per pawn)
				u8 moveSpot { static_cast<u8>(ctz64(doublePush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)) });
			}
			allyPawns ^= bitboardFromIndex(currentAllyPawnIndex);
		}
	} else {    // King is in double check - only king moves legal - no castling
		u64 allyKingMoves { this->kingAttacks(allyKingLocation) & ~opponentAttackBoard & notAlly };
		while (allyKingMoves) {
			u8 destinationSquare { static_cast<u8>(ctz64(allyKingMoves)) };
			auto destinationPiece { this->pieceAtIndex[destinationSquare] };
			legalMoves.append({ .originIndex      = allyKingLocation,
			                    .destinationIndex = destinationSquare,
			                    .flags            = static_cast<u16>(isPiece(destinationPiece) | (allyKing << 4) | destinationPiece) });
			allyKingMoves ^= bitboardFromIndex(destinationSquare);
		}
	}

	return legalMoves;
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

template <chess::attackRayDirection direction>
[[nodiscard]] static inline constexpr chess::u64 positiveRayAttacks_checkAvoidance(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
	chess::u64 attacks { chess::util::constants::attackRays[direction][squareFrom] };
	chess::u64 blocker { (attacks & occupiedSquares) | 0x8000000000000000 };
	attacks ^= chess::util::constants::attackRays[direction][chess::util::ctz64(blocker)];
	return attacks;
}

template <chess::attackRayDirection direction>
[[nodiscard]] static inline constexpr chess::u64 negativeRayAttacks_checkAvoidance(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
	chess::u64 attacks { chess::util::constants::attackRays[direction][squareFrom] };
	chess::u64 blocker { (attacks & occupiedSquares) | 0x1 };
	attacks ^= chess::util::constants::attackRays[direction][63U - chess::util::clz64(blocker)];
	return attacks;
}

[[nodiscard]] chess::u64 chess::position::attacks(const chess::boardAnnotations turn) const noexcept {
	chess::boardAnnotations pawn   = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackPawn | turn);
	chess::boardAnnotations knight = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKnight | turn);
	chess::boardAnnotations bishop = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackBishop | turn);
	chess::boardAnnotations rook   = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackRook | turn);
	chess::boardAnnotations queen  = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackQueen | turn);
	chess::boardAnnotations king   = static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKing | turn);

	chess::u64 resultAttackBoard { 0 };

	// 'occupied squares'
	const chess::u64 occupiedSquares { this->occupied() ^ this->bitboards[boardAnnotations::whiteKing ^ turn] };

	// Find bishop moves
	for (chess::u64 remainingBishops { this->bitboards[bishop] | this->bitboards[queen] }; remainingBishops;) {
		chess::u8 squareFrom { chess::util::ctz64(remainingBishops) };
		resultAttackBoard |= positiveRayAttacks_checkAvoidance<chess::northWest>(squareFrom, occupiedSquares) | positiveRayAttacks_checkAvoidance<chess::northEast>(squareFrom, occupiedSquares) | negativeRayAttacks_checkAvoidance<chess::southWest>(squareFrom, occupiedSquares) | negativeRayAttacks_checkAvoidance<chess::southEast>(squareFrom, occupiedSquares);
		remainingBishops ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find rook moves
	for (chess::u64 remainingRooks { this->bitboards[rook] | this->bitboards[queen] }; remainingRooks;) {
		chess::u8 squareFrom { chess::util::ctz64(remainingRooks) };
		resultAttackBoard |= positiveRayAttacks_checkAvoidance<chess::north>(squareFrom, occupiedSquares) | positiveRayAttacks_checkAvoidance<chess::west>(squareFrom, occupiedSquares) | negativeRayAttacks_checkAvoidance<chess::south>(squareFrom, occupiedSquares) | negativeRayAttacks_checkAvoidance<chess::east>(squareFrom, occupiedSquares);
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

[[nodiscard]] chess::u64 chess::position::attackers(const chess::squareAnnotations square, const chess::boardAnnotations colorAttacking) const noexcept {    // checking white king
	//Attackers
	chess::boardAnnotations pawn { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackPawn | colorAttacking) };    // whites pieces
	chess::boardAnnotations knight { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackKnight | colorAttacking) };
	chess::boardAnnotations bishop { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackBishop | colorAttacking) };
	chess::boardAnnotations rook { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackRook | colorAttacking) };
	chess::boardAnnotations queen { static_cast<chess::boardAnnotations>(chess::boardAnnotations::blackQueen | colorAttacking) };
	return (bishopAttacks(square) & (this->bitboards[bishop] | this->bitboards[queen])) |
	       (rookAttacks(square) & (this->bitboards[rook] | this->bitboards[queen])) |
	       (knightAttacks(square) & this->bitboards[knight]) |
	       ((colorAttacking ? chess::util::constants::pawnAttacks[0][square] : chess::util::constants::pawnAttacks[1][square]) & this->bitboards[pawn]);
}
