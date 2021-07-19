#include <array>
#include <sstream>
#include <iostream>
#include <string>

#include "../include/chess.h"

[[nodiscard]] std::string chess::game::ascii() const noexcept {
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

chess::game::game(const std::string& fen) noexcept :
	bitboards {} {
	using namespace chess::constants;
	if (chess::game::validateFen(fen)) {
		// Empty position with a valid flag enabled

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
					this->pieceAtIndex[insertIndex] = boardAnnotations::empty;
				}
			} else if (piece != '/') {
				switch (u64 insertAtSquare = chess::util::bitboardFromIndex(insertIndex); piece) {
					case 'p':
						this->pieceAtIndex[insertIndex] = boardAnnotations::blackPawn;
						this->bitboards[boardAnnotations::black] |= insertAtSquare;
						this->bitboards[boardAnnotations::blackPawn] |= insertAtSquare;
						break;
					case 'P':
						this->pieceAtIndex[insertIndex] = boardAnnotations::whitePawn;
						this->bitboards[boardAnnotations::white] |= insertAtSquare;
						this->bitboards[boardAnnotations::whitePawn] |= insertAtSquare;
						break;
					case 'n':
						this->pieceAtIndex[insertIndex] = boardAnnotations::blackKnight;
						this->bitboards[boardAnnotations::black] |= insertAtSquare;
						this->bitboards[boardAnnotations::blackKnight] |= insertAtSquare;
						break;
					case 'N':
						this->pieceAtIndex[insertIndex] = boardAnnotations::whiteKnight;
						this->bitboards[boardAnnotations::white] |= insertAtSquare;
						this->bitboards[boardAnnotations::whiteKnight] |= insertAtSquare;
						break;
					case 'b':
						this->pieceAtIndex[insertIndex] = boardAnnotations::blackBishop;
						this->bitboards[boardAnnotations::black] |= insertAtSquare;
						this->bitboards[boardAnnotations::blackBishop] |= insertAtSquare;
						break;
					case 'B':
						this->pieceAtIndex[insertIndex] = boardAnnotations::whiteBishop;
						this->bitboards[boardAnnotations::white] |= insertAtSquare;
						this->bitboards[boardAnnotations::whiteBishop] |= insertAtSquare;
						break;
					case 'r':
						this->pieceAtIndex[insertIndex] = boardAnnotations::blackRook;
						this->bitboards[boardAnnotations::black] |= insertAtSquare;
						this->bitboards[boardAnnotations::blackRook] |= insertAtSquare;
						break;
					case 'R':
						this->pieceAtIndex[insertIndex] = boardAnnotations::whiteRook;
						this->bitboards[boardAnnotations::white] |= insertAtSquare;
						this->bitboards[boardAnnotations::whiteRook] |= insertAtSquare;
						break;
					case 'q':
						this->pieceAtIndex[insertIndex] = boardAnnotations::blackQueen;
						this->bitboards[boardAnnotations::black] |= insertAtSquare;
						this->bitboards[boardAnnotations::blackQueen] |= insertAtSquare;
						break;
					case 'Q':
						this->pieceAtIndex[insertIndex] = boardAnnotations::whiteQueen;
						this->bitboards[boardAnnotations::white] |= insertAtSquare;
						this->bitboards[boardAnnotations::whiteQueen] |= insertAtSquare;
						break;
					case 'k':
						this->pieceAtIndex[insertIndex] = boardAnnotations::blackKing;
						this->bitboards[boardAnnotations::black] |= insertAtSquare;
						this->bitboards[boardAnnotations::blackKing] |= insertAtSquare;
						break;
					case 'K':
						this->pieceAtIndex[insertIndex] = boardAnnotations::whiteKing;
						this->bitboards[boardAnnotations::white] |= insertAtSquare;
						this->bitboards[boardAnnotations::whiteKing] |= insertAtSquare;
						break;
				}
				insertIndex -= 1;
			}
		}

		// Set the unified bitboards
		this->bitboards[boardAnnotations::occupied] = this->bitboards[boardAnnotations::white] | this->bitboards[boardAnnotations::black];
		this->bitboards[boardAnnotations::empty]    = ~this->bitboards[boardAnnotations::occupied];

		// Set turn flag
		this->turn = tokens[1] == "w" ? boardAnnotations::white : boardAnnotations::black;

		this->flags.push_back({});

		// Set castling flags
		for (char flag : tokens[2]) {
			switch (flag) {
				case 'K':
					this->flags.back().castleWK = true;
					break;
				case 'Q':
					this->flags.back().castleWQ = true;
					break;
				case 'k':
					this->flags.back().castleBK = true;
					break;
				case 'q':
					this->flags.back().castleBQ = true;
					break;
			}
		}

		// Set en passant target square
		if (tokens[3] != "-") {
			this->flags.back().enPassantTargetBitboard = 1ULL << chess::util::algebraicToSquare(tokens[3]);
		} else {
			this->flags.back().enPassantTargetBitboard = 0x0ULL;
		}
		// Set halfmove clock
		this->flags.back().halfMoveClock = static_cast<chess::u8>(std::stoul(tokens[4]));

		// Set fullmove number
		this->fullMoveClock = static_cast<chess::u16>(std::stoul(tokens[5]));

		// Set the zobrist hash of the position
		this->setZobrist();
	}
}

[[nodiscard]] std::string chess::game::toFen() const noexcept {
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

	result += this->turn == white ? 'w' : 'b';
	result += ' ';
	if (this->flags.back().castleWK)
		result += 'K';
	if (this->flags.back().castleWQ)
		result += 'Q';
	if (this->flags.back().castleBK)
		result += 'k';
	if (this->flags.back().castleBQ)
		result += 'q';
	if (result.back() == ' ')
		result += '-';

	result += ' ';

	if (this->flags.back().enPassantTargetBitboard == 0x0ULL) {
		result += '-';
	} else {
		result += chess::util::squareToAlgebraic(chess::util::ctz64(this->flags.back().enPassantTargetBitboard));
	}

	result += ' ';
	result += std::to_string(this->flags.back().halfMoveClock);
	result += ' ';
	result += std::to_string(this->fullMoveClock);
	return result;
}