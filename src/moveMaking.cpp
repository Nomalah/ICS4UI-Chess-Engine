#include "../include/chess.h"
#include <iostream>

void chess::game::move(chess::move desiredMove) {
	gameHistory.push_back(gameHistory.back().move(desiredMove)); // Throws error?
}

chess::position chess::position::move(chess::move desiredMove) {
	chess::position result = *this;
	chess::position::nextTurn(result);
	// Remove the piece from it's origin square
	result.bitboards[chess::util::colorOf(desiredMove.movePiece())] &= ~desiredMove.originSquare();	 // Colour only
	result.bitboards[desiredMove.movePiece()] &= ~desiredMove.originSquare();						 // Colour and Piece
	result.insertPieceAtIndex(desiredMove.origin, chess::util::nullOf(desiredMove.movePiece()));	 // Set index value to null / Empty
	switch (desiredMove.moveFlags()) {
		case 0x1000:																								  // Capture
			result.bitboards[chess::util::colorOf(desiredMove.capturedPiece())] &= ~desiredMove.destinationSquare();  // Delete captured piece's colour
			result.bitboards[desiredMove.capturedPiece()] &= ~desiredMove.destinationSquare();						  // Delete captured piece
			[[fallthrough]];
		case 0x0000:																							 // Quiet move
			result.bitboards[chess::util::colorOf(desiredMove.movePiece())] |= desiredMove.destinationSquare();	 // Colour only
			result.bitboards[desiredMove.movePiece()] |= desiredMove.destinationSquare();						 // Colour and Piece
			result.insertPieceAtIndex(desiredMove.destination, desiredMove.movePiece());						 // fill index value
			break;

		case 0x1100 ... 0x1F00:																						  // Capture (Promotion)
			result.bitboards[chess::util::colorOf(desiredMove.capturedPiece())] &= ~desiredMove.destinationSquare();  // Delete captured piece's colour
			result.bitboards[desiredMove.capturedPiece()] &= ~desiredMove.destinationSquare();						  // Delete captured piece
			[[fallthrough]];
		case 0x0100 ... 0x0F00:																						  // Promotion
			result.bitboards[chess::util::colorOf(desiredMove.promotionPiece())] |= desiredMove.destinationSquare();  // Colour only
			result.bitboards[desiredMove.promotionPiece()] |= desiredMove.destinationSquare();						  // Colour and Piece
			result.insertPieceAtIndex(desiredMove.destination, desiredMove.promotionPiece());						  // fill index value
			break;
        
        case 0x2000: // White Kingside
			result.bitboards[chess::util::constants::whiteRook] ^= 0x5; // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::whiteKing] = 0x2;
			result.insertPieceAtIndex(0, chess::util::constants::whiteNull);
			result.insertPieceAtIndex(1, chess::util::constants::whiteKing);
			result.insertPieceAtIndex(2, chess::util::constants::whiteRook);
			break;
		case 0x3000: // White Queenside
			result.bitboards[chess::util::constants::whiteRook] ^= 0x90;	 // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::whiteKing] = 0x20;
			result.insertPieceAtIndex(7, chess::util::constants::whiteNull);
			result.insertPieceAtIndex(5, chess::util::constants::whiteKing);
			result.insertPieceAtIndex(4, chess::util::constants::whiteRook);
			break;
		case 0x4000: // Black Kingside
			result.bitboards[chess::util::constants::blackRook] ^= 0x0500000000000000ULL;  // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::blackKing] = 0x0200000000000000ULL;
			result.insertPieceAtIndex(56, chess::util::constants::blackNull);
			result.insertPieceAtIndex(57, chess::util::constants::blackKing);
			result.insertPieceAtIndex(58, chess::util::constants::blackRook);
			break;
		case 0x5000: // Black Queenside
			result.bitboards[chess::util::constants::blackRook] ^= 0x9000000000000000ULL;  // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::blackKing] = 0x2000000000000000ULL;
			result.insertPieceAtIndex(63, chess::util::constants::whiteNull);
			result.insertPieceAtIndex(61, chess::util::constants::blackKing);
			result.insertPieceAtIndex(60, chess::util::constants::blackRook);
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