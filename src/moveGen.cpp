#include <iostream>

#include "../include/chess.h"

[[nodiscard]] chess::staticVector<chess::moveData> chess::game::moves() const noexcept {
	if (this->threeFoldRep() || this->currentPosition().halfMoveClock >= 50) {
		return staticVector<chess::moveData> {};
	}
	// Continue with normal move generation
	return this->currentPosition().moves();
}

[[nodiscard]] chess::staticVector<chess::moveData> chess::position::moves() const noexcept {
	using namespace chess;
	using namespace chess::util;
	using namespace chess::util::constants;
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
	const u64 attackersOfAllyKing { this->attackers(static_cast<squareAnnotations>(allyKingLocation), opponentColor) };
	const u64 notAttackedByOpponent { ~this->attacks(opponentColor) };
	u64 pinnedPieces { 0 };
	const auto pieceMoves = [&pinnedPieces, &legalMoves, &notAlly, this]<boardAnnotations typeOfPieceToFindMoves>(const boardAnnotations piece, const u64 mask = bitboardFull) {
		u64 allyPieces { this->bitboards[piece] & ~pinnedPieces };    // Pieces that are not pinned
		while (allyPieces) {
			const u8 currentAllyPieceIndex { ctz64(allyPieces) };
			u64 allyPieceMoves { this->pieceMoves<typeOfPieceToFindMoves>(currentAllyPieceIndex) & notAlly & mask };
			while (allyPieceMoves) {
				const auto destinationSquare { ctz64(allyPieceMoves) };
				const auto destinationPiece { this->pieceAtIndex[destinationSquare] };
				legalMoves.append({ .originIndex      = currentAllyPieceIndex,
				                    .destinationIndex = destinationSquare,
				                    .flags            = static_cast<u16>(getCaptureFlag(destinationPiece) | (piece << 4) | destinationPiece) });
				allyPieceMoves ^= bitboardFromIndex(destinationSquare);
			}
			allyPieces ^= bitboardFromIndex(currentAllyPieceIndex);
		}
	};
	if (const auto numberOfAttackers { popcnt64(attackersOfAllyKing) }; numberOfAttackers == 0) {    // Not in check
		// Calculate all pinned pieces
		auto calculateVerticalPin = [=, &pinnedPieces, &legalMoves, this](const auto rayAttackFunction, const auto findBlockerFunction) {
			if (const auto rayFromKingToBlocker = rayAttackFunction(allyKingLocation, this->occupied()); rayFromKingToBlocker & this->bitboards[allyColor]) {
				const u8 blockerLocation          = findBlockerFunction(rayFromKingToBlocker);
				const auto blockerBitboard        = bitboardFromIndex(blockerLocation);
				const auto rayFromBlockerToPinner = rayAttackFunction(blockerLocation, this->occupied());
				if (const auto pinningPiece = rayFromBlockerToPinner & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]); pinningPiece) {
					// The blocker is pinned
					pinnedPieces |= blockerBitboard;    // Add the pinned pieces to the mask of pinned pieces
					auto movementMask { (rayFromBlockerToPinner | rayFromKingToBlocker) ^ blockerBitboard };
					if (this->pieceAtIndex[blockerLocation] == allyRook || this->pieceAtIndex[blockerLocation] == allyQueen) {
						while (movementMask) {
							const auto moveSpot      = ctz64(movementMask);
							const auto capturedPiece = this->pieceAtIndex[moveSpot];
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(getCaptureFlag(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece) });
							movementMask ^= bitboardFromIndex(moveSpot);
						}
					} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
						chess::u64 singlePush = this->turn() ? ((1ULL << blockerLocation) << 8) & this->empty() : ((1ULL << blockerLocation) >> 8) & this->empty();
						chess::u64 doublePush = this->turn() ? (singlePush << 8) & this->empty() & 0xFF000000ULL : (singlePush >> 8) & this->empty() & 0xFF00000000ULL;
						if (singlePush) {    // not double pawn push
							auto moveSpot { ctz64(singlePush) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(allyPawn << 4) });
							if (doublePush) {    // double pawn push (only one possible per pawn)
								u8 moveSpot { ctz64(doublePush) };
								legalMoves.append({ .originIndex      = blockerLocation,
								                    .destinationIndex = moveSpot,
								                    .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)) });
							}
						}
					}
				}
			}
		};

		auto calculateHorizontalPin = [=, &pinnedPieces, &legalMoves, this](const auto rayAttackFunction, const auto findBlockerFunction) {
			if (const auto rayFromKingToBlocker = rayAttackFunction(allyKingLocation, this->occupied()); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				const u8 blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				const auto blockerBitboard { bitboardFromIndex(blockerLocation) };
				const auto rayFromBlockerToPinner { rayAttackFunction(blockerLocation, this->occupied()) };
				if (const auto pinningPiece { rayFromBlockerToPinner & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]) }; pinningPiece) {
					pinnedPieces |= blockerBitboard;
					auto movementMask { (rayFromBlockerToPinner | rayFromKingToBlocker) ^ blockerBitboard };
					if (this->pieceAtIndex[blockerLocation] == allyRook || this->pieceAtIndex[blockerLocation] == allyQueen) {
						while (movementMask) {
							const auto moveSpot { ctz64(movementMask) };
							const auto capturedPiece { this->pieceAtIndex[moveSpot] };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(getCaptureFlag(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece) });
							movementMask ^= bitboardFromIndex(moveSpot);
						}
					}
				}
			}
		};
		auto calculateDiagonalPin = [=, &pinnedPieces, &legalMoves, this](const auto rayAttackFunction, const auto findBlockerFunction) {
			if (const auto rayFromKingToBlocker = rayAttackFunction(allyKingLocation, this->occupied()); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				const u8 blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				const auto blockerBitboard { bitboardFromIndex(blockerLocation) };
				const auto rayFromBlocker { rayAttackFunction(blockerLocation, this->occupied()) };
				if (const auto pinningPiece { rayFromBlocker & (this->bitboards[opponentBishop] | this->bitboards[opponentQueen]) }; pinningPiece) {    // Bishops and Queens can pin on a diagonal
					pinnedPieces |= blockerBitboard;                                                                                                    // Remove the piece from the rest of move generation
					auto movementMask { (rayFromBlocker | rayFromKingToBlocker) ^ blockerBitboard };                                                    // The ray includes the blocker, but not the attacker, which means our blocker must be removed from our movement mask
					if (this->pieceAtIndex[blockerLocation] == allyBishop || this->pieceAtIndex[blockerLocation] == allyQueen) {
						while (movementMask) {
							const auto moveSpot { ctz64(movementMask) };
							const auto capturedPiece { this->pieceAtIndex[moveSpot] };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(getCaptureFlag(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece) });
							movementMask ^= bitboardFromIndex(moveSpot);
						}
					} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
						chess::u64 capture = this->turn() ? chess::util::constants::pawnAttacks[1][blockerLocation] & (this->bitboards[chess::boardAnnotations::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][blockerLocation] & (this->bitboards[chess::boardAnnotations::white] | this->enPassantTargetSquare);
						capture &= movementMask;
						// Only one capture is allowed when pinned
						if (capture & this->enPassantTargetSquare) {
							// enPassant is possible if pinned
							const auto moveSpot { ctz64(this->enPassantTargetSquare) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | (allyPawn << 4)) });
						} else if (capture) {
							const auto moveSpot { ctz64(capture) };
							legalMoves.append({ .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot,
							                    .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
						}
					}
				}
			}
		};
		calculateVerticalPin(&chess::util::positiveRayAttacks<north>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateVerticalPin(&chess::util::negativeRayAttacks<south>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		calculateHorizontalPin(&chess::util::positiveRayAttacks<west>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateHorizontalPin(&chess::util::negativeRayAttacks<east>, [](auto bitboard) -> u8 { return ctz64(bitboard); });

		calculateDiagonalPin(&chess::util::positiveRayAttacks<northWest>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateDiagonalPin(&chess::util::positiveRayAttacks<northEast>, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); });
		calculateDiagonalPin(&chess::util::negativeRayAttacks<southWest>, [](auto bitboard) -> u8 { return ctz64(bitboard); });
		calculateDiagonalPin(&chess::util::negativeRayAttacks<southEast>, [](auto bitboard) -> u8 { return ctz64(bitboard); });

		pieceMoves.operator()<knight>(allyKnight);
		pieceMoves.operator()<rook>(allyRook);
		pieceMoves.operator()<bishop>(allyBishop);
		pieceMoves.operator()<queen>(allyQueen);
		pieceMoves.operator()<king>(allyKing, notAttackedByOpponent);

		if (allyColor == white) {
			if (this->castleWK()) {
				if (!(this->occupied() & (bitboardFromIndex(g1) | bitboardFromIndex(f1))) && (notAttackedByOpponent & bitboardFromIndex(f1)) && (notAttackedByOpponent & bitboardFromIndex(g1))) {
					legalMoves.append({ .originIndex      = e1,
					                    .destinationIndex = g1,
					                    .flags            = static_cast<u16>(0x2000 | (whiteKing << 4)) });
				}
			}
			if (this->castleWQ()) {
				if (!(this->occupied() & (bitboardFromIndex(d1) | bitboardFromIndex(c1) | bitboardFromIndex(b1))) && (notAttackedByOpponent & bitboardFromIndex(d1)) && (notAttackedByOpponent & bitboardFromIndex(c1))) {
					legalMoves.append({ .originIndex      = e1,
					                    .destinationIndex = c1,
					                    .flags            = static_cast<u16>(0x3000 | (whiteKing << 4)) });
				}
			}
		} else {
			if (this->castleBK()) {
				if (!(this->occupied() & (bitboardFromIndex(g8) | bitboardFromIndex(f8))) && (notAttackedByOpponent & bitboardFromIndex(f8)) && (notAttackedByOpponent & bitboardFromIndex(g8))) {
					legalMoves.append({ .originIndex      = e8,
					                    .destinationIndex = g8,
					                    .flags            = static_cast<u16>(0x4000 | (blackKing << 4)) });
				}
			}
			if (this->castleBQ()) {
				if (!(this->occupied() & (bitboardFromIndex(d8) | bitboardFromIndex(c8) | bitboardFromIndex(b8))) && (notAttackedByOpponent & bitboardFromIndex(d8)) && (notAttackedByOpponent & bitboardFromIndex(c8))) {
					legalMoves.append({ .originIndex      = e8,
					                    .destinationIndex = c8,
					                    .flags            = static_cast<u16>(0x5000 | (blackKing << 4)) });
				}
			}
		}

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		while (allyPawns) {
			const auto currentAllyPawnIndex { ctz64(allyPawns) };
			chess::u64 singlePush = this->turn() ? ((1ULL << currentAllyPawnIndex) << 8) & this->empty() : ((1ULL << currentAllyPawnIndex) >> 8) & this->empty();
			chess::u64 doublePush = this->turn() ? (singlePush << 8) & this->empty() & 0xFF000000ULL : (singlePush >> 8) & this->empty() & 0xFF00000000ULL;
			chess::u64 capture    = this->turn() ? chess::util::constants::pawnAttacks[1][currentAllyPawnIndex] & (this->bitboards[chess::boardAnnotations::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][currentAllyPawnIndex] & (this->bitboards[chess::boardAnnotations::white] | this->enPassantTargetSquare);
			const auto enPassant { capture & this->enPassantTargetSquare };
			auto notEnPassantCapture { capture ^ enPassant };
			const auto promotionPush { singlePush & (allyColor == white ? 0xFF00000000000000 : 0xFF) };
			auto promotionCapture { capture & (allyColor == white ? 0xFF00000000000000 : 0xFF) };

			if (promotionCapture) {
				while (promotionCapture) {    // not double pawn push
					const u8 moveSpot { static_cast<u8>(ctz64(promotionCapture)) };
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>(0x1000 | (allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>(0x1000 | (allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>(0x1000 | (allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>(0x1000 | (allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
					promotionCapture ^= bitboardFromIndex(moveSpot);
				}
			} else if (notEnPassantCapture) {
				while (notEnPassantCapture) {    // not double pawn push
					const u8 moveSpot { static_cast<u8>(ctz64(notEnPassantCapture)) };
					auto destinationPiece { this->pieceAtIndex[moveSpot] };
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | destinationPiece) });
					notEnPassantCapture ^= bitboardFromIndex(moveSpot);
				}
			}

			if (enPassant) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				auto newPos { this->move({ .originIndex      = currentAllyPawnIndex,
					                       .destinationIndex = moveSpot,
					                       .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) }) };
				if (!newPos.attackers(static_cast<squareAnnotations>(allyKingLocation), opponentColor)) [[likely]] {
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) });
				}
			}

			if (promotionPush) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(promotionPush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
			} else if (singlePush) {
				const u8 moveSpot { static_cast<u8>(ctz64(singlePush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(allyPawn << 4) });
				if (doublePush) {    // double pawn push (only one possible per pawn)
					const u8 moveSpot { static_cast<u8>(ctz64(doublePush)) };
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)) });
				}
			}

			allyPawns ^= bitboardFromIndex(currentAllyPawnIndex);
		}
	} else if (numberOfAttackers == 1) {    // King is in check - no castling - only blocking captures or running

		// All attacks must either block or capture the attacker
		// attackersOfAllyKing <- mask for capturing attacking pieces
		// blockingOfAllyKing <- mask for blocking attacking pieces
		const u64 captureMask { attackersOfAllyKing };
		u64 blockMask { 0 };
		const auto opponentAttackerLocation { ctz64(attackersOfAllyKing) };
		if (attackersOfAllyKing & this->bitboards[opponentBishop]) {
			blockMask = this->pieceMoves<bishop>(opponentAttackerLocation) & this->pieceMoves<bishop>(allyKingLocation);
		} else if (attackersOfAllyKing & this->bitboards[opponentRook]) {
			blockMask = this->pieceMoves<rook>(opponentAttackerLocation) & this->pieceMoves<rook>(allyKingLocation);
		} else if (attackersOfAllyKing & this->bitboards[opponentQueen]) {
			if (auto rookAttackFromKing { this->pieceMoves<rook>(allyKingLocation) }; rookAttackFromKing & this->bitboards[opponentQueen]) {    // The check is along a file or rank
				blockMask = this->pieceMoves<rook>(opponentAttackerLocation) & rookAttackFromKing;
			} else {    // The check is along a diagonal
				blockMask = this->pieceMoves<bishop>(opponentAttackerLocation) & this->pieceMoves<bishop>(allyKingLocation);
			}
		}

		// Mark all pinned pieces - there is no position where a pinned piece can capture the checker
		const auto markPinned = [&pinnedPieces, allyKingLocation, opponentQueen, allyColor, this](auto direction, auto getBlockerIndexFunction, const auto pinningPiece) {
			if (auto ray = attackRays[direction][allyKingLocation] & this->occupied(); ray) {
				if (auto blockerLocation = getBlockerIndexFunction(ray); colorOf(this->pieceAtIndex[blockerLocation]) == allyColor) {    // If the blocker is an ally, it might be pinned
					if (auto ray2 = attackRays[direction][blockerLocation] & this->occupied(); ray2) {
						auto pinnerIndex { getBlockerIndexFunction(ray2) };
						if (this->pieceAtIndex[pinnerIndex] == pinningPiece || this->pieceAtIndex[pinnerIndex] == opponentQueen) {
							pinnedPieces |= bitboardFromIndex(blockerLocation);
						}
					}
				}
			}
		};

		markPinned(
			north, [](auto bitboard) -> u8 { return ctz64(bitboard); }, opponentRook);
		markPinned(
			south, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); }, opponentRook);
		markPinned(
			west, [](auto bitboard) -> u8 { return ctz64(bitboard); }, opponentRook);
		markPinned(
			east, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); }, opponentRook);
		markPinned(
			northWest, [](auto bitboard) -> u8 { return ctz64(bitboard); }, opponentBishop);
		markPinned(
			southWest, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); }, opponentBishop);
		markPinned(
			northEast, [](auto bitboard) -> u8 { return ctz64(bitboard); }, opponentBishop);
		markPinned(
			southEast, [](auto bitboard) -> u8 { return 63U - clz64(bitboard); }, opponentBishop);

		pieceMoves.operator()<knight>(allyKnight, (captureMask | blockMask));
		pieceMoves.operator()<rook>(allyRook, (captureMask | blockMask));
		pieceMoves.operator()<bishop>(allyBishop, (captureMask | blockMask));
		pieceMoves.operator()<queen>(allyQueen, (captureMask | blockMask));
		pieceMoves.operator()<king>(allyKing, notAttackedByOpponent);

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		const u64 enPassantTargetSquare_local { this->pieceAtIndex[opponentAttackerLocation] != opponentPawn ? 0 : this->enPassantTargetSquare };    // En passant is only available if the checking piece is a pawn
		while (allyPawns) {
			const auto currentAllyPawnIndex { ctz64(allyPawns) };
			chess::u64 singlePush = this->turn() ? ((1ULL << currentAllyPawnIndex) << 8) & this->empty() : ((1ULL << currentAllyPawnIndex) >> 8) & this->empty();
			chess::u64 doublePush = this->turn() ? (singlePush << 8) & this->empty() & 0xFF000000ULL : (singlePush >> 8) & this->empty() & 0xFF00000000ULL;
			chess::u64 capture    = this->turn() ? chess::util::constants::pawnAttacks[1][currentAllyPawnIndex] & (this->bitboards[chess::boardAnnotations::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][currentAllyPawnIndex] & (this->bitboards[chess::boardAnnotations::white] | this->enPassantTargetSquare);
			const auto enPassant  = capture & enPassantTargetSquare_local;
			singlePush &= blockMask;
			doublePush &= blockMask;
			capture &= captureMask;    // Includes en passant, as there can only be one capture when in check
			const auto promotionPush { singlePush & (allyColor == white ? 0xFF00000000000000 : 0xFF) };
			const auto promotionCapture { capture & (allyColor == white ? 0xFF00000000000000 : 0xFF) };

			if (promotionCapture) {
				const u8 moveSpot { static_cast<u8>(ctz64(promotionCapture)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(0x1000 | (allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(0x1000 | (allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(0x1000 | (allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(0x1000 | (allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
			} else if (capture) {
				const u8 moveSpot { static_cast<u8>(ctz64(capture)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
			} else if (enPassant) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				auto newPos { this->move({ .originIndex      = currentAllyPawnIndex,
					                       .destinationIndex = moveSpot,
					                       .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) }) };
				if (!newPos.attackers(static_cast<squareAnnotations>(allyKingLocation), opponentColor)) [[likely]] {
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) });
				}
			}

			if (promotionPush) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(promotionPush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]) });
			} else if (singlePush) {
				const u8 moveSpot { static_cast<u8>(ctz64(singlePush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>(allyPawn << 4) });
			}

			if (doublePush) {    // double pawn push (only one possible per pawn)
				const u8 moveSpot { static_cast<u8>(ctz64(doublePush)) };
				legalMoves.append({ .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot,
				                    .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)) });
			}
			allyPawns ^= bitboardFromIndex(currentAllyPawnIndex);
		}
	} else {    // King is in double check - only king moves legal - no castling
		pieceMoves.operator()<king>(allyKing, notAttackedByOpponent);
	}

	return legalMoves;
}

[[nodiscard]] chess::u64 chess::position::attacks(const chess::boardAnnotations turn) const noexcept {
	const chess::boardAnnotations attackingPawn { chess::util::constructPiece(pawn, turn) };
	const chess::boardAnnotations attackingKnight { chess::util::constructPiece(knight, turn) };
	const chess::boardAnnotations attackingBishop { chess::util::constructPiece(bishop, turn) };
	const chess::boardAnnotations attackingRook { chess::util::constructPiece(rook, turn) };
	const chess::boardAnnotations attackingQueen { chess::util::constructPiece(queen, turn) };
	const chess::boardAnnotations attackingKing { chess::util::constructPiece(king, turn) };

	chess::u64 resultAttackBoard { 0 };

	// 'occupied squares'
	const chess::u64 occupiedSquares { this->occupied() ^ this->bitboards[boardAnnotations::whiteKing ^ turn] };    // remove the king

	// Find bishop moves
	for (chess::u64 remainingBishops { this->bitboards[attackingBishop] | this->bitboards[attackingQueen] }; remainingBishops;) {
		const chess::u8 squareFrom { chess::util::ctz64(remainingBishops) };
		resultAttackBoard |= chess::util::positiveRayAttacks<chess::northWest>(squareFrom, occupiedSquares) | chess::util::positiveRayAttacks<chess::northEast>(squareFrom, occupiedSquares) | chess::util::negativeRayAttacks<chess::southWest>(squareFrom, occupiedSquares) | chess::util::negativeRayAttacks<chess::southEast>(squareFrom, occupiedSquares);
		remainingBishops ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find rook moves
	for (chess::u64 remainingRooks { this->bitboards[attackingRook] | this->bitboards[attackingQueen] }; remainingRooks;) {
		const chess::u8 squareFrom { chess::util::ctz64(remainingRooks) };
		resultAttackBoard |= chess::util::positiveRayAttacks<chess::north>(squareFrom, occupiedSquares) | chess::util::positiveRayAttacks<chess::west>(squareFrom, occupiedSquares) | chess::util::negativeRayAttacks<chess::south>(squareFrom, occupiedSquares) | chess::util::negativeRayAttacks<chess::east>(squareFrom, occupiedSquares);
		remainingRooks ^= chess::util::bitboardFromIndex(squareFrom);
	}

	// Find knight moves
	for (chess::u64 remainingKnights { this->bitboards[attackingKnight] }; remainingKnights;) {
		const chess::u8 squareFrom { chess::util::ctz64(remainingKnights) };
		resultAttackBoard |= this->pieceMoves<knight>(squareFrom);
		remainingKnights ^= chess::util::bitboardFromIndex(squareFrom);
	}

	resultAttackBoard |= chess::util::constants::kingAttacks[chess::util::ctz64(this->bitboards[attackingKing])];

	for (chess::u64 remainingPawns { this->bitboards[attackingPawn] }; remainingPawns;) {
		const chess::u8 squareFrom { chess::util::ctz64(remainingPawns) };
		resultAttackBoard |= turn ? chess::util::constants::pawnAttacks[1][squareFrom] : chess::util::constants::pawnAttacks[0][squareFrom];
		remainingPawns ^= chess::util::bitboardFromIndex(squareFrom);
	}

	return resultAttackBoard;
}

[[nodiscard]] chess::u64 chess::position::attackers(const chess::squareAnnotations square, const chess::boardAnnotations colorAttacking) const noexcept {
	//Attackers
	const chess::boardAnnotations attackingPawn { chess::util::constructPiece(chess::boardAnnotations::pawn, colorAttacking) };
	const chess::boardAnnotations attackingKnight { chess::util::constructPiece(chess::boardAnnotations::knight, colorAttacking) };
	const chess::boardAnnotations attackingBishop { chess::util::constructPiece(chess::boardAnnotations::bishop, colorAttacking) };
	const chess::boardAnnotations attackingRook { chess::util::constructPiece(chess::boardAnnotations::rook, colorAttacking) };
	const chess::boardAnnotations attackingQueen { chess::util::constructPiece(chess::boardAnnotations::queen, colorAttacking) };
	return (this->pieceMoves<bishop>(square) & (this->bitboards[attackingBishop] | this->bitboards[attackingQueen])) |
	       (this->pieceMoves<rook>(square) & (this->bitboards[attackingRook] | this->bitboards[attackingQueen])) |
	       (this->pieceMoves<knight>(square) & this->bitboards[attackingKnight]) |
	       ((colorAttacking ? chess::util::constants::pawnAttacks[0][square] : chess::util::constants::pawnAttacks[1][square]) & this->bitboards[attackingPawn]);
}
