#include <iostream>

#include "../include/chess.h"

[[nodiscard]] chess::staticVector<chess::moveData> chess::game::moves() const noexcept {
	// Continue with normal move generation
    if(this->currentPosition().turn() == white){
	    return this->currentPosition().moves<white>();
    }else{
		return this->currentPosition().moves<black>();
	}
}

template <chess::boardAnnotations allyColor>
[[nodiscard]] chess::staticVector<chess::moveData> chess::game::moves() const noexcept {
	if (this->threeFoldRep() || this->currentPosition().halfMoveClock >= 50) {
		return staticVector<chess::moveData> {};
	}
	// Continue with normal move generation
	return this->currentPosition().moves<allyColor>();
}

template <chess::boardAnnotations piece>
inline void generatePieceMoves(const chess::position& positionS, chess::staticVector<chess::moveData>& legalMoves, const chess::u64 pinnedPieces, const chess::u64 notAlly, const chess::u64 mask = chess::util::constants::bitboardFull) noexcept {
	chess::u64 allyPieces { positionS.bitboards[piece] & ~pinnedPieces };    // Pieces that are not pinned
	while (allyPieces) {
		const chess::u8 currentAllyPieceIndex { chess::util::ctz64(allyPieces) };
		chess::u64 allyPieceMoves { positionS.pieceMoves<chess::util::getPieceOf(piece)>(currentAllyPieceIndex, positionS.bitboards[chess::boardAnnotations::occupied]) & notAlly & mask };
		while (allyPieceMoves) {
			const auto destinationSquare { chess::util::ctz64(allyPieceMoves) };
			const auto destinationPiece { positionS.pieceAtIndex[destinationSquare] };
			legalMoves.append({ .originIndex      = currentAllyPieceIndex,
			                    .destinationIndex = destinationSquare,
			                    .flags            = static_cast<chess::u16>(chess::util::getCaptureFlag(destinationPiece) | (piece << 4) | destinationPiece) });
			chess::util::zeroLSB(allyPieceMoves);
		}
		chess::util::zeroLSB(allyPieces);
	}
};

template <chess::boardAnnotations allyColor>
[[nodiscard]] chess::staticVector<chess::moveData> chess::position::moves() const noexcept {
	using namespace chess;
	using namespace chess::util;
	using namespace chess::util::constants;
	constexpr boardAnnotations allyPawn { constructPiece(pawn, allyColor) };
	constexpr boardAnnotations allyKnight { constructPiece(knight, allyColor) };
	constexpr boardAnnotations allyBishop { constructPiece(bishop, allyColor) };
	constexpr boardAnnotations allyRook { constructPiece(rook, allyColor) };
	constexpr boardAnnotations allyQueen { constructPiece(queen, allyColor) };
	constexpr boardAnnotations allyKing { constructPiece(king, allyColor) };

	constexpr boardAnnotations opponentColor { ~allyColor };
	constexpr boardAnnotations opponentPawn { constructPiece(pawn, opponentColor) };
	constexpr boardAnnotations opponentKnight { constructPiece(knight, opponentColor) };
	constexpr boardAnnotations opponentBishop { constructPiece(bishop, opponentColor) };
	constexpr boardAnnotations opponentRook { constructPiece(rook, opponentColor) };
	constexpr boardAnnotations opponentQueen { constructPiece(queen, opponentColor) };
	constexpr boardAnnotations opponentKing { constructPiece(king, opponentColor) };

	const u64 notAlly { ~this->bitboards[allyColor] };
	staticVector<moveData> legalMoves;    // chess::move = 4 bytes * 254 + 8 byte pointer = 1KB

	const squareAnnotations allyKingLocation { ctz64(this->bitboards[allyKing]) };

    // Start with knights and pawns, and then add sliders, as they can pin.
	u64 checkers = (this->pieceMoves<knight>(allyKingLocation, this->bitboards[occupied]) & this->bitboards[opponentKnight]) |
	               (pawnAttacks[allyColor >> 3][allyKingLocation] & this->bitboards[opponentPawn]) | 
                   (this->pieceMoves<bishop>(allyKingLocation, this->bitboards[occupied]) & (this->bitboards[opponentBishop] | this->bitboards[opponentQueen])) |
	               (this->pieceMoves<rook>(allyKingLocation, this->bitboards[occupied]) & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]));

	const u64 notAttackedByOpponent { ~this->attacks<opponentColor>() };
	u64 pinnedPieces { 0 };
	if (const auto numberOfAttackers { popcnt64(checkers) }; numberOfAttackers == 0) {    // Not in check
		// Calculate all pinned pieces
		auto calculateVerticalPin = [=, &pinnedPieces, &legalMoves, this](const auto rayAttackFunction, const auto findBlockerFunction) {
			if (const auto rayFromKingToBlocker = rayAttackFunction(allyKingLocation, this->bitboards[occupied]); rayFromKingToBlocker & this->bitboards[allyColor]) {
				const u8 blockerLocation          = findBlockerFunction(rayFromKingToBlocker);
				const auto blockerBitboard        = bitboardFromIndex(blockerLocation);
				const auto rayFromBlockerToPinner = rayAttackFunction(blockerLocation, this->bitboards[occupied]);
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
							zeroLSB(movementMask);
						}
					} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
						chess::u64 singlePush = allyColor == white ? ((1ULL << blockerLocation) << 8) & this->empty() : ((1ULL << blockerLocation) >> 8) & this->empty();
						chess::u64 doublePush = allyColor == white ? (singlePush << 8) & this->empty() & 0xFF000000ULL : (singlePush >> 8) & this->empty() & 0xFF00000000ULL;
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
			if (const auto rayFromKingToBlocker = rayAttackFunction(allyKingLocation, this->bitboards[occupied]); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				const u8 blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				const auto blockerBitboard { bitboardFromIndex(blockerLocation) };
				const auto rayFromBlockerToPinner { rayAttackFunction(blockerLocation, this->bitboards[occupied]) };
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
							zeroLSB(movementMask);
						}
					}
				}
			}
		};
		auto calculateDiagonalPin = [=, &pinnedPieces, &legalMoves, this](const auto rayAttackFunction, const auto findBlockerFunction) {
			if (const auto rayFromKingToBlocker = rayAttackFunction(allyKingLocation, this->bitboards[occupied]); rayFromKingToBlocker & this->bitboards[allyColor]) {    // If the blocker is an ally, it might be pinned
				const u8 blockerLocation { findBlockerFunction(rayFromKingToBlocker) };
				const auto blockerBitboard { bitboardFromIndex(blockerLocation) };
				const auto rayFromBlocker { rayAttackFunction(blockerLocation, this->bitboards[occupied]) };
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
							zeroLSB(movementMask);
						}
					} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
						const chess::u64 capture = pawnAttacks[allyColor >> 3][blockerLocation] & (this->bitboards[opponentColor] | this->enPassantTargetSquare) & movementMask;
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

		generatePieceMoves<allyKnight>(*this, legalMoves, pinnedPieces, notAlly);
		generatePieceMoves<allyRook>(*this, legalMoves, pinnedPieces, notAlly);
		generatePieceMoves<allyBishop>(*this, legalMoves, pinnedPieces, notAlly);
		generatePieceMoves<allyQueen>(*this, legalMoves, pinnedPieces, notAlly);
		
		if constexpr (allyColor == white) {
			if (this->castleWK()) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(g1) | bitboardFromIndex(f1))) && (notAttackedByOpponent & bitboardFromIndex(f1)) && (notAttackedByOpponent & bitboardFromIndex(g1))) {
					legalMoves.append({ .originIndex      = e1,
					                    .destinationIndex = g1,
					                    .flags            = static_cast<u16>(0x2000 | (whiteKing << 4)) });
				}
			}
			if (this->castleWQ()) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(d1) | bitboardFromIndex(c1) | bitboardFromIndex(b1))) && (notAttackedByOpponent & bitboardFromIndex(d1)) && (notAttackedByOpponent & bitboardFromIndex(c1))) {
					legalMoves.append({ .originIndex      = e1,
					                    .destinationIndex = c1,
					                    .flags            = static_cast<u16>(0x3000 | (whiteKing << 4)) });
				}
			}
		} else {
			if (this->castleBK()) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(g8) | bitboardFromIndex(f8))) && (notAttackedByOpponent & bitboardFromIndex(f8)) && (notAttackedByOpponent & bitboardFromIndex(g8))) {
					legalMoves.append({ .originIndex      = e8,
					                    .destinationIndex = g8,
					                    .flags            = static_cast<u16>(0x4000 | (blackKing << 4)) });
				}
			}
			if (this->castleBQ()) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(d8) | bitboardFromIndex(c8) | bitboardFromIndex(b8))) && (notAttackedByOpponent & bitboardFromIndex(d8)) && (notAttackedByOpponent & bitboardFromIndex(c8))) {
					legalMoves.append({ .originIndex      = e8,
					                    .destinationIndex = c8,
					                    .flags            = static_cast<u16>(0x5000 | (blackKing << 4)) });
				}
			}
		}

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		while (allyPawns) {
			const auto currentAllyPawnIndex { ctz64(allyPawns) };
			chess::u64 singlePush = allyColor == white ? ((1ULL << currentAllyPawnIndex) << 8) & this->empty() : ((1ULL << currentAllyPawnIndex) >> 8) & this->empty();
			chess::u64 doublePush = allyColor == white ? (singlePush << 8) & this->empty() & 0xFF000000ULL : (singlePush >> 8) & this->empty() & 0xFF00000000ULL;
			chess::u64 capture    = pawnAttacks[allyColor >> 3][currentAllyPawnIndex] & (this->bitboards[opponentColor] | this->enPassantTargetSquare);
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
					zeroLSB(promotionCapture);
				}
			} else if (notEnPassantCapture) {
				while (notEnPassantCapture) {    // not double pawn push
					const u8 moveSpot { static_cast<u8>(ctz64(notEnPassantCapture)) };
					auto destinationPiece { this->pieceAtIndex[moveSpot] };
					legalMoves.append({ .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot,
					                    .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | destinationPiece) });
					zeroLSB(notEnPassantCapture);
				}
			}

			if (enPassant) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				auto newPos { this->move({ .originIndex      = currentAllyPawnIndex,
					                       .destinationIndex = moveSpot,
					                       .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4) }) };
				if (!newPos.attackers<opponentColor>(allyKingLocation)) [[likely]] {
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

			zeroLSB(allyPawns);
		}
	} else if (numberOfAttackers == 1) {    // King is in check - no castling - only blocking captures or running

		// All attacks must either block or capture the attacker
		// checkers <- mask for capturing attacking pieces
		// blockingOfAllyKing <- mask for blocking attacking pieces
		const u64 captureMask { checkers };
		u64 blockMask { 0 };

		// Mark all pinned pieces - there is no position where a pinned piece can capture the checker
		const auto markPinned = [&](const auto attackFunction, const chess::boardAnnotations pinningPiece) {
			const auto ray = attackFunction(allyKingLocation, this->bitboards[opponentColor]);
			if (ray & (this->bitboards[pinningPiece] | this->bitboards[opponentQueen])) {
				if (popcnt64(ray & this->bitboards[allyColor]) == 1) {    // If there is one ally piece inbetween, it's pinned
					pinnedPieces |= ray & this->bitboards[allyColor];
				} else if (popcnt64(ray & this->bitboards[allyColor]) == 0) { // If there isn't an ally piece, it's the checking piece.
					blockMask = ray & notAlly;
				}
			}
		};

		markPinned(rayAttack<north>, opponentRook);
		markPinned(rayAttack<south>, opponentRook);
		markPinned(rayAttack<west>, opponentRook);
		markPinned(rayAttack<east>, opponentRook);
		markPinned(rayAttack<northWest>, opponentBishop);
		markPinned(rayAttack<southWest>, opponentBishop);
		markPinned(rayAttack<northEast>, opponentBishop);
		markPinned(rayAttack<southEast>, opponentBishop);

		generatePieceMoves<allyKnight>(*this, legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));
		generatePieceMoves<allyRook>(*this, legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));
		generatePieceMoves<allyBishop>(*this, legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));
		generatePieceMoves<allyQueen>(*this, legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		const u64 enPassantTargetSquare_local { checkers & this->bitboards[opponentPawn] ? this->enPassantTargetSquare : 0 };    // En passant is only available if the checking piece is a pawn
		while (allyPawns) {
			const auto currentAllyPawnIndex { ctz64(allyPawns) };
			chess::u64 singlePush = allyColor == white ? ((1ULL << currentAllyPawnIndex) << 8) & this->empty() : ((1ULL << currentAllyPawnIndex) >> 8) & this->empty();
			chess::u64 doublePush = allyColor == white ? (singlePush << 8) & this->empty() & 0xFF000000ULL : (singlePush >> 8) & this->empty() & 0xFF00000000ULL;
			chess::u64 capture    = pawnAttacks[allyColor >> 3][currentAllyPawnIndex] & (this->bitboards[opponentColor] | this->enPassantTargetSquare);
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
				if (!newPos.attackers<opponentColor>(allyKingLocation)) [[likely]] {
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
			zeroLSB(allyPawns);
		}
	} 
    // King can always move.
    u64 allyKingMoves { this->pieceMoves<king>(allyKingLocation, this->bitboards[occupied]) & notAlly & notAttackedByOpponent };
    while (allyKingMoves) {
        const auto destinationSquare { ctz64(allyKingMoves) };
        const auto destinationPiece { this->pieceAtIndex[destinationSquare] };
        legalMoves.append({ .originIndex      = allyKingLocation,
                            .destinationIndex = destinationSquare,
                            .flags            = static_cast<u16>(getCaptureFlag(destinationPiece) | (allyKing << 4) | destinationPiece) });
        zeroLSB(allyKingMoves);
    }

	return legalMoves;
}

template <chess::boardAnnotations attackingColor>
[[nodiscard]] chess::u64 chess::position::attacks() const noexcept {
	using namespace chess::util;
	constexpr chess::boardAnnotations attackingPawn { constructPiece(pawn, attackingColor) };
	constexpr chess::boardAnnotations attackingKnight { constructPiece(knight, attackingColor) };
	constexpr chess::boardAnnotations attackingBishop { constructPiece(bishop, attackingColor) };
	constexpr chess::boardAnnotations attackingRook { constructPiece(rook, attackingColor) };
	constexpr chess::boardAnnotations attackingQueen { constructPiece(queen, attackingColor) };
	constexpr chess::boardAnnotations attackingKing { constructPiece(king, attackingColor) };

	chess::u64 resultAttackBoard { 0 };

	// 'occupied squares'
	const chess::u64 occupiedSquares { this->bitboards[occupied] ^ this->bitboards[boardAnnotations::whiteKing ^ attackingColor] };    // remove the king

	// Find bishop attacks
	for (chess::u64 remainingBishops { this->bitboards[attackingBishop] | this->bitboards[attackingQueen] }; remainingBishops; zeroLSB(remainingBishops)) {
		const chess::u8 squareFrom { ctz64(remainingBishops) };
		resultAttackBoard |= this->pieceMoves<bishop>(squareFrom, occupiedSquares);
	}

	// Find rook attacks
	for (chess::u64 remainingRooks { this->bitboards[attackingRook] | this->bitboards[attackingQueen] }; remainingRooks; zeroLSB(remainingRooks)) {
		const chess::u8 squareFrom { ctz64(remainingRooks) };
		resultAttackBoard |= this->pieceMoves<rook>(squareFrom, occupiedSquares);
	}

	// Find knight attacks
	for (chess::u64 remainingKnights { this->bitboards[attackingKnight] }; remainingKnights; zeroLSB(remainingKnights)) {
		const chess::u8 squareFrom { ctz64(remainingKnights) };
		resultAttackBoard |= this->pieceMoves<knight>(squareFrom, occupiedSquares);
	}

	resultAttackBoard |= constants::kingAttacks[ctz64(this->bitboards[attackingKing])];

    if constexpr (attackingColor == white) {
		resultAttackBoard |= (this->bitboards[attackingPawn] << 9) & ~0x0101010101010101;
		resultAttackBoard |= (this->bitboards[attackingPawn] << 7) & ~0x8080808080808080;
	} else {
		resultAttackBoard |= (this->bitboards[attackingPawn] >> 9) & ~0x8080808080808080;
		resultAttackBoard |= (this->bitboards[attackingPawn] >> 7) & ~0x0101010101010101;
	}

	return resultAttackBoard;
}

template <chess::boardAnnotations attackingColor>
[[nodiscard]] chess::u64 chess::position::attackers(const chess::squareAnnotations square) const noexcept {
	using namespace chess::util;
	return (this->pieceMoves<bishop>(square, this->bitboards[occupied]) & (this->bitboards[constructPiece(bishop, attackingColor)] | this->bitboards[constructPiece(queen, attackingColor)])) |
	       (this->pieceMoves<rook>(square, this->bitboards[occupied]) & (this->bitboards[constructPiece(rook, attackingColor)] | this->bitboards[constructPiece(queen, attackingColor)])) |
	       (this->pieceMoves<knight>(square, this->bitboards[occupied]) & this->bitboards[constructPiece(knight, attackingColor)]) |
	       (constants::pawnAttacks[~attackingColor >> 3][square] & this->bitboards[constructPiece(pawn, attackingColor)]);
}