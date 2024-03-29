#include <iostream>
#include <iomanip>
#include <utility>
#include <cassert>

#include "chess.hpp"

void chess::game::move(const chess::moveData desiredMove) noexcept {
	gameHistory.push_back(gameHistory.back().move(desiredMove));
}

[[nodiscard]] chess::position chess::position::move(chess::moveData desiredMove) const noexcept {
	using namespace chess::constants;
	chess::position result         = *this;
	result.enPassantTargetBitboard = 0x0;
	result.halfMoveClock++;
	if (this->turn() == chess::piece::black)
		result.fullMoveClock++;
	result.flags ^= chess::piece::white;
	// Remove the piece from it's origin square
	result.bitboards[chess::util::colorOf(desiredMove.movePiece())] &= ~desiredMove.originSquare();    // Colour only
	result.bitboards[desiredMove.movePiece()] &= ~desiredMove.originSquare();                          // Colour and Piece
	result.pieceAtIndex[desiredMove.originIndex] = chess::util::nullOf(desiredMove.movePiece());       // Set index value to null / Empty
	if (desiredMove.moveFlags() == 0x0000) {
        result.bitboards[chess::util::colorOf(desiredMove.movePiece())] |= desiredMove.destinationSquare();    // Colour only
        result.bitboards[desiredMove.movePiece()] |= desiredMove.destinationSquare();                          // Colour and Piece
        result.pieceAtIndex[desiredMove.destinationIndex] = desiredMove.movePiece(); 
    } else if (desiredMove.moveFlags() == 0x1000) {
        result.halfMoveClock = 0;                                                                                   // Capture
        result.bitboards[chess::util::colorOf(desiredMove.capturedPiece())] &= ~desiredMove.destinationSquare();    // Delete captured piece's colour
        result.bitboards[desiredMove.capturedPiece()] &= ~desiredMove.destinationSquare(); // fill index value                         // Delete captured piece
        result.bitboards[chess::util::colorOf(desiredMove.movePiece())] |= desiredMove.destinationSquare();    // Colour only
        result.bitboards[desiredMove.movePiece()] |= desiredMove.destinationSquare();                          // Colour and Piece
        result.pieceAtIndex[desiredMove.destinationIndex] = desiredMove.movePiece(); 
    } else if (desiredMove.moveFlags() >= 0x1100 && desiredMove.moveFlags() <= 0x1F00) {                                                                                   // Capture (Promotion)
        result.bitboards[chess::util::colorOf(desiredMove.capturedPiece())] &= ~desiredMove.destinationSquare();    // Delete captured piece's colour
        result.bitboards[desiredMove.capturedPiece()] &= ~desiredMove.destinationSquare();                          // Delete captured piece                                                                                         // Promotion
        result.bitboards[chess::util::colorOf(desiredMove.promotionPiece())] |= desiredMove.destinationSquare();    // Colour only
        result.bitboards[desiredMove.promotionPiece()] |= desiredMove.destinationSquare();                          // Colour and Piece
        result.pieceAtIndex[desiredMove.destinationIndex] = desiredMove.promotionPiece();                           // fill index value
    } else if (desiredMove.moveFlags() >= 0x0100 && desiredMove.moveFlags() <= 0x0F00) {                                          // Promotion
        result.bitboards[chess::util::colorOf(desiredMove.promotionPiece())] |= desiredMove.destinationSquare();    // Colour only
        result.bitboards[desiredMove.promotionPiece()] |= desiredMove.destinationSquare();                          // Colour and Piece
        result.pieceAtIndex[desiredMove.destinationIndex] = desiredMove.promotionPiece();                           // fill index value
	} else if (desiredMove.moveFlags() == 0x2000) {    // White Kingside
        result.bitboards[white] ^= 0x7;               // flip the bits for the white bitboard
        result.bitboards[whiteRook] ^= 0x5;           // Flip the bits for the castling rook
        result.bitboards[whiteKing] = 0x2;
        result.pieceAtIndex[h1]     = piece::empty;
        result.pieceAtIndex[g1]     = whiteKing;
        result.pieceAtIndex[f1]     = whiteRook;
    } else if (desiredMove.moveFlags() == 0x3000) {    // White Queenside
        result.bitboards[white] ^= 0xB0;        // flip the bits for the white bitboard
        result.bitboards[whiteRook] ^= 0x90;    // Flip the bits for the castling rook
        result.bitboards[whiteKing] = 0x20;
        result.pieceAtIndex[a1]     = piece::empty;
        result.pieceAtIndex[c1]     = whiteKing;
        result.pieceAtIndex[d1]     = whiteRook;
	} else if (desiredMove.moveFlags() == 0x4000) {                                                // Black Kingside
        result.bitboards[black] ^= 0x0700000000000000ULL;        // flip the bits for the black bitboard
        result.bitboards[blackRook] ^= 0x0500000000000000ULL;    // Flip the bits for the castling rook
        result.bitboards[blackKing] = 0x0200000000000000ULL;
        result.pieceAtIndex[h8]     = piece::empty;
        result.pieceAtIndex[g8]     = blackKing;
        result.pieceAtIndex[f8]     = blackRook;
	} else if (desiredMove.moveFlags() == 0x5000) {                                                // Black Queenside
        result.bitboards[black] ^= 0xB000000000000000ULL;        // flip the bits for the black bitboard
        result.bitboards[blackRook] ^= 0x9000000000000000ULL;    // Flip the bits for the castling rook
        result.bitboards[blackKing] = 0x2000000000000000ULL;
        result.pieceAtIndex[a8]     = piece::empty;
        result.pieceAtIndex[c8]     = blackKing;
        result.pieceAtIndex[d8]     = blackRook;
	} else if (desiredMove.moveFlags() == 0x6000) {   // White taking black en passent
        result.bitboards[white] |= desiredMove.destinationSquare();
        result.bitboards[whitePawn] |= desiredMove.destinationSquare();
        result.bitboards[black] &= ~(desiredMove.destinationSquare() >> 8);
        result.bitboards[blackPawn] &= ~(desiredMove.destinationSquare() >> 8);
        result.pieceAtIndex[desiredMove.destinationIndex]     = whitePawn;
        result.pieceAtIndex[desiredMove.destinationIndex - 8] = piece::empty;
	} else if (desiredMove.moveFlags() == 0x7000) {    // Black taking white en passent
        result.bitboards[black] |= desiredMove.destinationSquare();
        result.bitboards[blackPawn] |= desiredMove.destinationSquare();
        result.bitboards[white] &= ~(desiredMove.destinationSquare() << 8);
        result.bitboards[whitePawn] &= ~(desiredMove.destinationSquare() << 8);
        result.pieceAtIndex[desiredMove.destinationIndex]     = blackPawn;
        result.pieceAtIndex[desiredMove.destinationIndex + 8] = piece::empty;
	} else if (desiredMove.moveFlags() == 0x8000) {                                                          // White double pawn push
        result.bitboards[white] |= desiredMove.destinationSquare();        // Colour only
        result.bitboards[whitePawn] |= desiredMove.destinationSquare();    // Colour and Piece
        result.pieceAtIndex[desiredMove.destinationIndex] = whitePawn;     // fill index value
        result.enPassantTargetBitboard                      = desiredMove.destinationSquare() >> 8;
	} else if (desiredMove.moveFlags() == 0x9000) {                                                        // Black double pawn push
        result.bitboards[black] |= desiredMove.destinationSquare();        // Colour only
        result.bitboards[blackPawn] |= desiredMove.destinationSquare();    // Colour and Piece
        result.pieceAtIndex[desiredMove.destinationIndex] = blackPawn;     // fill index value
        result.enPassantTargetBitboard                      = desiredMove.destinationSquare() << 8;
    } else {
        std::cerr << "Unknown Move: " << std::hex << desiredMove.moveFlags() << '\n';
        assert(false);
	}

	// Invalidate castling
	switch (desiredMove.movePiece()) {
		case blackRook:
			result.flags &= desiredMove.originIndex == 56 ? ~0x20 : (desiredMove.originIndex == 63 ? ~0x10 : ~0x0);
			break;
		case blackKing:
			result.flags &= ~(0x20 | 0x10);
			break;
		case whiteRook:
			result.flags &= desiredMove.originIndex == 0 ? ~0x80 : (desiredMove.originIndex == 7 ? ~0x40 : ~0x0);
			break;
		case whiteKing:
			result.flags &= ~(0x80 | 0x40);
			break;
		case whitePawn:
			[[fallthrough]];
		case blackPawn:
			result.halfMoveClock = 0;
			break;
		default:
			break;
	}

	// Invalidate castling
	switch (desiredMove.capturedPiece()) {
		case blackRook:
			result.flags &= desiredMove.destinationIndex == 56 ? ~0x20 : (desiredMove.destinationIndex == 63 ? ~0x10 : ~0x0);
			break;
		case whiteRook:
			result.flags &= desiredMove.destinationIndex == 0 ? ~0x80 : (desiredMove.destinationIndex == 7 ? ~0x40 : ~0x0);
			break;
		default:
			break;
	}

	result.bitboards[piece::occupied] = result.bitboards[white] | result.bitboards[black];
	result.bitboards[piece::empty]    = ~result.bitboards[piece::occupied];
	result.setZobrist();
	return result;
}

void chess::game::move(const std::string& uciMove) noexcept {
	chess::moveData libraryMove {
		.flags            = 0,
		.originIndex      = chess::util::algebraicToSquare(uciMove.substr(0, 2)),
		.destinationIndex = chess::util::algebraicToSquare(uciMove.substr(2, 2))
	};
	libraryMove.flags |= this->currentPosition().pieceAtIndex[libraryMove.destinationIndex];
	libraryMove.flags |= this->currentPosition().pieceAtIndex[libraryMove.originIndex] << 4;

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

	this->move(libraryMove);
}

bool chess::game::undo() noexcept {
	if (gameHistory.size() > 1) {
		gameHistory.pop_back();
		return true;
	} else {
		return false;
	}
}
