#pragma once

#include <string>
#include <vector>

namespace chess {
typedef unsigned long long u64;
typedef unsigned char u8;
namespace util{
constexpr u64 iter = 1ULL << 63;
u8 algebraicToSquare(std::string algebraic);
}  // namespace util
// More stores the
struct move {
	u64 start;
	u64 end;
	u8 flags;
	bool isValid();
};

struct position {
	// Most to least information dense
	u64 whitePieces;
	u64 blackPieces;
	u64 whitePawns;
	u64 blackPawns;
	u64 whiteKnights;
	u64 blackKnights;
	u64 whiteBishops;
	u64 blackBishops;
	u64 whiteRooks;
	u64 blackRooks;
	u64 whiteQueens;
	u64 blackQueens;
	u64 whiteKings;
	u64 blackKings;
	u8 flags;
	u8 halfMoveClock;
	u64 enPassantTargetSquare;

	std::vector<move> moves();
	std::string ascii();
	inline bool turn();	  // 0 - 1 (1 bit)
	inline u8 castle();	  // 0 - 15 (4 bits)
	inline bool valid();  // 0 - 1 (1 bit)
	static position fromFen(std::string fen);
	static std::string toFen();
	static std::string toFen(std::size_t turnCount);
	static bool validateFen(std::string fen);

	static bool pieceEqual(position a, position b);
};

struct game {
	std::vector<position> gameHistory;

	std::vector<move> moves();
	bool finished();
	inline position currentPosition() { return gameHistory.back(); };
	void move(move desiredMove);
	bool undo();
	bool validMove(chess::move desiredMove);
	u8 result();
};

game defaultGame();
move strToMove(std::string move);

namespace debugging {
std::string convert(u64 bitBoard);
}




}  // namespace chess