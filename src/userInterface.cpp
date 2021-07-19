#include <array>
#include <sstream>
#include <string>

#include "../include/chess.h"

[[nodiscard]] std::string chess::position::ascii() const noexcept {
	std::string result { " +--------+\n" };
	result.reserve(132);    // Size of final string

	for (auto targetSquare { 63 }, y { 0 }; y < 8; y++) {
		result += static_cast<char>(y + '0');
		result += '|';
		for (auto x { 0 }; x < 8; x++, targetSquare--) {
			result += chess::util::pieceToChar(this->pieceAtIndex[targetSquare]);
		}
		result += "|\n";
	}

	result += " +--------+\n  abcdefgh";
	return result;
}

[[nodiscard]] chess::position chess::position::fromFen(const std::string& fen) noexcept {
	using namespace chess::constants;
	if (chess::position::validateFen(fen)) {
		// Empty position with a valid flag enabled
		chess::position result {
			.bitboards               = {},
			.pieceAtIndex            = {},
			.flags                   = 0x04,
			.halfMoveClock           = 0,
			.enPassantTargetBitboard = 0,
			.zobristHash             = 0
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
					result.pieceAtIndex[insertIndex] = piece::empty;
				}
			} else if (piece != '/') {
				switch (u64 insertAtSquare = chess::util::bitboardFromIndex(insertIndex); piece) {
					case 'p':
						result.pieceAtIndex[insertIndex] = piece::blackPawn;
						result.bitboards[piece::black] |= insertAtSquare;
						result.bitboards[piece::blackPawn] |= insertAtSquare;
						break;
					case 'P':
						result.pieceAtIndex[insertIndex] = piece::whitePawn;
						result.bitboards[piece::white] |= insertAtSquare;
						result.bitboards[piece::whitePawn] |= insertAtSquare;
						break;
					case 'n':
						result.pieceAtIndex[insertIndex] = piece::blackKnight;
						result.bitboards[piece::black] |= insertAtSquare;
						result.bitboards[piece::blackKnight] |= insertAtSquare;
						break;
					case 'N':
						result.pieceAtIndex[insertIndex] = piece::whiteKnight;
						result.bitboards[piece::white] |= insertAtSquare;
						result.bitboards[piece::whiteKnight] |= insertAtSquare;
						break;
					case 'b':
						result.pieceAtIndex[insertIndex] = piece::blackBishop;
						result.bitboards[piece::black] |= insertAtSquare;
						result.bitboards[piece::blackBishop] |= insertAtSquare;
						break;
					case 'B':
						result.pieceAtIndex[insertIndex] = piece::whiteBishop;
						result.bitboards[piece::white] |= insertAtSquare;
						result.bitboards[piece::whiteBishop] |= insertAtSquare;
						break;
					case 'r':
						result.pieceAtIndex[insertIndex] = piece::blackRook;
						result.bitboards[piece::black] |= insertAtSquare;
						result.bitboards[piece::blackRook] |= insertAtSquare;
						break;
					case 'R':
						result.pieceAtIndex[insertIndex] = piece::whiteRook;
						result.bitboards[piece::white] |= insertAtSquare;
						result.bitboards[piece::whiteRook] |= insertAtSquare;
						break;
					case 'q':
						result.pieceAtIndex[insertIndex] = piece::blackQueen;
						result.bitboards[piece::black] |= insertAtSquare;
						result.bitboards[piece::blackQueen] |= insertAtSquare;
						break;
					case 'Q':
						result.pieceAtIndex[insertIndex] = piece::whiteQueen;
						result.bitboards[piece::white] |= insertAtSquare;
						result.bitboards[piece::whiteQueen] |= insertAtSquare;
						break;
					case 'k':
						result.pieceAtIndex[insertIndex] = piece::blackKing;
						result.bitboards[piece::black] |= insertAtSquare;
						result.bitboards[piece::blackKing] |= insertAtSquare;
						break;
					case 'K':
						result.pieceAtIndex[insertIndex] = piece::whiteKing;
						result.bitboards[piece::white] |= insertAtSquare;
						result.bitboards[piece::whiteKing] |= insertAtSquare;
						break;
				}
				insertIndex -= 1;
			}
		}

		// Set turn flag
		result.flags |= tokens[1] == "w" ? piece::white : piece::black;

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
		if (tokens[3] != "-") {
			result.enPassantTargetBitboard = chess::util::bitboardFromIndex(chess::util::algebraicToSquare(tokens[3]));
		} else {
			result.enPassantTargetBitboard = 0;
		}
		// Set halfmove clock
		result.halfMoveClock = static_cast<chess::u8>(std::stoul(tokens[4]));

		// Set fullmove number
		result.fullMoveClock = static_cast<chess::u16>(std::stoul(tokens[5]));

		// Set the zobrist hash of the position]
		result.bitboards[piece::occupied] = result.bitboards[piece::white] | result.bitboards[piece::black];
		result.bitboards[piece::empty]    = ~result.bitboards[piece::occupied];
		result.setZobrist();
		return result;
	} else {
		// Empty position with valid flag not set
		return {
			.bitboards               = {},
			.pieceAtIndex            = {},
			.flags                   = 0,
			.halfMoveClock           = 0,
			.enPassantTargetBitboard = 0,
			.zobristHash             = 0
		};
	}
}

[[nodiscard]] std::string chess::position::toFen() const noexcept {
	std::string result;
	result.reserve(64 + 7 + 1 + 4 + 2 + 4 + 4);    // 64 max squares + 7 slashes + 1 turn + 4 castle + 2 halfmove + 4 fullmove + 4 spaces

	for (auto targetSquare { 63 }, y { 0 }; y < 8; y++) {
		int emptyCount { 0 };
		for (auto x { 0 }; x < 8; x++, targetSquare--) {
			char piece { chess::util::pieceToChar(this->pieceAtIndex[targetSquare]) };
			if (piece != '*') {
				if (emptyCount != 0)
					result += std::to_string(emptyCount);
				result += piece;
				emptyCount = 0;
			} else {
				emptyCount++;
			}
		}
		if (emptyCount != 0) {
			result += std::to_string(emptyCount);
		}
		result += '/';
	}
	result.back() = ' ';

	result += this->turn() == white ? 'w' : 'b';
	result += ' ';
	if (this->castleWK())
		result += 'K';
	if (this->castleWQ())
		result += 'Q';
	if (this->castleBK())
		result += 'k';
	if (this->castleBQ())
		result += 'q';
	if ((this->flags & 0xF0) == 0)
		result += '-';

	result += ' ';

	if (this->enPassantTargetBitboard == 0) {
		result += '-';
	} else {
		result += chess::util::squareToAlgebraic(chess::util::ctz64(this->enPassantTargetBitboard));
	}

	result += ' ';
	result += std::to_string(this->halfMoveClock);
	result += ' ';
	result += std::to_string(this->fullMoveClock);
	return result;
}