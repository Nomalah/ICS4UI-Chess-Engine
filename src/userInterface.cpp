#include <array>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "../include/chess.h"

std::string chess::position::ascii() {
	std::string result = " +--------+\n";
	result.reserve(132); // Size of final string

	u64 targetSquare = chess::util::iter;	
	for (std::size_t y = 8; y > 0; y--) {
		result += (char)(y + 48);
		result += "|";
		for (std::size_t x = 8; x > 0; x--) {
			if ((this->whiteBishops | this->blackBishops) & targetSquare) {
				result += 'b';
			} else if ((this->whitePawns | this->blackPawns) & targetSquare) {
				result += 'p';
			} else if ((this->whiteKnights | this->blackKnights) & targetSquare) {
				result += 'n';
			} else if ((this->whiteRooks | this->blackRooks) & targetSquare) {
				result += 'r';
			} else if ((this->whiteQueens | this->blackQueens) & targetSquare) {
				result += 'q';
			} else if ((this->whiteKings | this->blackKings) & targetSquare) {
				result += 'k';
			} else {
				result += '*';
			}

			if (this->whitePieces & targetSquare) {
				board[y][x] -= 32;
			}
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
		chess::position result = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0};
		std::stringstream toTokenize(fen);
		std::string output;
		std::array<std::string, 6> tokens;
		auto iter = tokens.begin();
		while (std::getline(toTokenize, output, ' ')) {
			*iter++ = output;
		}

        // Fill board with fen pieces
		u64 insertAtSquare = chess::util::iter;
		for (char piece : tokens[0]) {
			//std::cout << chess::debugging::convert(insertAtSquare) << std::endl;
			if (piece > '0' && piece <= '9') {
				insertAtSquare >>= piece - '0';
			} else if (piece != '/') {
				switch (piece) {
					case 'p':
						result.blackPieces |= insertAtSquare;
						result.blackPawns |= insertAtSquare;
						break;
					case 'P':
						result.whitePieces |= insertAtSquare;
						result.whitePawns |= insertAtSquare;
						break;
					case 'n':
						result.blackPieces |= insertAtSquare;
						result.blackKnights |= insertAtSquare;
						break;
					case 'N':
						result.whitePieces |= insertAtSquare;
						result.whiteKnights |= insertAtSquare;
						break;
					case 'b':
						result.blackPieces |= insertAtSquare;
						result.blackBishops |= insertAtSquare;
						break;
					case 'B':
						result.whitePieces |= insertAtSquare;
						result.whiteBishops |= insertAtSquare;
						break;
					case 'r':
						result.blackPieces |= insertAtSquare;
						result.blackRooks |= insertAtSquare;
						break;
					case 'R':
						result.whitePieces |= insertAtSquare;
						result.whiteRooks |= insertAtSquare;
						break;
					case 'q':
						result.blackPieces |= insertAtSquare;
						result.blackQueens |= insertAtSquare;
						break;
					case 'Q':
						result.whitePieces |= insertAtSquare;
						result.whiteQueens |= insertAtSquare;
						break;
					case 'k':
						result.blackPieces |= insertAtSquare;
						result.blackKings |= insertAtSquare;
						break;
					case 'K':
						result.whitePieces |= insertAtSquare;
						result.whiteKings |= insertAtSquare;
						break;
				}
				insertAtSquare >>= 1;
			}
		}
        
        // Set turn flag
		result.flags |= (tokens[1] == "w") << 6;

        // Set castling flags
		for (char flag : tokens[2]) {
			switch (flag) {
				case 'K':
					result.flags |= 0x08;
                    break;
                case 'Q':
					result.flags |= 0x04;
					break;
                case 'k':
					result.flags |= 0x02;
					break;
				case 'q':
					result.flags |= 0x01;
					break;
			}
		}

        // Set en passant target square
		result.enPassantTargetSquare = chess::util::algebraicToSquare(tokens[3]);

		// Set halfmove clock
		result.halfMoveClock = std::stoul(tokens[4]);

		// Set fullmove number
		// (Unused and not implimented)
		return result;
	} else {
        // Empty position with valid flag not set
		return {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	}
}


chess::game chess::defaultGame() {
	std::vector<position> gameHistory;
	gameHistory.push_back(position::fromFen(
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	return {gameHistory};
}