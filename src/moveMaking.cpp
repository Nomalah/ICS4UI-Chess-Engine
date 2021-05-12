#include <iomanip>
#include <iostream>

#include "../include/chess.h"

void chess::game::move(chess::move desiredMove) {
	gameHistory.push_back(gameHistory.back().move(desiredMove));  // Throws error?
}

chess::position chess::position::move(chess::move desiredMove) {
	chess::position result = *this;
	result.enPassantTargetSquare = 0x0;
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

		case 0x2000:													 // White Kingside
			result.bitboards[chess::util::constants::white] ^= 0x7;		 // flip the bits for the white bitboard
			result.bitboards[chess::util::constants::whiteRook] ^= 0x5;	 // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::whiteKing] = 0x2;
			result.insertPieceAtIndex(0, chess::util::constants::whiteNull);
			result.insertPieceAtIndex(1, chess::util::constants::whiteKing);
			result.insertPieceAtIndex(2, chess::util::constants::whiteRook);
			break;
		case 0x3000:													  // White Queenside
			result.bitboards[chess::util::constants::white] ^= 0xB0;	  // flip the bits for the white bitboard
			result.bitboards[chess::util::constants::whiteRook] ^= 0x90;  // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::whiteKing] = 0x20;
			result.insertPieceAtIndex(7, chess::util::constants::whiteNull);
			result.insertPieceAtIndex(5, chess::util::constants::whiteKing);
			result.insertPieceAtIndex(4, chess::util::constants::whiteRook);
			break;
		case 0x4000:																	   // Black Kingside
			result.bitboards[chess::util::constants::black] ^= 0x0700000000000000ULL;	   // flip the bits for the black bitboard
			result.bitboards[chess::util::constants::blackRook] ^= 0x0500000000000000ULL;  // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::blackKing] = 0x0200000000000000ULL;
			result.insertPieceAtIndex(56, chess::util::constants::blackNull);
			result.insertPieceAtIndex(57, chess::util::constants::blackKing);
			result.insertPieceAtIndex(58, chess::util::constants::blackRook);
			break;
		case 0x5000:																	   // Black Queenside
			result.bitboards[chess::util::constants::black] ^= 0xB000000000000000ULL;	   // flip the bits for the black bitboard
			result.bitboards[chess::util::constants::blackRook] ^= 0x9000000000000000ULL;  // Flip the bits for the castling rook
			result.bitboards[chess::util::constants::blackKing] = 0x2000000000000000ULL;
			result.insertPieceAtIndex(63, chess::util::constants::whiteNull);
			result.insertPieceAtIndex(61, chess::util::constants::blackKing);
			result.insertPieceAtIndex(60, chess::util::constants::blackRook);
			break;
		case 0x6000:  // White taking black en passent
			result.bitboards[chess::util::constants::white] |= desiredMove.destinationSquare();
			result.bitboards[chess::util::constants::whitePawn] |= desiredMove.destinationSquare();
			result.bitboards[chess::util::constants::black] &= ~(desiredMove.destinationSquare() >> 8);
			result.bitboards[chess::util::constants::blackPawn] &= ~(desiredMove.destinationSquare() >> 8);
			result.insertPieceAtIndex(desiredMove.destination, chess::util::constants::whitePawn);
			result.insertPieceAtIndex(desiredMove.destination - 8, chess::util::constants::blackNull);
			break;
		case 0x7000:  // Black taking white en passent
			result.bitboards[chess::util::constants::black] |= desiredMove.destinationSquare();
			result.bitboards[chess::util::constants::blackPawn] |= desiredMove.destinationSquare();
			result.bitboards[chess::util::constants::white] &= ~(desiredMove.destinationSquare() << 8);
			result.bitboards[chess::util::constants::whitePawn] &= ~(desiredMove.destinationSquare() << 8);
			result.insertPieceAtIndex(desiredMove.destination, chess::util::constants::blackPawn);
			result.insertPieceAtIndex(desiredMove.destination + 8, chess::util::constants::whiteNull);
			break;
		case 0x8000:																				 // White double pawn push
			result.bitboards[chess::util::constants::white] |= desiredMove.destinationSquare();		 // Colour only
			result.bitboards[chess::util::constants::whitePawn] |= desiredMove.destinationSquare();	 // Colour and Piece
			result.insertPieceAtIndex(desiredMove.destination, chess::util::constants::whitePawn);	 // fill index value
			result.enPassantTargetSquare = desiredMove.destinationSquare() >> 8;
			break;
		case 0x9000:																				 // Black double pawn push
			result.bitboards[chess::util::constants::black] |= desiredMove.destinationSquare();		 // Colour only
			result.bitboards[chess::util::constants::blackPawn] |= desiredMove.destinationSquare();	 // Colour and Piece
			result.insertPieceAtIndex(desiredMove.destination, chess::util::constants::blackPawn);	 // fill index value
			result.enPassantTargetSquare = desiredMove.destinationSquare() << 8;
			break;
		default:
			std::cerr << "Unknown Move: " << std::hex << desiredMove.moveFlags() << '\n';
			assert(false);
			break;
	}

    // Invalidate castling
    switch (desiredMove.movePiece()){
        case chess::util::constants::blackRook:
			result.flags &= desiredMove.origin == 56 ? ~0x20 : (desiredMove.origin == 63 ? ~0x10 : ~0x0);
			break;
		case chess::util::constants::blackKing:
			result.flags &= ~(0x20 | 0x10);
			break;
		case chess::util::constants::whiteRook:
			result.flags &= desiredMove.origin == 0 ? ~0x80 : (desiredMove.origin == 7 ? ~0x40 : ~0x0);
			break;
		case chess::util::constants::whiteKing:
            result.flags &= ~(0x80 | 0x40);
            break;
        default:
			break;
	}

    // Invalidate castling
    switch (desiredMove.capturedPiece()){
        case chess::util::constants::blackRook:
			result.flags &= desiredMove.destination == 56 ? ~0x20 : (desiredMove.destination == 63 ? ~0x10 : ~0x0);
			break;
		case chess::util::constants::whiteRook:
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