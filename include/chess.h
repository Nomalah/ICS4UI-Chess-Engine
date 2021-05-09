#pragma once

#include <array>
#include <string>
#include <vector>

namespace chess {
typedef unsigned long long u64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
namespace util {
constexpr u64 iter = 1ULL << 63;
u8 ctz64(u64 bitboard);
u8 clz64(u64 bitboard);
inline constexpr u8 algebraicToSquare(const std::string& algebraic) {
	if (algebraic.length() == 2) {
		u8 total = 0;
		if (algebraic[0] >= 'a' && algebraic[0] <= 'h') {
			total += 7 - algebraic[0] + 'a';
			if (algebraic[1] >= '1' && algebraic[1] <= '8') {
				total += (algebraic[1] - '1') * 8;
				return total;
			}
		}
	}
	return 0x0ULL;
};
std::string squareToAlgebraic(u8 square);
inline constexpr u64 bitboardFromIndex(u8 index) { return 1ULL << index; };
namespace constants {
constexpr u64 full = 0xFFFFFFFFFFFFFFFFULL;
enum attackRayDirection {
	north = 0,
	northEast = 1,
	east = 2,
	southEast = 3,
	south = 4,
	southWest = 5,
	west = 6,
	northWest = 7
};
enum boardModifiers : chess::u8 {
	black = 0x0,
	blackPawn = 0x1,
	blackKnight = 0x2,
	blackBishop = 0x3,
	blackRook = 0x4,
	blackQueen = 0x5,
	blackKing = 0x6,
	blackNull = 0x7,
	white = 0x8,
	whitePawn = 0x9,
	whiteKnight = 0xA,
	whiteBishop = 0xB,
	whiteRook = 0xC,
	whiteQueen = 0xD,
	whiteKing = 0xE,
	whiteNull = 0xF
};
char pieceToChar(chess::util::constants::boardModifiers piece);
constexpr std::array<std::array<chess::u64, 64>, 8> generateAttackRays() {
	std::array<std::array<chess::u64, 64>, 8> resultRays{};
	constexpr chess::u64 horizontal = 0xFFULL;
	constexpr chess::u64 vertical = 0x0101010101010101ULL;
	constexpr chess::u64 diagonal = 0x0102040810204080ULL;
	constexpr chess::u64 antiDiagonal = 0x8040201008040201ULL;
	for (std::size_t d = attackRayDirection::north; d <= attackRayDirection::northWest; d++) {
		for (std::size_t y = 0; y < 8; y++) {
			for (std::size_t x = 0; x < 8; x++) {
				switch (d) {
					case attackRayDirection::north:	 // North
						resultRays[d][y * 8 + x] = ((vertical << x) ^ (1ULL << (y * 8 + x))) & (full << (y * 8));
						break;
					case attackRayDirection::south:	 // South
						resultRays[d][y * 8 + x] = ((vertical << x) ^ (1ULL << (y * 8 + x))) & ~(full << (y * 8));
						break;
					case attackRayDirection::east:	// East
						resultRays[d][y * 8 + x] = ((horizontal << y * 8) ^ (1ULL << (y * 8 + x))) & ~(full << (y * 8 + x));
						break;
					case attackRayDirection::west:	// West
						resultRays[d][y * 8 + x] = ((horizontal << y * 8) ^ (1ULL << (y * 8 + x))) & (full << (y * 8 + x));
						break;
					case attackRayDirection::northEast:	 // NorthEast
						resultRays[d][y * 8 + x] = ((x + y > 6 ? diagonal << (8 * (x + y - 7)) : diagonal >> (8 * (7 - x - y))) ^ (1ULL << (y * 8 + x))) & (full << (y * 8));
						break;
					case attackRayDirection::southWest:	 // SouthWest
						resultRays[d][y * 8 + x] = ((x + y > 6 ? diagonal << (8 * (x + y - 7)) : diagonal >> (8 * (7 - x - y))) ^ (1ULL << (y * 8 + x))) & ~(full << (y * 8));
						break;
					case attackRayDirection::southEast:	 // SouthEast
						resultRays[d][y * 8 + x] = ((y > x ? antiDiagonal << (8 * (y - x)) : antiDiagonal >> (8 * (x - y))) ^ (1ULL << (y * 8 + x))) & ~(full << (y * 8 + x));
						break;
					case attackRayDirection::northWest:	 // NorthWest
						resultRays[d][y * 8 + x] = ((y > x ? antiDiagonal << (8 * (y - x)) : antiDiagonal >> (8 * (x - y))) ^ (1ULL << (y * 8 + x))) & (full << (y * 8 + x));
						break;
				}
			}
		}
	}
	return resultRays;
}
constexpr std::array<std::array<chess::u64, 64>, 8> attackRays = generateAttackRays();

constexpr std::array<chess::u64, 64> generateKnightJumps() {
	constexpr std::array<chess::u64, 8> fileMask = {
		0xC0C0C0C0C0C0C0C0ULL, 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0101010101010101ULL, 0x0303030303030303ULL};
	std::array<chess::u64, 64> resultJumps{};
	chess::u64 defaultJump = 0x0000142200221400ULL;
	for (std::size_t i = 0; i < 27; i++) {
		resultJumps[i] = (defaultJump >> (27ULL - i)) & ~fileMask[i % 8];
	}

	for (std::size_t i = 27; i < 64; i++) {
		resultJumps[i] = (defaultJump << (i - 27ULL)) & ~fileMask[i % 8];
	}

	return resultJumps;
}
constexpr std::array<chess::u64, 64> knightJumps = generateKnightJumps();

constexpr std::array<chess::u64, 64> generateKingAttacks() {
	constexpr std::array<chess::u64, 8> fileMask = {
		0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL};
	std::array<chess::u64, 64> resultJumps{};
	chess::u64 defaultAttack = 0x0000001C141C0000ULL;
	for (std::size_t i = 0; i < 27; i++) {
		resultJumps[i] = (defaultAttack >> (27ULL - i)) & ~fileMask[i % 8];
	}

	for (std::size_t i = 27; i < 64; i++) {
		resultJumps[i] = (defaultAttack << (i - 27ULL)) & ~fileMask[i % 8];
	}

	return resultJumps;
}
constexpr std::array<chess::u64, 64> kingAttacks = generateKingAttacks();

constexpr std::array<std::array<chess::u64, 64>, 2> generatePawnAttacks() {
	constexpr std::array<chess::u64, 8> fileMask = {
		0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL};
	std::array<std::array<chess::u64, 64>, 2> resultAttacks{};
	constexpr chess::u64 defaultAttackWhite = 0x0280ULL;
	for (std::size_t i = 0; i < 64; i++) {
		resultAttacks[1][i] = (defaultAttackWhite << i) & ~fileMask[i % 8] & ~0xFFFFULL;
	}

    constexpr chess::u64 defaultAttackBlack = 0x0140000000000000ULL;
	for (std::size_t i = 0; i < 64; i++) {
		resultAttacks[0][i] = (defaultAttackBlack >> (63-i)) & ~fileMask[i % 8] & ~0xFFFF000000000000ULL;
	}

	return resultAttacks;
}
constexpr std::array<std::array<chess::u64, 64>, 2> pawnAttacks = generatePawnAttacks();
}  // namespace constants
chess::util::constants::boardModifiers colorOf(chess::util::constants::boardModifiers piece);
chess::util::constants::boardModifiers nullOf(chess::util::constants::boardModifiers piece);
inline chess::u16 isPiece(chess::util::constants::boardModifiers piece) { return (piece & 0x7) == 0x7 ? 0x0000 : 0x1000; };
}  // namespace util

// More stores the
struct move {
	u8 origin;
	u8 destination;
	u16 flags;
	inline chess::util::constants::boardModifiers capturedPiece() const noexcept { return static_cast<chess::util::constants::boardModifiers>(flags & 0x000F); };
	inline chess::util::constants::boardModifiers movePiece() const noexcept { return static_cast<chess::util::constants::boardModifiers>((flags & 0x00F0) >> 4); };
	inline chess::util::constants::boardModifiers promotionPiece() const noexcept { return static_cast<chess::util::constants::boardModifiers>((flags & 0x0F00) >> 8); };
	inline u64 originSquare() const noexcept { return 1ULL << origin; };
	inline u64 destinationSquare() const noexcept { return 1ULL << destination; };
	inline u16 moveFlags() const noexcept { return flags & 0xFF00; };
    static inline std::string flagToString(u16 flag){
		return "";
	}
	std::string toString() const noexcept {
		return chess::util::squareToAlgebraic(this->origin) + chess::util::squareToAlgebraic(this->destination)/* + flagToString(this->moveFlags())*/;
	};
};

struct position {
	// Most to least information dense
	std::array<chess::u64, 16> bitboards;
	
    std::array<chess::u8, 32> indexToPiece;
	inline chess::util::constants::boardModifiers pieceAtIndex(u8 index) const noexcept {
		return static_cast<chess::util::constants::boardModifiers>(index % 2 ? indexToPiece[index / 2] & 0x0F : (indexToPiece[index / 2] & 0xF0) >> 4);
	}
	inline void insertPieceAtIndex(u8 index, chess::util::constants::boardModifiers piece) noexcept {
		indexToPiece[index / 2] = (index % 2 ? (indexToPiece[index / 2] & 0xF0) | piece : (indexToPiece[index / 2] & 0x0F) | (piece << 4));
	}

	u8 flags;
	u8 halfMoveClock;
	u64 enPassantTargetSquare;

	std::vector<move> moves();
	position move(move desiredMove);
	inline u64 bishopAttacks(u8 bishopLocation) const noexcept;
	inline u64 rookAttacks(u8 rookLocation) const noexcept;
	inline u64 knightAttacks(u8 knightLocation) const noexcept;
	inline u64 kingAttacks(u8 kingLocation) const noexcept;
	inline u64 pawnAttacks(u8 squareFrom) const noexcept;
	u64 positiveRayAttacks(u8 location, util::constants::attackRayDirection direction) const noexcept;
	u64 negativeRayAttacks(u8 location, util::constants::attackRayDirection direction) const noexcept;
	std::string ascii();
	inline u8 turn() const noexcept { return flags & 0x08; };	  // 0 - 1 (1 bit)
	inline chess::u64 occupied() const noexcept { return bitboards[chess::util::constants::boardModifiers::white] | bitboards[chess::util::constants::boardModifiers::black]; };
	inline chess::u64 empty() const noexcept { return ~occupied(); };
	inline u8 castle() const noexcept { return flags & 0xF0; };	 // 0 - 15 (4 bits)
	inline bool valid() const noexcept { return flags & 0x04; };  // 0 - 1 (1 bit)
	inline u64 castleWK() const noexcept { return flags & 0x80 ? 0x02ULL : 0x0ULL; };
	inline u64 castleWQ() const noexcept { return flags & 0x40 ? 0x20ULL : 0x0ULL; };
	inline u64 castleBK() const noexcept { return flags & 0x20 ? 0x0200000000000000ULL : 0x0ULL; };
	inline u64 castleBQ() const noexcept { return flags & 0x10 ? 0x2000000000000000ULL : 0x0ULL; };
	static position fromFen(std::string fen);
	static std::string toFen();
	static std::string toFen(std::size_t turnCount);
	static bool validateFen(std::string fen);
	inline static void nextTurn(position& toModify) noexcept { toModify.flags ^= 0x08; }; // Switches the turn on a position
	static bool pieceEqual(position a, position b);
};

struct game {
	std::vector<position> gameHistory;

	std::vector<move> moves();
	bool finished();
	inline position currentPosition() { return gameHistory.back(); };
	void move(move desiredMove);
	bool undo();
	inline u8 result() { return 0; };  // unused
};

game defaultGame();
move strToMove(std::string move);

namespace debugging {
std::string convert(u64 bitBoard);
}

}  // namespace chess