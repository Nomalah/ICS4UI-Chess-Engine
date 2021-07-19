#include <iostream>

#include "../include/chess.h"

[[nodiscard]] chess::moveList chess::game::moves() const noexcept {
	// Continue with normal move generation
	if (this->turn == chess::boardAnnotations::black) {
		return this->moves<black>();
	} else {
		return this->moves<white>();
	}
}

template <chess::boardAnnotations piece>
void chess::game::generatePieceMoves(chess::moveList& legalMoves, const chess::u64 pinnedPieces, const chess::u64 notAlly, const chess::u64 mask) const noexcept {
	chess::u64 allyPieces { this->bitboards[piece] & ~pinnedPieces };    // Pieces that are not pinned
	while (allyPieces) {
		const chess::u8 currentAllyPieceIndex { chess::util::ctz64(allyPieces) };
		chess::u64 allyPieceMoves { this->pieceMoves<chess::util::getPieceOf(piece)>(currentAllyPieceIndex, this->bitboards[chess::boardAnnotations::occupied]) & notAlly & mask };
		while (allyPieceMoves) {
			const auto destinationSquare { chess::util::ctz64(allyPieceMoves) };
			const auto destinationPiece { this->pieceAtIndex[destinationSquare] };
			legalMoves.append({ .flags            = static_cast<chess::u16>(chess::util::getCaptureFlag(destinationPiece) | (piece << 4) | destinationPiece),
			                    .originIndex      = currentAllyPieceIndex,
			                    .destinationIndex = destinationSquare });
			chess::util::zeroLSB(allyPieceMoves);
		}
		chess::util::zeroLSB(allyPieces);
	}
};

template <chess::boardAnnotations allyColor>
[[nodiscard]] chess::moveList chess::game::moves() const noexcept {
	if (this->threeFoldRep() || this->flags.back().halfMoveClock >= 50) {
		return chess::moveList { false };
	}

	using namespace chess;
	using namespace chess::util;
	using namespace chess::constants;
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

	const squareAnnotations allyKingLocation { ctz64(this->bitboards[allyKing]) };

	// Start with knights and pawns, and then add sliders, as they can pin.
	u64 checkers = (this->pieceMoves<knight>(allyKingLocation, this->bitboards[occupied]) & this->bitboards[opponentKnight]) |
	               (pawnAttacks[allyColor >> 3][allyKingLocation] & this->bitboards[opponentPawn]) |
	               (this->pieceMoves<bishop>(allyKingLocation, this->bitboards[occupied]) & (this->bitboards[opponentBishop] | this->bitboards[opponentQueen])) |
	               (this->pieceMoves<rook>(allyKingLocation, this->bitboards[occupied]) & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]));

	const u64 notAttackedByOpponent { ~this->attacks<opponentColor>() };
	u64 pinnedPieces { 0 };
	const auto numberOfAttackers { popcnt64(checkers) };

	moveList legalMoves { static_cast<bool>(numberOfAttackers) };

	if (numberOfAttackers == 0) {    // Not in check
		// Calculate all pinned pieces
		auto calculateVerticalPin = [=, &pinnedPieces, &legalMoves, this](const auto attackFunction) {
			const auto ray = attackFunction(allyKingLocation, this->bitboards[opponentColor]);
			if (ray & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]) && popcnt64(ray & this->bitboards[allyColor]) == 1) {    // If there is one ally piece inbetween, it's pinned
				pinnedPieces |= ray & this->bitboards[allyColor];
				auto movementMask { ray & notAlly };
				auto blockerLocation { ctz64(ray & this->bitboards[allyColor]) };
				if (ray & (this->bitboards[allyRook] | ray & this->bitboards[allyQueen])) {
					while (movementMask) {
						const auto moveSpot      = ctz64(movementMask);
						const auto capturedPiece = this->pieceAtIndex[moveSpot];
						legalMoves.append({ .flags            = static_cast<u16>(getCaptureFlag(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece),
						                    .originIndex      = blockerLocation,
						                    .destinationIndex = moveSpot });
						zeroLSB(movementMask);
					}
				} else if (ray & this->bitboards[allyPawn]) {
					chess::u64 singlePush = allyColor == white ? ((1ULL << blockerLocation) << 8) & this->bitboards[empty] : ((1ULL << blockerLocation) >> 8) & this->bitboards[empty];
					chess::u64 doublePush = allyColor == white ? (singlePush << 8) & this->bitboards[empty] & 0xFF000000ULL : (singlePush >> 8) & this->bitboards[empty] & 0xFF00000000ULL;
					if (singlePush) {    // not double pawn push
						auto moveSpot { ctz64(singlePush) };
						legalMoves.append({ .flags            = static_cast<u16>(allyPawn << 4),
						                    .originIndex      = blockerLocation,
						                    .destinationIndex = moveSpot });
						if (doublePush) {    // double pawn push (only one possible per pawn)
							u8 moveSpot { ctz64(doublePush) };
							legalMoves.append({ .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)),
							                    .originIndex      = blockerLocation,
							                    .destinationIndex = moveSpot });
						}
					}
				}
			}
		};

		auto calculateHorizontalPin = [=, &pinnedPieces, &legalMoves, this](const auto attackFunction) {
			const auto ray = attackFunction(allyKingLocation, this->bitboards[opponentColor]);
			if (ray & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]) && popcnt64(ray & this->bitboards[allyColor]) == 1) {    // If there is one ally piece inbetween, it's pinned
				pinnedPieces |= ray & this->bitboards[allyColor];
				auto movementMask { ray & notAlly };
				auto blockerLocation { ctz64(ray & this->bitboards[allyColor]) };
				if (this->pieceAtIndex[blockerLocation] == allyRook || this->pieceAtIndex[blockerLocation] == allyQueen) {
					while (movementMask) {
						const auto moveSpot { ctz64(movementMask) };
						const auto capturedPiece { this->pieceAtIndex[moveSpot] };
						legalMoves.append({ .flags            = static_cast<u16>(getCaptureFlag(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece),
						                    .originIndex      = blockerLocation,
						                    .destinationIndex = moveSpot });
						zeroLSB(movementMask);
					}
				}
			}
		};
		auto calculateDiagonalPin = [=, &pinnedPieces, &legalMoves, this](const auto attackFunction) {
			const auto ray = attackFunction(allyKingLocation, this->bitboards[opponentColor]);
			if (ray & (this->bitboards[opponentBishop] | this->bitboards[opponentQueen]) && popcnt64(ray & this->bitboards[allyColor]) == 1) {    // If there is one ally piece inbetween, it's pinned
				pinnedPieces |= ray & this->bitboards[allyColor];
				auto movementMask { ray & notAlly };
				auto blockerLocation { ctz64(ray & this->bitboards[allyColor]) };
				if (this->pieceAtIndex[blockerLocation] == allyBishop || this->pieceAtIndex[blockerLocation] == allyQueen) {
					while (movementMask) {
						const auto moveSpot { ctz64(movementMask) };
						const auto capturedPiece { this->pieceAtIndex[moveSpot] };
						legalMoves.append({ .flags            = static_cast<u16>(getCaptureFlag(capturedPiece) | (this->pieceAtIndex[blockerLocation] << 4) | capturedPiece),
						                    .originIndex      = blockerLocation,
						                    .destinationIndex = moveSpot });
						zeroLSB(movementMask);
					}
				} else if (this->pieceAtIndex[blockerLocation] == allyPawn) {
					const chess::u64 capture = pawnAttacks[allyColor >> 3][blockerLocation] & (this->bitboards[opponentColor] | this->flags.back().enPassantTargetBitboard) & movementMask;
					// Only one capture is allowed when pinned
					if (capture & this->flags.back().enPassantTargetBitboard) {
						// enPassant is possible if pinned
						const auto moveSpot { ctz64(this->flags.back().enPassantTargetBitboard) };
						legalMoves.append({ .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | (allyPawn << 4)),
						                    .originIndex      = blockerLocation,
						                    .destinationIndex = moveSpot });
					} else if (capture) {
						const auto moveSpot { ctz64(capture) };
						legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
						                    .originIndex      = blockerLocation,
						                    .destinationIndex = moveSpot });
					}
				}
			}
		};

		calculateVerticalPin(rayAttack<north>);
		calculateVerticalPin(rayAttack<south>);
		calculateHorizontalPin(rayAttack<west>);
		calculateHorizontalPin(rayAttack<east>);

		calculateDiagonalPin(rayAttack<northWest>);
		calculateDiagonalPin(rayAttack<northEast>);
		calculateDiagonalPin(rayAttack<southWest>);
		calculateDiagonalPin(rayAttack<southEast>);

		generatePieceMoves<allyKnight>(legalMoves, pinnedPieces, notAlly);
		generatePieceMoves<allyRook>(legalMoves, pinnedPieces, notAlly);
		generatePieceMoves<allyBishop>(legalMoves, pinnedPieces, notAlly);
		generatePieceMoves<allyQueen>(legalMoves, pinnedPieces, notAlly);

		if constexpr (allyColor == white) {
			if (this->flags.back().castleWK) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(g1) | bitboardFromIndex(f1))) && (notAttackedByOpponent & bitboardFromIndex(f1)) && (notAttackedByOpponent & bitboardFromIndex(g1))) {
					legalMoves.append({ .flags            = static_cast<u16>(0x2000 | (whiteKing << 4)),
					                    .originIndex      = e1,
					                    .destinationIndex = g1 });
				}
			}
			if (this->flags.back().castleWQ) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(d1) | bitboardFromIndex(c1) | bitboardFromIndex(b1))) && (notAttackedByOpponent & bitboardFromIndex(d1)) && (notAttackedByOpponent & bitboardFromIndex(c1))) {
					legalMoves.append({ .flags            = static_cast<u16>(0x3000 | (whiteKing << 4)),
					                    .originIndex      = e1,
					                    .destinationIndex = c1 });
				}
			}
		} else {
			if (this->flags.back().castleBK) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(g8) | bitboardFromIndex(f8))) && (notAttackedByOpponent & bitboardFromIndex(f8)) && (notAttackedByOpponent & bitboardFromIndex(g8))) {
					legalMoves.append({ .flags            = static_cast<u16>(0x4000 | (blackKing << 4)),
					                    .originIndex      = e8,
					                    .destinationIndex = g8 });
				}
			}
			if (this->flags.back().castleBQ) {
				if (!(this->bitboards[occupied] & (bitboardFromIndex(d8) | bitboardFromIndex(c8) | bitboardFromIndex(b8))) && (notAttackedByOpponent & bitboardFromIndex(d8)) && (notAttackedByOpponent & bitboardFromIndex(c8))) {
					legalMoves.append({ .flags            = static_cast<u16>(0x5000 | (blackKing << 4)),
					                    .originIndex      = e8,
					                    .destinationIndex = c8 });
				}
			}
		}

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		while (allyPawns) {
			const auto currentAllyPawnIndex { ctz64(allyPawns) };
			chess::u64 singlePush = allyColor == white ? ((1ULL << currentAllyPawnIndex) << 8) & this->bitboards[empty] : ((1ULL << currentAllyPawnIndex) >> 8) & this->bitboards[empty];
			chess::u64 doublePush = allyColor == white ? (singlePush << 8) & this->bitboards[empty] & 0xFF000000ULL : (singlePush >> 8) & this->bitboards[empty] & 0xFF00000000ULL;
			chess::u64 capture    = pawnAttacks[allyColor >> 3][currentAllyPawnIndex] & (this->bitboards[opponentColor] | this->flags.back().enPassantTargetBitboard);
			const auto enPassant { capture & this->flags.back().enPassantTargetBitboard };
			auto notEnPassantCapture { capture ^ enPassant };
			const auto promotionPush { singlePush & (allyColor == white ? 0xFF00000000000000 : 0xFF) };
			auto promotionCapture { capture & (allyColor == white ? 0xFF00000000000000 : 0xFF) };

			if (promotionCapture) {
				while (promotionCapture) {    // not double pawn push
					const u8 moveSpot { static_cast<u8>(ctz64(promotionCapture)) };
					legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
					legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
					legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
					legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
					zeroLSB(promotionCapture);
				}
			} else if (notEnPassantCapture) {
				while (notEnPassantCapture) {    // not double pawn push
					const u8 moveSpot { static_cast<u8>(ctz64(notEnPassantCapture)) };
					auto destinationPiece { this->pieceAtIndex[moveSpot] };
					legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | destinationPiece),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
					zeroLSB(notEnPassantCapture);
				}
			}

			if (enPassant) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				const u64 postCaptureOccupied { this->bitboards[boardAnnotations::occupied] ^ (enPassant | (1ULL << currentAllyPawnIndex) | (allyColor == white ? enPassant >> 8 : enPassant << 8)) };

				if (!((rayAttack<east>(allyKingLocation, postCaptureOccupied) | rayAttack<west>(allyKingLocation, postCaptureOccupied)) & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]))) [[likely]] {
					legalMoves.append({ .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
				}
			}

			if (promotionPush) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(promotionPush)) };
				legalMoves.append({ .flags            = static_cast<u16>((allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>((allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>((allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>((allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
			} else if (singlePush) {
				const u8 moveSpot { static_cast<u8>(ctz64(singlePush)) };
				legalMoves.append({ .flags            = static_cast<u16>(allyPawn << 4),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				if (doublePush) {    // double pawn push (only one possible per pawn)
					const u8 moveSpot { static_cast<u8>(ctz64(doublePush)) };
					legalMoves.append({ .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
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
				} else if (popcnt64(ray & this->bitboards[allyColor]) == 0) {    // If there isn't an ally piece, it's the checking piece.
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

		generatePieceMoves<allyKnight>(legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));
		generatePieceMoves<allyRook>(legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));
		generatePieceMoves<allyBishop>(legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));
		generatePieceMoves<allyQueen>(legalMoves, pinnedPieces, notAlly, (captureMask | blockMask));

		u64 allyPawns { this->bitboards[allyPawn] & ~pinnedPieces };
		const u64 enPassantTargetSquare_local { checkers & this->bitboards[opponentPawn] ? this->flags.back().enPassantTargetBitboard : 0 };    // En passant is only available if the checking piece is a pawn
		while (allyPawns) {
			const auto currentAllyPawnIndex { ctz64(allyPawns) };
			chess::u64 singlePush = allyColor == white ? ((1ULL << currentAllyPawnIndex) << 8) & this->bitboards[empty] : ((1ULL << currentAllyPawnIndex) >> 8) & this->bitboards[empty];
			chess::u64 doublePush = allyColor == white ? (singlePush << 8) & this->bitboards[empty] & 0xFF000000ULL : (singlePush >> 8) & this->bitboards[empty] & 0xFF00000000ULL;
			chess::u64 capture    = pawnAttacks[allyColor >> 3][currentAllyPawnIndex] & (this->bitboards[opponentColor] | this->flags.back().enPassantTargetBitboard);
			const auto enPassant  = capture & enPassantTargetSquare_local;
			singlePush &= blockMask;
			doublePush &= blockMask;
			capture &= captureMask;    // Includes en passant, as there can only be one capture when in check
			const auto promotionPush { singlePush & (allyColor == white ? 0xFF00000000000000 : 0xFF) };
			const auto promotionCapture { capture & (allyColor == white ? 0xFF00000000000000 : 0xFF) };

			if (promotionCapture) {
				const u8 moveSpot { static_cast<u8>(ctz64(promotionCapture)) };
				legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
			} else if (capture) {
				const u8 moveSpot { static_cast<u8>(ctz64(capture)) };
				legalMoves.append({ .flags            = static_cast<u16>(0x1000 | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
			} else if (enPassant) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(enPassant)) };
				const u64 postCaptureOccupied { this->bitboards[boardAnnotations::occupied] ^ (enPassant | (1ULL << currentAllyPawnIndex) | (allyColor == white ? enPassant >> 8 : enPassant << 8)) };
				if (!((rayAttack<east>(allyKingLocation, postCaptureOccupied) | rayAttack<west>(allyKingLocation, postCaptureOccupied)) & (this->bitboards[opponentRook] | this->bitboards[opponentQueen]))) [[likely]] {
					legalMoves.append({ .flags            = static_cast<u16>((allyColor ? 0x6000 : 0x7000) | allyPawn << 4),
					                    .originIndex      = currentAllyPawnIndex,
					                    .destinationIndex = moveSpot });
				}
			}

			if (promotionPush) [[unlikely]] {
				const u8 moveSpot { static_cast<u8>(ctz64(promotionPush)) };
				legalMoves.append({ .flags            = static_cast<u16>((allyKnight << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>((allyBishop << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>((allyRook << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
				legalMoves.append({ .flags            = static_cast<u16>((allyQueen << 8) | (allyPawn << 4) | this->pieceAtIndex[moveSpot]),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
			} else if (singlePush) {
				const u8 moveSpot { static_cast<u8>(ctz64(singlePush)) };
				legalMoves.append({ .flags            = static_cast<u16>(allyPawn << 4),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
			}

			if (doublePush) {    // double pawn push (only one possible per pawn)
				const u8 moveSpot { static_cast<u8>(ctz64(doublePush)) };
				legalMoves.append({ .flags            = static_cast<u16>((allyColor ? 0x8000 : 0x9000) | (allyPawn << 4)),
				                    .originIndex      = currentAllyPawnIndex,
				                    .destinationIndex = moveSpot });
			}
			zeroLSB(allyPawns);
		}
	}
	// King can always move.
	u64 allyKingMoves { this->pieceMoves<king>(allyKingLocation, this->bitboards[occupied]) & notAlly & notAttackedByOpponent };
	while (allyKingMoves) {
		const auto destinationSquare { ctz64(allyKingMoves) };
		const auto destinationPiece { this->pieceAtIndex[destinationSquare] };
		legalMoves.append({ .flags            = static_cast<u16>(getCaptureFlag(destinationPiece) | (allyKing << 4) | destinationPiece),
		                    .originIndex      = allyKingLocation,
		                    .destinationIndex = destinationSquare });
		zeroLSB(allyKingMoves);
	}

	return legalMoves;
}

template <chess::boardAnnotations attackingColor>
[[nodiscard]] chess::u64 chess::game::attacks() const noexcept {
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