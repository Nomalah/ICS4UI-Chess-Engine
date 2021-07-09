#include <array>
#include <sstream>
#include <string>

#include "../include/chess.h"

[[nodiscard]] std::string chess::position::ascii() const noexcept {
	std::string result { " +--------+\n" };
	result.reserve(132);    // Size of final string

	for (chess::u64 targetSquare { chess::util::constants::bitboardIter }, y { 8 }; y > 0; y--) {
		result += static_cast<char>(y + '0');
		result += '|';
		for (std::size_t x = 8; x > 0; x--, targetSquare >>= 1) {
			result += chess::util::pieceToChar(this->pieceAtIndex[y * 8 + x - 9]);
		}
		result += "|\n";
	}

	result += " +--------+\n  abcdefgh";
	return result;
}

[[nodiscard]] chess::position chess::position::fromFen(const std::string& fen) noexcept {
	using namespace chess::util::constants;
	if (chess::position::validateFen(fen)) {
		// Empty position with a valid flag enabled
		chess::position result {
			.bitboards             = {},
			.pieceAtIndex          = {},
			.flags                 = 0x04,
			.halfMoveClock         = 0,
			.enPassantTargetSquare = 0,
			.zobristHash           = 0
		};
		std::stringstream toTokenize { fen };
		std::array<std::string, 6> tokens;
		for (auto& token : tokens) {
			std::getline(toTokenize, token, ' ');
		}

		// Fill board with fen pieces
		u8 insertIndex = 63;
		for (const char piece : tokens[0]) {
			if (piece > '0' && piece <= '9') {
				for (char _ = 0; _ < piece - '0'; _++, insertIndex--) {
					result.pieceAtIndex[insertIndex] = boardAnnotations::null;
				}
			} else if (piece != '/') {
				switch (u64 insertAtSquare = chess::util::bitboardFromIndex(insertIndex); piece) {
					case 'p':
						result.pieceAtIndex[insertIndex] = boardAnnotations::blackPawn;
						result.bitboards[boardAnnotations::black] |= insertAtSquare;
						result.bitboards[boardAnnotations::blackPawn] |= insertAtSquare;
						break;
					case 'P':
						result.pieceAtIndex[insertIndex] = boardAnnotations::whitePawn;
						result.bitboards[boardAnnotations::white] |= insertAtSquare;
						result.bitboards[boardAnnotations::whitePawn] |= insertAtSquare;
						break;
					case 'n':
						result.pieceAtIndex[insertIndex] = boardAnnotations::blackKnight;
						result.bitboards[boardAnnotations::black] |= insertAtSquare;
						result.bitboards[boardAnnotations::blackKnight] |= insertAtSquare;
						break;
					case 'N':
						result.pieceAtIndex[insertIndex] = boardAnnotations::whiteKnight;
						result.bitboards[boardAnnotations::white] |= insertAtSquare;
						result.bitboards[boardAnnotations::whiteKnight] |= insertAtSquare;
						break;
					case 'b':
						result.pieceAtIndex[insertIndex] = boardAnnotations::blackBishop;
						result.bitboards[boardAnnotations::black] |= insertAtSquare;
						result.bitboards[boardAnnotations::blackBishop] |= insertAtSquare;
						break;
					case 'B':
						result.pieceAtIndex[insertIndex] = boardAnnotations::whiteBishop;
						result.bitboards[boardAnnotations::white] |= insertAtSquare;
						result.bitboards[boardAnnotations::whiteBishop] |= insertAtSquare;
						break;
					case 'r':
						result.pieceAtIndex[insertIndex] = boardAnnotations::blackRook;
						result.bitboards[boardAnnotations::black] |= insertAtSquare;
						result.bitboards[boardAnnotations::blackRook] |= insertAtSquare;
						break;
					case 'R':
						result.pieceAtIndex[insertIndex] = boardAnnotations::whiteRook;
						result.bitboards[boardAnnotations::white] |= insertAtSquare;
						result.bitboards[boardAnnotations::whiteRook] |= insertAtSquare;
						break;
					case 'q':
						result.pieceAtIndex[insertIndex] = boardAnnotations::blackQueen;
						result.bitboards[boardAnnotations::black] |= insertAtSquare;
						result.bitboards[boardAnnotations::blackQueen] |= insertAtSquare;
						break;
					case 'Q':
						result.pieceAtIndex[insertIndex] = boardAnnotations::whiteQueen;
						result.bitboards[boardAnnotations::white] |= insertAtSquare;
						result.bitboards[boardAnnotations::whiteQueen] |= insertAtSquare;
						break;
					case 'k':
						result.pieceAtIndex[insertIndex] = boardAnnotations::blackKing;
						result.bitboards[boardAnnotations::black] |= insertAtSquare;
						result.bitboards[boardAnnotations::blackKing] |= insertAtSquare;
						break;
					case 'K':
						result.pieceAtIndex[insertIndex] = boardAnnotations::whiteKing;
						result.bitboards[boardAnnotations::white] |= insertAtSquare;
						result.bitboards[boardAnnotations::whiteKing] |= insertAtSquare;
						break;
				}
				insertIndex -= 1;
			}
		}

		// Set turn flag
		result.flags |= tokens[1] == "w" ? boardAnnotations::white : boardAnnotations::black;

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
		result.halfMoveClock = static_cast<chess::u8>(std::stoul(tokens[4]));

		// Set fullmove number
		// (Unused and not implimented)

		// Set the zobrist hash of the position]
		result.bitboards[boardAnnotations::occupied] = result.bitboards[boardAnnotations::white] | result.bitboards[boardAnnotations::black];
		result.setZobrist();
		return result;
	} else {
		// Empty position with valid flag not set
		return {
			.bitboards             = {},
			.pieceAtIndex          = {},
			.flags                 = 0,
			.halfMoveClock         = 0,
			.enPassantTargetSquare = 0,
			.zobristHash           = 0
		};
	}
}
