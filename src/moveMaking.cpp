#include <iomanip>
#include <iostream>

#include "../include/chess.h"

void chess::game::move(chess::moveData desiredMove) {
	gameHistory.push_back(gameHistory.back().move(desiredMove));  // Throws error?
}

chess::position chess::position::move(chess::moveData desiredMove) {
    using namespace chess::util::constants;
	chess::position result = *this;
	result.enPassantTargetSquare = 0x0;
	chess::position::nextTurn(result);
	// Remove the piece from it's origin square
	result.bitboards[chess::util::colorOf(desiredMove.movePiece())] &= ~desiredMove.originSquare();	 // Colour only
	result.bitboards[desiredMove.movePiece()] &= ~desiredMove.originSquare();						 // Colour and Piece
	result.pieceAtIndex[desiredMove.origin] = chess::util::nullOf(desiredMove.movePiece());	 // Set index value to null / Empty
	switch (desiredMove.moveFlags()) {
		case 0x1000:																								  // Capture
			result.bitboards[chess::util::colorOf(desiredMove.capturedPiece())] &= ~desiredMove.destinationSquare();  // Delete captured piece's colour
			result.bitboards[desiredMove.capturedPiece()] &= ~desiredMove.destinationSquare();						  // Delete captured piece
			[[fallthrough]];
		case 0x0000:																							 // Quiet move
			result.bitboards[chess::util::colorOf(desiredMove.movePiece())] |= desiredMove.destinationSquare();	 // Colour only
			result.bitboards[desiredMove.movePiece()] |= desiredMove.destinationSquare();						 // Colour and Piece
			result.pieceAtIndex[desiredMove.destination] = desiredMove.movePiece();						 // fill index value
			break;

		case 0x1100 ... 0x1F00:																						  // Capture (Promotion)
			result.bitboards[chess::util::colorOf(desiredMove.capturedPiece())] &= ~desiredMove.destinationSquare();  // Delete captured piece's colour
			result.bitboards[desiredMove.capturedPiece()] &= ~desiredMove.destinationSquare();						  // Delete captured piece
			[[fallthrough]];
		case 0x0100 ... 0x0F00:																						  // Promotion
			result.bitboards[chess::util::colorOf(desiredMove.promotionPiece())] |= desiredMove.destinationSquare();  // Colour only
			result.bitboards[desiredMove.promotionPiece()] |= desiredMove.destinationSquare();						  // Colour and Piece
			result.pieceAtIndex[desiredMove.destination] = desiredMove.promotionPiece();						  // fill index value
			break;

		case 0x2000:													 // White Kingside
			result.bitboards[white] ^= 0x7;		 // flip the bits for the white bitboard
			result.bitboards[whiteRook] ^= 0x5;	 // Flip the bits for the castling rook
			result.bitboards[whiteKing] = 0x2;
			result.pieceAtIndex[h1] = whiteNull;
			result.pieceAtIndex[g1] = whiteKing;
			result.pieceAtIndex[f1] = whiteRook;
			break;
		case 0x3000:													  // White Queenside
			result.bitboards[white] ^= 0xB0;	  // flip the bits for the white bitboard
			result.bitboards[whiteRook] ^= 0x90;  // Flip the bits for the castling rook
			result.bitboards[whiteKing] = 0x20;
			result.pieceAtIndex[a1] = whiteNull;
			result.pieceAtIndex[c1] = whiteKing;
			result.pieceAtIndex[d1] = whiteRook;
			break;
		case 0x4000:																	   // Black Kingside
			result.bitboards[black] ^= 0x0700000000000000ULL;	   // flip the bits for the black bitboard
			result.bitboards[blackRook] ^= 0x0500000000000000ULL;  // Flip the bits for the castling rook
			result.bitboards[blackKing] = 0x0200000000000000ULL;
			result.pieceAtIndex[h8] = blackNull;
			result.pieceAtIndex[g8] = blackKing;
			result.pieceAtIndex[f8] = blackRook;
			break;
		case 0x5000:																	   // Black Queenside
			result.bitboards[black] ^= 0xB000000000000000ULL;	   // flip the bits for the black bitboard
			result.bitboards[blackRook] ^= 0x9000000000000000ULL;  // Flip the bits for the castling rook
			result.bitboards[blackKing] = 0x2000000000000000ULL;
			result.pieceAtIndex[a8] = whiteNull;
			result.pieceAtIndex[c8] = blackKing;
			result.pieceAtIndex[d8] = blackRook;
			break;
		case 0x6000:  // White taking black en passent
			result.bitboards[white] |= desiredMove.destinationSquare();
			result.bitboards[whitePawn] |= desiredMove.destinationSquare();
			result.bitboards[black] &= ~(desiredMove.destinationSquare() >> 8);
			result.bitboards[blackPawn] &= ~(desiredMove.destinationSquare() >> 8);
			result.pieceAtIndex[desiredMove.destination] = whitePawn;
			result.pieceAtIndex[desiredMove.destination - 8] = blackNull;
			break;
		case 0x7000:  // Black taking white en passent
			result.bitboards[black] |= desiredMove.destinationSquare();
			result.bitboards[blackPawn] |= desiredMove.destinationSquare();
			result.bitboards[white] &= ~(desiredMove.destinationSquare() << 8);
			result.bitboards[whitePawn] &= ~(desiredMove.destinationSquare() << 8);
			result.pieceAtIndex[desiredMove.destination] = blackPawn;
			result.pieceAtIndex[desiredMove.destination - 8] = whiteNull;
			break;
		case 0x8000:																				 // White double pawn push
			result.bitboards[white] |= desiredMove.destinationSquare();		 // Colour only
			result.bitboards[whitePawn] |= desiredMove.destinationSquare();	 // Colour and Piece
			result.pieceAtIndex[desiredMove.destination] = whitePawn;	 // fill index value
			result.enPassantTargetSquare = desiredMove.destinationSquare() >> 8;
			break;
		case 0x9000:																				 // Black double pawn push
			result.bitboards[black] |= desiredMove.destinationSquare();		 // Colour only
			result.bitboards[blackPawn] |= desiredMove.destinationSquare();	 // Colour and Piece
			result.pieceAtIndex[desiredMove.destination] = blackPawn;	 // fill index value
			result.enPassantTargetSquare = desiredMove.destinationSquare() << 8;
			break;
		default:
			std::cerr << "Unknown Move: " << std::hex << desiredMove.moveFlags() << '\n';
			assert(false);
			break;
	}

    // Invalidate castling
    switch (desiredMove.movePiece()){
        case blackRook:
			result.flags &= desiredMove.origin == 56 ? ~0x20 : (desiredMove.origin == 63 ? ~0x10 : ~0x0);
			break;
		case blackKing:
			result.flags &= ~(0x20 | 0x10);
			break;
		case whiteRook:
			result.flags &= desiredMove.origin == 0 ? ~0x80 : (desiredMove.origin == 7 ? ~0x40 : ~0x0);
			break;
		case whiteKing:
            result.flags &= ~(0x80 | 0x40);
            break;
        default:
			break;
	}

    // Invalidate castling
    switch (desiredMove.capturedPiece()){
        case blackRook:
			result.flags &= desiredMove.destination == 56 ? ~0x20 : (desiredMove.destination == 63 ? ~0x10 : ~0x0);
			break;
		case whiteRook:
			result.flags &= desiredMove.destination == 0 ? ~0x80 : (desiredMove.destination == 7 ? ~0x40 : ~0x0);
			break;
        default:
			break;
	}
	return result;
}

bool chess::game::undo() {
	if (gameHistory.size() > 1) {
		gameHistory.pop_back();
		return true;
	} else {
		return false;
	}
}