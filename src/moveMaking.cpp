#include <iostream>
#include <iomanip>
#include <utility>
#include <cassert>

#include "../include/chess.h"

void chess::game::make(const chess::moveData desiredMove) noexcept {
	using namespace chess::constants;
	irreversible newFlags { .zobristHash             = 0x0ULL,
		                    .enPassantTargetBitboard = 0x0ULL,
		                    .castleWK                = this->flags.back().castleWK,
		                    .castleWQ                = this->flags.back().castleWQ,
		                    .castleBK                = this->flags.back().castleBK,
		                    .castleBQ                = this->flags.back().castleBQ,
		                    .halfMoveClock           = static_cast<unsigned char>(this->flags.back().halfMoveClock + 1) };

	if (this->turn == chess::boardAnnotations::black)
		this->fullMoveClock++;
	this->turn = static_cast<chess::boardAnnotations>(this->turn ^ chess::boardAnnotations::white);
	// Remove the piece from it's origin square
	this->bitboards[chess::util::colorOf(desiredMove.movePiece())] ^= desiredMove.originSquare();    // Colour only
	this->bitboards[desiredMove.movePiece()] ^= desiredMove.originSquare();                          // Colour and Piece
	this->pieceAtIndex[desiredMove.originIndex] = boardAnnotations::empty;                           // Set index value to Empty
	switch (desiredMove.moveFlags()) {
		case chess::moveFlags::capture:
			newFlags.halfMoveClock = 0;                                                                               // Capture
			this->bitboards[chess::util::colorOf(desiredMove.capturedPiece())] ^= desiredMove.destinationSquare();    // Delete captured piece's colour
			this->bitboards[desiredMove.capturedPiece()] ^= desiredMove.destinationSquare();                          // Delete captured piece
			[[fallthrough]];
		case chess::moveFlags::quiet:                                                                             // Quiet move
			this->bitboards[chess::util::colorOf(desiredMove.movePiece())] |= desiredMove.destinationSquare();    // Colour only
			this->bitboards[desiredMove.movePiece()] |= desiredMove.destinationSquare();                          // Colour and Piece
			this->pieceAtIndex[desiredMove.destinationIndex] = desiredMove.movePiece();                           // fill index value
			break;

		case chess::moveFlags::capturePromotionBegin... chess::moveFlags::capturePromotionEnd:                        // Capture (Promotion)
			this->bitboards[chess::util::colorOf(desiredMove.capturedPiece())] ^= desiredMove.destinationSquare();    // Delete captured piece's colour
			this->bitboards[desiredMove.capturedPiece()] ^= desiredMove.destinationSquare();                          // Delete captured piece
			[[fallthrough]];
		case chess::moveFlags::quietPromotionBegin... chess::moveFlags::quietPromotionEnd:                             // Promotion
			this->bitboards[chess::util::colorOf(desiredMove.promotionPiece())] |= desiredMove.destinationSquare();    // Colour only
			this->bitboards[desiredMove.promotionPiece()] |= desiredMove.destinationSquare();                          // Colour and Piece
			this->pieceAtIndex[desiredMove.destinationIndex] = desiredMove.promotionPiece();                           // fill index value
			break;

		case chess::moveFlags::whiteKingsideCastle:    // White Kingside
			this->bitboards[white] ^= 0x7;             // flip the bits for the white bitboard
			this->bitboards[whiteRook] ^= 0x5;         // Flip the bits for the castling rook
			this->bitboards[whiteKing] = 0x2;
			this->pieceAtIndex[h1]     = boardAnnotations::empty;
			this->pieceAtIndex[g1]     = whiteKing;
			this->pieceAtIndex[f1]     = whiteRook;
			break;
		case chess::moveFlags::whiteQueensideCastle:    // White Queenside
			this->bitboards[white] ^= 0xB0;             // flip the bits for the white bitboard
			this->bitboards[whiteRook] ^= 0x90;         // Flip the bits for the castling rook
			this->bitboards[whiteKing] = 0x20;
			this->pieceAtIndex[a1]     = boardAnnotations::empty;
			this->pieceAtIndex[c1]     = whiteKing;
			this->pieceAtIndex[d1]     = whiteRook;
			break;
		case chess::moveFlags::blackKingsideCastle:                 // Black Kingside
			this->bitboards[black] ^= 0x0700000000000000ULL;        // flip the bits for the black bitboard
			this->bitboards[blackRook] ^= 0x0500000000000000ULL;    // Flip the bits for the castling rook
			this->bitboards[blackKing] = 0x0200000000000000ULL;
			this->pieceAtIndex[h8]     = boardAnnotations::empty;
			this->pieceAtIndex[g8]     = blackKing;
			this->pieceAtIndex[f8]     = blackRook;
			break;
		case chess::moveFlags::blackQueensideCastle:                // Black Queenside
			this->bitboards[black] ^= 0xB000000000000000ULL;        // flip the bits for the black bitboard
			this->bitboards[blackRook] ^= 0x9000000000000000ULL;    // Flip the bits for the castling rook
			this->bitboards[blackKing] = 0x2000000000000000ULL;
			this->pieceAtIndex[a8]     = boardAnnotations::empty;
			this->pieceAtIndex[c8]     = blackKing;
			this->pieceAtIndex[d8]     = blackRook;
			break;
		case chess::moveFlags::whiteEnPassant:    // White taking black en passent
			this->bitboards[white] |= desiredMove.destinationSquare();
			this->bitboards[whitePawn] |= desiredMove.destinationSquare();
			this->bitboards[black] ^= (desiredMove.destinationSquare() >> 8);
			this->bitboards[blackPawn] ^= (desiredMove.destinationSquare() >> 8);
			this->pieceAtIndex[desiredMove.destinationIndex]     = whitePawn;
			this->pieceAtIndex[desiredMove.destinationIndex - 8] = boardAnnotations::empty;
			break;
		case chess::moveFlags::blackEnPassant:    // Black taking white en passent
			this->bitboards[black] |= desiredMove.destinationSquare();
			this->bitboards[blackPawn] |= desiredMove.destinationSquare();
			this->bitboards[white] ^= (desiredMove.destinationSquare() << 8);
			this->bitboards[whitePawn] ^= (desiredMove.destinationSquare() << 8);
			this->pieceAtIndex[desiredMove.destinationIndex]     = blackPawn;
			this->pieceAtIndex[desiredMove.destinationIndex + 8] = boardAnnotations::empty;
			break;
		case chess::moveFlags::whiteDoublePush:                               // White double pawn push
			this->bitboards[white] |= desiredMove.destinationSquare();        // Colour only
			this->bitboards[whitePawn] |= desiredMove.destinationSquare();    // Colour and Piece
			this->pieceAtIndex[desiredMove.destinationIndex] = whitePawn;     // fill index value
			newFlags.enPassantTargetBitboard                 = desiredMove.destinationSquare() >> 8;
			break;
		case chess::moveFlags::blackDoublePush:                               // Black double pawn push
			this->bitboards[black] |= desiredMove.destinationSquare();        // Colour only
			this->bitboards[blackPawn] |= desiredMove.destinationSquare();    // Colour and Piece
			this->pieceAtIndex[desiredMove.destinationIndex] = blackPawn;     // fill index value
			newFlags.enPassantTargetBitboard                 = desiredMove.destinationSquare() << 8;
			break;
		default:
			std::cerr << "Unknown Move: " << std::hex << desiredMove.moveFlags() << '\n';
			assert(false);
			break;
	}

	// Invalidate castling
	switch (desiredMove.movePiece()) {
		case blackRook:
			if (desiredMove.originIndex == 56) {
				newFlags.castleBK = false;
			} else if (desiredMove.originIndex == 63) {
				newFlags.castleBQ = false;
			}
			break;
		case blackKing:
			newFlags.castleBK = false;
			newFlags.castleBQ = false;
			break;
		case whiteRook:
			if (desiredMove.originIndex == 0) {
				newFlags.castleWK = false;
			} else if (desiredMove.originIndex == 7) {
				newFlags.castleWQ = false;
			}
			break;
		case whiteKing:
			newFlags.castleWK = false;
			newFlags.castleWQ = false;
			break;
		case whitePawn:
			[[fallthrough]];
		case blackPawn:
			newFlags.halfMoveClock = 0;
			break;
		default:
			break;
	}

	// Invalidate castling
	switch (desiredMove.capturedPiece()) {
		case blackRook:
			if (desiredMove.destinationIndex == 56) {
				newFlags.castleBK = false;
			} else if (desiredMove.destinationIndex == 63) {
				newFlags.castleBQ = false;
			}
			break;
		case whiteRook:
			if (desiredMove.destinationIndex == 0) {
				newFlags.castleWK = false;
			} else if (desiredMove.destinationIndex == 7) {
				newFlags.castleWQ = false;
			}
			break;
		default:
			break;
	}
	this->flags.push_back(newFlags);
	this->bitboards[boardAnnotations::occupied] = this->bitboards[white] | this->bitboards[black];
	this->bitboards[boardAnnotations::empty]    = ~this->bitboards[boardAnnotations::occupied];
	this->setZobrist();
}

void chess::game::make(const std::string& uciMove) noexcept {
	chess::moveData libraryMove {
		.flags            = 0,
		.originIndex      = chess::util::algebraicToSquare(uciMove.substr(0, 2)),
		.destinationIndex = chess::util::algebraicToSquare(uciMove.substr(2, 2))
	};
	libraryMove.flags |= this->pieceAtIndex[libraryMove.destinationIndex];
	libraryMove.flags |= this->pieceAtIndex[libraryMove.originIndex] << 4;

	if (uciMove.length() == 5) {
		constexpr std::array<std::pair<char, chess::u8>, 5> charToPiece { { { 'n', chess::knight },
			                                                                { 'b', chess::bishop },
			                                                                { 'r', chess::rook },
			                                                                { 'q', chess::queen } } };
		for (auto& mapping : charToPiece) {
			if (mapping.first == uciMove[4]) {
				libraryMove.flags |= (mapping.second | chess::util::colorOf(libraryMove.movePiece())) << 8;
				break;
			}
		}
	}

	// Check for castling
	switch (libraryMove.movePiece()) {
		case chess::blackKing:
			if (libraryMove.originIndex == e8) {
				if (libraryMove.destinationIndex == g8) {
					libraryMove.flags |= chess::moveFlags::blackKingsideCastle;
				} else if (libraryMove.destinationIndex == c8) {
					libraryMove.flags |= chess::moveFlags::blackQueensideCastle;
				}
			}
			break;
		case chess::whiteKing:
			if (libraryMove.originIndex == e1) {
				if (libraryMove.destinationIndex == g1) {
					libraryMove.flags |= chess::moveFlags::whiteKingsideCastle;
				} else if (libraryMove.destinationIndex == c1) {
					libraryMove.flags |= chess::moveFlags::whiteQueensideCastle;
				}
			}
			break;
		case chess::blackPawn:
			if (libraryMove.originIndex >= h7 && libraryMove.originIndex <= a7 && libraryMove.destinationIndex >= h5 && libraryMove.destinationIndex <= a5) {
				libraryMove.flags |= chess::moveFlags::blackDoublePush;
			} else if (libraryMove.originIndex % 8 != libraryMove.destinationIndex % 8) {
				if (!chess::util::getCaptureFlag(libraryMove.capturedPiece())) {
					libraryMove.flags |= chess::moveFlags::blackEnPassant;
					libraryMove.flags &= 0xFFF0;
					libraryMove.flags |= chess::whitePawn;
				}
			}
			break;
		case chess::whitePawn:
			if (libraryMove.originIndex >= h2 && libraryMove.originIndex <= a2 && libraryMove.destinationIndex >= h4 && libraryMove.destinationIndex <= a4) {
				libraryMove.flags |= chess::moveFlags::whiteDoublePush;
			} else if (libraryMove.originIndex % 8 != libraryMove.destinationIndex % 8) {
				if (!chess::util::getCaptureFlag(libraryMove.capturedPiece())) {
					libraryMove.flags |= chess::moveFlags::whiteEnPassant;
					libraryMove.flags &= 0xFFF0;
					libraryMove.flags |= chess::blackPawn;
				}
			}
			break;
		default:
			break;
	}

	if (libraryMove.moveFlags() == 0) {
		libraryMove.flags |= chess::util::getCaptureFlag(libraryMove.capturedPiece());
	}

	this->make(libraryMove);
}

void chess::game::unmake(const chess::moveData desiredMove) noexcept {
	this->flags.pop_back();
	if (this->turn != chess::boardAnnotations::black)
		this->fullMoveClock--;
	this->turn = static_cast<chess::boardAnnotations>(this->turn ^ chess::boardAnnotations::white);
	// Remove the piece from it's origin square
	this->bitboards[chess::util::colorOf(desiredMove.movePiece())] ^= desiredMove.originSquare() | desiredMove.destinationSquare();    // Colour only
	this->bitboards[desiredMove.movePiece()] ^= desiredMove.originSquare() | desiredMove.destinationSquare();                          // Colour and Piece
	this->pieceAtIndex[desiredMove.originIndex] = desiredMove.movePiece();                                                             // Set index value to Empty
	switch (desiredMove.moveFlags()) {
		case chess::moveFlags::capture:    // Capture
			this->bitboards[chess::util::colorOf(desiredMove.capturedPiece())] |= desiredMove.destinationSquare();
			this->bitboards[desiredMove.capturedPiece()] |= desiredMove.destinationSquare();
			this->pieceAtIndex[desiredMove.destinationIndex] = desiredMove.capturedPiece();
			break;

		case chess::moveFlags::capturePromotionBegin... chess::moveFlags::capturePromotionEnd:                        // Capture (Promotion)
			this->bitboards[chess::util::colorOf(desiredMove.capturedPiece())] ^= desiredMove.destinationSquare();    // Delete captured piece's colour
			this->bitboards[desiredMove.capturedPiece()] ^= desiredMove.destinationSquare();                          // Delete captured piece
			this->bitboards[desiredMove.promotionPiece()] ^= desiredMove.destinationSquare();                         // Colour and Piece
			this->bitboards[desiredMove.movePiece()] ^= desiredMove.destinationSquare();
			this->pieceAtIndex[desiredMove.destinationIndex] = desiredMove.capturedPiece();
			break;

		case chess::moveFlags::quietPromotionBegin... chess::moveFlags::quietPromotionEnd:    // Promotion
			this->bitboards[desiredMove.movePiece()] ^= desiredMove.destinationSquare();
			this->bitboards[desiredMove.promotionPiece()] ^= desiredMove.destinationSquare();    // Colour and Piece
			[[fallthrough]];
		case chess::moveFlags::whiteDoublePush:
			[[fallthrough]];
		case chess::moveFlags::blackDoublePush:
			[[fallthrough]];
		case chess::moveFlags::quiet:
			this->pieceAtIndex[desiredMove.destinationIndex] = empty;    // fill index value
			break;

		case chess::moveFlags::whiteKingsideCastle:    // White Kingside
			this->bitboards[white] ^= 0b0101;          // flip the bits for the white bitboard
			this->bitboards[whiteRook] ^= 0b0101;      // Flip the bits for the castling rook
			this->pieceAtIndex[h1] = whiteRook;
			this->pieceAtIndex[g1] = empty;
			this->pieceAtIndex[f1] = empty;
			break;
		case chess::moveFlags::whiteQueensideCastle:     // White Queenside
			this->bitboards[white] ^= 0b10010000;        // flip the bits for the white bitboard
			this->bitboards[whiteRook] ^= 0b10010000;    // Flip the bits for the castling rook
			this->pieceAtIndex[a1] = whiteRook;
			this->pieceAtIndex[c1] = empty;
			this->pieceAtIndex[d1] = empty;
			break;
		case chess::moveFlags::blackKingsideCastle:                 // Black Kingside
			this->bitboards[black] ^= 0x0500000000000000ULL;        // flip the bits for the black bitboard
			this->bitboards[blackRook] ^= 0x0500000000000000ULL;    // Flip the bits for the castling rook
			this->pieceAtIndex[h8] = blackRook;
			this->pieceAtIndex[g8] = empty;
			this->pieceAtIndex[f8] = empty;
			break;
		case chess::moveFlags::blackQueensideCastle:                // Black Queenside
			this->bitboards[black] ^= 0x9000000000000000ULL;        // flip the bits for the black bitboard
			this->bitboards[blackRook] ^= 0x9000000000000000ULL;    // Flip the bits for the castling rook
			this->pieceAtIndex[a8] = blackRook;
			this->pieceAtIndex[c8] = empty;
			this->pieceAtIndex[d8] = empty;
			break;
		case chess::moveFlags::whiteEnPassant:    // White taking black en passent
			this->bitboards[black] ^= (desiredMove.destinationSquare() >> 8);
			this->bitboards[blackPawn] ^= (desiredMove.destinationSquare() >> 8);
			this->pieceAtIndex[desiredMove.destinationIndex]     = empty;
			this->pieceAtIndex[desiredMove.destinationIndex - 8] = blackPawn;
			break;
		case chess::moveFlags::blackEnPassant:    // Black taking white en passent
			this->bitboards[white] ^= (desiredMove.destinationSquare() << 8);
			this->bitboards[whitePawn] ^= (desiredMove.destinationSquare() << 8);
			this->pieceAtIndex[desiredMove.destinationIndex]     = empty;
			this->pieceAtIndex[desiredMove.destinationIndex + 8] = whitePawn;
			break;
		default:
			std::cerr << "Unknown Move: " << std::hex << desiredMove.moveFlags() << '\n';
			assert(false);
			break;
	}

	this->bitboards[occupied] = this->bitboards[white] | this->bitboards[black];
	this->bitboards[empty]    = ~this->bitboards[occupied];
	this->setZobrist();
}
