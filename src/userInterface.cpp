#include <array>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "../include/chess.h"

std::string chess::position::ascii() {
	std::string result = " +--------+\n";
	result.reserve(132);  // Size of final string

	chess::u64 targetSquare = chess::util::iter;
	for (std::size_t y = 8; y > 0; y--) {
		result += (char)(y + 48);
		result += "|";
		for (std::size_t x = 8; x > 0; x--) {
			result += chess::util::constants::pieceToChar(this->pieceAtIndex[y * 8 + x - 9]);
			targetSquare >>= 1;
		}

		result += "|\n";
	}

	result += " +--------+\n  abcdefgh";
	return result;
}

chess::position chess::position::fromFen(std::string fen) {
	if (chess::position::validateFen(fen)) {
		// Empty position with a valid flag enabled
		chess::position result = {{}, {}, 0x04, 0, 0};
		std::stringstream toTokenize(fen);
		std::string output;
		std::array<std::string, 6> tokens;
		auto iter = tokens.begin();
		while (std::getline(toTokenize, output, ' ')) {
			*iter++ = output;
		}

		// Fill board with fen pieces
		u8 insertIndex = 63;
		for (char piece : tokens[0]) {
			u64 insertAtSquare = chess::util::bitboardFromIndex(insertIndex);
			//std::cout << chess::debugging::bitboardToString(insertAtSquare) << std::endl;
			if (piece > '0' && piece <= '9') {
                for(std::size_t _ = 0; _ < piece - '0'; _++, insertIndex--){
					result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whiteNull;
				}
			}
			else if (piece != '/') {
				switch (piece) {
					case 'p':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::blackPawn;
						result.bitboards[chess::util::constants::boardAnnotations::black] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::blackPawn] |= insertAtSquare;
						break;
					case 'P':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whitePawn;
						result.bitboards[chess::util::constants::boardAnnotations::white] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::whitePawn] |= insertAtSquare;
						break;
					case 'n':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::blackKnight;
						result.bitboards[chess::util::constants::boardAnnotations::black] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::blackKnight] |= insertAtSquare;
						break;
					case 'N':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whiteKnight;
						result.bitboards[chess::util::constants::boardAnnotations::white] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::whiteKnight] |= insertAtSquare;
						break;
					case 'b':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::blackBishop;
						result.bitboards[chess::util::constants::boardAnnotations::black] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::blackBishop] |= insertAtSquare;
						break;
					case 'B':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whiteBishop;
						result.bitboards[chess::util::constants::boardAnnotations::white] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::whiteBishop] |= insertAtSquare;
						break;
					case 'r':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::blackRook;
						result.bitboards[chess::util::constants::boardAnnotations::black] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::blackRook] |= insertAtSquare;
						break;
					case 'R':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whiteRook;
						result.bitboards[chess::util::constants::boardAnnotations::white] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::whiteRook] |= insertAtSquare;
						break;
					case 'q':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::blackQueen;
						result.bitboards[chess::util::constants::boardAnnotations::black] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::blackQueen] |= insertAtSquare;
						break;
					case 'Q':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whiteQueen;
						result.bitboards[chess::util::constants::boardAnnotations::white] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::whiteQueen] |= insertAtSquare;
						break;
					case 'k':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::blackKing;
						result.bitboards[chess::util::constants::boardAnnotations::black] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::blackKing] |= insertAtSquare;
						break;
					case 'K':
						result.pieceAtIndex[insertIndex] = chess::util::constants::boardAnnotations::whiteKing;
						result.bitboards[chess::util::constants::boardAnnotations::white] |= insertAtSquare;
						result.bitboards[chess::util::constants::boardAnnotations::whiteKing] |= insertAtSquare;
						break;
				}
				insertIndex -= 1;
			}
		}

		// Set turn flag
		result.flags |= tokens[1] == "w" ? 0x08 : 0x00;

		// Set castling flags
		for (char flag : tokens[2]) {
			switch (flag) {
				case 'K':
					result.flags |= 0x80;
					break;
				case 'Q':
					result.flags |= 0x40;
					break;
				case 'k':
					result.flags |= 0x20;
					break;
				case 'q':
					result.flags |= 0x10;
					break;
			}
		}

		// Set en passant target square
		result.enPassantTargetSquare = chess::util::bitboardFromIndex(chess::util::algebraicToSquare(tokens[3]));

		// Set halfmove clock
		result.halfMoveClock = std::stoul(tokens[4]);

		// Set fullmove number
		// (Unused and not implimented)
		return result;
	} else {
		// Empty position with valid flag not set
		return {{}, {}, 0, 0, 0};
	}
}

bool chess::game::finished() {
	return false;
}

chess::game chess::defaultGame() {
	std::vector<position> gameHistory;
	gameHistory.push_back(position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	return {gameHistory};
}