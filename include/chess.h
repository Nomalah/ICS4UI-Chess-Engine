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
inline chess::u8 ctz64(const u64 bitboard) noexcept {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_ctzll(bitboard);
#else
	static_assert(-1);
#endif
}

inline chess::u8 clz64(const u64 bitboard) noexcept {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_clzll(bitboard);
#else
	static_assert(-1);
#endif
}

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
enum boardAnnotations : chess::u8 {
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

enum squareAnnotations : chess::u8 {
	h1, g1, f1, e1, d1, c1, b1, a1,
    h2, g2, f2, e2, d2, c2, b2, a2,
    h3, g3, f3, e3, d3, c3, b3, a3,
    h4, g4, f4, e4, d4, c4, b4, a4,
    h5, g5, f5, e5, d5, c5, b5, a5,
    h6, g6, f6, e6, d6, c6, b6, a6,
    h7, g7, f7, e7, d7, c7, b7, a7,
    h8, g8, f8, e8, d8, c8, b8, a8
};

char pieceToChar(chess::util::constants::boardAnnotations piece);
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

namespace{
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

constexpr std::array<std::array<chess::u64, 64>, 2> generatePawnAttacks() {
	constexpr std::array<chess::u64, 8> fileMask = {
		0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL};
	std::array<std::array<chess::u64, 64>, 2> resultAttacks{};
	constexpr chess::u64 defaultAttackWhite = 0x0280ULL;
	for (std::size_t i = 0; i < 64; i++) {
		resultAttacks[1][i] = (defaultAttackWhite << i) & ~fileMask[i % 8] & ~0xFFULL;
	}

    constexpr chess::u64 defaultAttackBlack = 0x0140000000000000ULL;
	for (std::size_t i = 0; i < 64; i++) {
		resultAttacks[0][i] = (defaultAttackBlack >> (63-i)) & ~fileMask[i % 8] & ~0xFF00000000000000ULL;
	}

	return resultAttacks;
}
}
constexpr std::array<std::array<chess::u64, 64>, 8> attackRays = generateAttackRays();
constexpr std::array<chess::u64, 64> kingAttacks = generateKingAttacks();
constexpr std::array<chess::u64, 64> knightJumps = generateKnightJumps();
constexpr std::array<std::array<chess::u64, 64>, 2> pawnAttacks = generatePawnAttacks();
}  // namespace constants

inline constexpr chess::util::constants::boardAnnotations colorOf(const chess::util::constants::boardAnnotations piece) noexcept {
	return static_cast<chess::util::constants::boardAnnotations>(piece & 0x08);
}

inline constexpr chess::util::constants::boardAnnotations nullOf(const chess::util::constants::boardAnnotations piece) noexcept {
	return static_cast<chess::util::constants::boardAnnotations>(piece | 0x07);
}
inline constexpr chess::u16 isPiece(chess::util::constants::boardAnnotations piece) { return (piece & 0x7) == 0x7 ? 0x0000 : 0x1000; };
}  // namespace util

// More stores the
struct moveData {
	u8 origin;
	u8 destination;
	u16 flags;
	inline chess::util::constants::boardAnnotations capturedPiece() const noexcept { return static_cast<chess::util::constants::boardAnnotations>(flags & 0x000F); };
	inline chess::util::constants::boardAnnotations movePiece() const noexcept { return static_cast<chess::util::constants::boardAnnotations>((flags & 0x00F0) >> 4); };
	inline chess::util::constants::boardAnnotations promotionPiece() const noexcept { return static_cast<chess::util::constants::boardAnnotations>((flags & 0x0F00) >> 8); };
	inline u64 originSquare() const noexcept { return 1ULL << origin; };
	inline u64 destinationSquare() const noexcept { return 1ULL << destination; };
	inline u16 moveFlags() const noexcept { return flags & 0xFF00; };
    static inline std::string flagToString(u16 flag){
        switch(flag & 0x0F00){
            case 0x0200:
				return "n";
			case 0x0300:
				return "b";
			case 0x0400:
                return "r";
			case 0x0500:
				return "q";
			case 0x0A00:
				return "N";
			case 0x0B00:
				return "B";
			case 0x0C00:
				return "R";
			case 0x0D00:
				return "Q";
			default:
				return "";
		}
	}
	std::string toString() const noexcept {
		return chess::util::squareToAlgebraic(this->origin) + chess::util::squareToAlgebraic(this->destination) + flagToString(this->flags);
	};
};

class moveList{
    private:
        std::array<moveData, 254> moves;
        moveData* insertLocation;
    public:
        moveList(): moves(), insertLocation(moves.data()){}
        moveList(const moveList& other) : moves(other.moves), insertLocation((this->moves.data() - other.moves.data()) + other.insertLocation){}
        inline void append(const moveData moveToInsert) noexcept { *(insertLocation++) = moveToInsert;}
        inline moveData* begin() noexcept { return moves.data(); }
        inline moveData* end() noexcept { return insertLocation; }
        inline u64 size() noexcept {return end() - begin();}
};

struct position {
	// Most to least information dense
	std::array<chess::u64, 16> bitboards;
	
    std::array<chess::util::constants::boardAnnotations, 64> pieceAtIndex;

	u8 flags;
	u8 halfMoveClock;
	u64 enPassantTargetSquare;

	moveList moves();
    chess::u64 attacks(chess::util::constants::boardAnnotations turn);
	bool squareAttacked(chess::util::constants::squareAnnotations square);
	position move(moveData desiredMove);
	inline u64 bishopAttacks(u8 bishopLocation, chess::util::constants::boardAnnotations turn) const noexcept;
	inline u64 rookAttacks(u8 rookLocation, chess::util::constants::boardAnnotations turn) const noexcept;
	inline u64 knightAttacks(u8 knightLocation, chess::util::constants::boardAnnotations turn) const noexcept;
	inline u64 kingAttacks(u8 kingLocation, chess::util::constants::boardAnnotations turn) const noexcept;
	inline std::array<u64, 2> pawnAttacks(u8 squareFrom) const noexcept;
    template <util::constants::attackRayDirection direction>
    chess::u64 positiveRayAttacks(u8 squareFrom, chess::util::constants::boardAnnotations turn) const noexcept {
        chess::u64 attacks = chess::util::constants::attackRays[direction][squareFrom];
        chess::u64 blocker = (attacks & this->occupied()) | 0x8000000000000000;
        squareFrom = chess::util::ctz64(blocker);
        attacks ^= chess::util::constants::attackRays[direction][squareFrom];
        return attacks;
    }

    template <util::constants::attackRayDirection direction>
    chess::u64 negativeRayAttacks(u8 squareFrom, chess::util::constants::boardAnnotations turn) const noexcept {
        chess::u64 attacks = chess::util::constants::attackRays[direction][squareFrom];
        chess::u64 blocker = (attacks & this->occupied()) | 0x1;
        squareFrom = 63U - chess::util::clz64(blocker);
        attacks ^= chess::util::constants::attackRays[direction][squareFrom];
        return attacks;
    }
	std::string ascii();
	inline chess::util::constants::boardAnnotations turn() const noexcept { return static_cast<chess::util::constants::boardAnnotations>(flags & 0x08); };  // 0 - 1 (1 bit)
	inline chess::u64 occupied() const noexcept { return bitboards[chess::util::constants::boardAnnotations::white] | bitboards[chess::util::constants::boardAnnotations::black]; };
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

	moveList moves();
	bool finished();
	inline position currentPosition() { return gameHistory.back(); };
	void move(moveData desiredMove);
	bool undo();
	inline u8 result() { return 0; };  // unused
};

game defaultGame();
moveData strToMove(std::string move);

namespace debugging {
std::string bitboardToString(u64 bitBoard);
}

}  // namespace chess