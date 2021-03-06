#pragma once

#include <array>
#include <string>
#include <vector>
#include <algorithm>

namespace chess {
	// Types used in the chess engine
	typedef unsigned long long u64;
	typedef unsigned char u8;
	typedef unsigned short u16;
	typedef unsigned int u32;
	enum attackRayDirection : chess::u8
	{
		north     = 0,
		northEast = 1,
		east      = 2,
		southEast = 3,
		south     = 4,
		southWest = 5,
		west      = 6,
		northWest = 7
	};

	enum boardAnnotations : chess::u8
	{
		noColor     = 0x0,
		pawn        = 0x1,
		knight      = 0x2,
		bishop      = 0x3,
		rook        = 0x4,
		queen       = 0x5,
		king        = 0x6,
		null        = 0x7,
		black       = 0x0,
		blackPawn   = 0x1,
		blackKnight = 0x2,
		blackBishop = 0x3,
		blackRook   = 0x4,
		blackQueen  = 0x5,
		blackKing   = 0x6,
		blackNull   = 0x7,
		white       = 0x8,
		whitePawn   = 0x9,
		whiteKnight = 0xA,
		whiteBishop = 0xB,
		whiteRook   = 0xC,
		whiteQueen  = 0xD,
		whiteKing   = 0xE,
		whiteNull   = 0xF
	};

	enum squareAnnotations : chess::u8
	{
		h1,
		g1,
		f1,
		e1,
		d1,
		c1,
		b1,
		a1,
		h2,
		g2,
		f2,
		e2,
		d2,
		c2,
		b2,
		a2,
		h3,
		g3,
		f3,
		e3,
		d3,
		c3,
		b3,
		a3,
		h4,
		g4,
		f4,
		e4,
		d4,
		c4,
		b4,
		a4,
		h5,
		g5,
		f5,
		e5,
		d5,
		c5,
		b5,
		a5,
		h6,
		g6,
		f6,
		e6,
		d6,
		c6,
		b6,
		a6,
		h7,
		g7,
		f7,
		e7,
		d7,
		c7,
		b7,
		a7,
		h8,
		g8,
		f8,
		e8,
		d8,
		c8,
		b8,
		a8
	};

	namespace util {
		[[nodiscard]] inline constexpr u8 algebraicToSquare(const std::string& algebraic) noexcept {
			if (algebraic.length() == 2) {
				u8 total { 0 };
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
		[[nodiscard]] std::string squareToAlgebraic(const chess::squareAnnotations square);
		[[nodiscard]] inline constexpr char pieceToChar(const chess::boardAnnotations piece, const bool uciFormat) {
			if (uciFormat) {
				constexpr char conversionList[] = { '\0', 'p', 'n', 'b', 'r', 'q', 'k', '\0', '\0', 'p', 'n', 'b', 'r', 'q', 'k', '\0' };
				return conversionList[piece];
			} else {
				constexpr char conversionList[] = { '*', 'p', 'n', 'b', 'r', 'q', 'k', '*', '*', 'P', 'N', 'B', 'R', 'Q', 'K', '*' };
				return conversionList[piece];
			}
		}
		[[nodiscard]] inline constexpr u64 bitboardFromIndex(const u8 index) { return 1ULL << index; };
		[[nodiscard]] inline constexpr chess::boardAnnotations oppositeTurn(const chess::boardAnnotations piece) noexcept {
			return static_cast<chess::boardAnnotations>(piece ^ 0x8);
		}

		namespace constants {

			constexpr u64 bitboardFull { 0xFFFFFFFFFFFFFFFFULL };
			constexpr u64 bitboardIter { 1ULL << 63 };
			inline constexpr squareAnnotations operator++(squareAnnotations& square) noexcept {
				return square = static_cast<squareAnnotations>(square + 1);
			}

			namespace {
				constexpr std::array<std::array<chess::u64, 64>, 8> generateAttackRays() {
					std::array<std::array<chess::u64, 64>, 8> resultRays {};
					constexpr chess::u64 horizontal { 0x00000000000000FFULL };
					constexpr chess::u64 vertical { 0x0101010101010101ULL };
					constexpr chess::u64 diagonal { 0x0102040810204080ULL };
					constexpr chess::u64 antiDiagonal { 0x8040201008040201ULL };
					for (u8 d { chess::attackRayDirection::north }; d <= chess::attackRayDirection::northWest; d++) {
						for (std::size_t y { 0 }; y < 8; y++) {
							for (std::size_t x { 0 }; x < 8; x++) {
								switch (d) {
									case chess::attackRayDirection::north:    // North
										resultRays[d][y * 8 + x] = ((vertical << x) ^ (1ULL << (y * 8 + x))) & (chess::util::constants::bitboardFull << (y * 8));
										break;
									case chess::attackRayDirection::south:    // South
										resultRays[d][y * 8 + x] = ((vertical << x) ^ (1ULL << (y * 8 + x))) & ~(chess::util::constants::bitboardFull << (y * 8));
										break;
									case chess::attackRayDirection::east:    // East
										resultRays[d][y * 8 + x] = ((horizontal << y * 8) ^ (1ULL << (y * 8 + x))) & ~(chess::util::constants::bitboardFull << (y * 8 + x));
										break;
									case chess::attackRayDirection::west:    // West
										resultRays[d][y * 8 + x] = ((horizontal << y * 8) ^ (1ULL << (y * 8 + x))) & (chess::util::constants::bitboardFull << (y * 8 + x));
										break;
									case chess::attackRayDirection::northEast:    // NorthEast
										resultRays[d][y * 8 + x] = ((x + y > 6 ? diagonal << (8 * (x + y - 7)) : diagonal >> (8 * (7 - x - y))) ^ (1ULL << (y * 8 + x))) & (chess::util::constants::bitboardFull << (y * 8));
										break;
									case chess::attackRayDirection::southWest:    // SouthWest
										resultRays[d][y * 8 + x] = ((x + y > 6 ? diagonal << (8 * (x + y - 7)) : diagonal >> (8 * (7 - x - y))) ^ (1ULL << (y * 8 + x))) & ~(chess::util::constants::bitboardFull << (y * 8));
										break;
									case chess::attackRayDirection::southEast:    // SouthEast
										resultRays[d][y * 8 + x] = ((y > x ? antiDiagonal << (8 * (y - x)) : antiDiagonal >> (8 * (x - y))) ^ (1ULL << (y * 8 + x))) & ~(chess::util::constants::bitboardFull << (y * 8 + x));
										break;
									case chess::attackRayDirection::northWest:    // NorthWest
										resultRays[d][y * 8 + x] = ((y > x ? antiDiagonal << (8 * (y - x)) : antiDiagonal >> (8 * (x - y))) ^ (1ULL << (y * 8 + x))) & (chess::util::constants::bitboardFull << (y * 8 + x));
										break;
								}
							}
						}
					}
					return resultRays;
				}

				constexpr std::array<chess::u64, 64> generateKnightJumps() {
					constexpr std::array<chess::u64, 8> fileMask { { 0xC0C0C0C0C0C0C0C0ULL, 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0101010101010101ULL, 0x0303030303030303ULL } };
					std::array<chess::u64, 64> resultJumps {};
					chess::u64 defaultJump { 0x0000142200221400ULL };    // Attacks from e4
					for (chess::squareAnnotations i { h1 }; i <= f4; ++i) {
						resultJumps[i] = (defaultJump >> (27ULL - i)) & ~fileMask[i % 8];
					}

					for (chess::squareAnnotations i { e4 }; i <= a8; ++i) {
						resultJumps[i] = (defaultJump << (i - 27ULL)) & ~fileMask[i % 8];
					}

					return resultJumps;
				}

				constexpr std::array<chess::u64, 64> generateKingAttacks() {
					constexpr std::array<chess::u64, 8> fileMask { { 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL } };
					std::array<chess::u64, 64> resultJumps {};
					chess::u64 defaultAttack { 0x0000001C141C0000ULL };    // Attacks from e4
					for (chess::squareAnnotations i { h1 }; i <= f4; ++i) {
						resultJumps[i] = (defaultAttack >> (e4 - i)) & ~fileMask[i % 8];
					}

					for (chess::squareAnnotations i { e4 }; i <= a8; ++i) {
						resultJumps[i] = (defaultAttack << (i - e4)) & ~fileMask[i % 8];
					}

					return resultJumps;
				}

				constexpr std::array<std::array<chess::u64, 64>, 2> generatePawnAttacks() {
					constexpr std::array<chess::u64, 8> fileMask { { 0x8080808080808080ULL, 0x0ULL, 0x0ULL, 0x0ULL, 0x0Ull, 0x0ULL, 0x0ULL, 0x0101010101010101ULL } };
					std::array<std::array<chess::u64, 64>, 2> resultAttacks {};
					constexpr chess::u64 defaultAttackWhite { 0x0000000000000280ULL };
					for (chess::squareAnnotations i { h1 }; i <= a8; ++i) {
						resultAttacks[1][i] = (defaultAttackWhite << i) & ~fileMask[i % 8] & ~0xFFULL;
					}

					constexpr chess::u64 defaultAttackBlack { 0x0140000000000000ULL };
					for (chess::squareAnnotations i { h1 }; i <= a8; ++i) {
						resultAttacks[0][i] = (defaultAttackBlack >> (a8 - i)) & ~fileMask[i % 8] & ~0xFF00000000000000ULL;
					}

					return resultAttacks;
				}

				constexpr std::array<std::array<chess::u64, 16>, 64> generateZobristBitStrings() {
					constexpr auto time_from_string { [](const char* str, int offset) {
						return static_cast<std::uint32_t>(str[offset] - '0') * 10 + static_cast<std::uint32_t>(str[offset + 1] - '0');
					} };

					std::array<std::array<chess::u64, 16>, 64> result {};
					// Seed the random number generation
					auto previous = [&time_from_string]() -> chess::u64 {
						const char* t { __TIME__ };
						return time_from_string(t, 0) * 60 * 60 + time_from_string(t, 3) * 60 + time_from_string(t, 6);
					}();
					for (auto& square : result) {
						for (auto& piece : square) {
							piece    = previous;
							previous = ((137 * previous + 457) % 922372036854775808ULL);
							piece ^= previous << 16;
							previous = ((137 * previous + 457) % 922372036854775808ULL);
							piece ^= previous << 32;
							previous = ((137 * previous + 457) % 922372036854775808ULL);
						}
					}
					return result;
				}
			}    // namespace

			constexpr std::array<std::array<chess::u64, 64>, 8> attackRays { generateAttackRays() };
			constexpr std::array<chess::u64, 64> kingAttacks { generateKingAttacks() };
			constexpr std::array<chess::u64, 64> knightJumps { generateKnightJumps() };
			constexpr std::array<std::array<chess::u64, 64>, 2> pawnAttacks { generatePawnAttacks() };

			constexpr std::array<std::array<chess::u64, 16>, 64> zobristBitStrings { generateZobristBitStrings() };
		}    // namespace constants

		// Intrinsic wrapper functions
		// Count trailing zeros (of a number's binary representation)
		[[nodiscard]] inline chess::squareAnnotations ctz64(const u64 bitboard) noexcept {
#if defined(__clang__) || defined(__GNUC__)
			return static_cast<chess::squareAnnotations>(__builtin_ctzll(bitboard));
#else
			static_assert(false, "Not a supported compiler platform - no known ctz function");
#endif
		}

		// Count leading zeros (of a number's binary representation)
		[[nodiscard]] inline chess::squareAnnotations clz64(const u64 bitboard) noexcept {
#if defined(__clang__) || defined(__GNUC__)
			return static_cast<chess::squareAnnotations>(__builtin_clzll(bitboard));
#else
			static_assert(false, "Not a supported compiler platform - no known clz function");
#endif
		}

		// Count the number of ones (in a number's binary representation)
		[[nodiscard]] inline auto popcnt64(const u64 bitboard) noexcept {
#if defined(__clang__) || defined(__GNUC__)
			return __builtin_popcountll(bitboard);
#else
			static_assert(false, "Not a supported compiler platform - no known popcount function");
#endif
		}

		// Functions to modify board/piece annotations
		[[nodiscard]] inline constexpr chess::boardAnnotations colorOf(const chess::boardAnnotations piece) noexcept {
			return static_cast<chess::boardAnnotations>(piece & 0x08);
		}
		[[nodiscard]] inline constexpr chess::boardAnnotations oppositeColorOf(const chess::boardAnnotations piece) noexcept {
			return static_cast<chess::boardAnnotations>(piece ^ 0x08);
		}

		[[nodiscard]] inline constexpr chess::boardAnnotations nullOf(const chess::boardAnnotations piece) noexcept {
			return static_cast<chess::boardAnnotations>(piece | 0x07);
		}
		[[nodiscard]] inline constexpr chess::u16 isPiece(chess::boardAnnotations piece) { return (piece & 0x7) == 0x7 ? 0x0000 : 0x1000; };

		[[nodiscard]] inline constexpr chess::boardAnnotations constructPiece(const chess::boardAnnotations piece, const chess::boardAnnotations color) {
			return static_cast<chess::boardAnnotations>(piece | color);
		}

		template <chess::attackRayDirection direction>
		[[nodiscard]] inline constexpr chess::u64 positiveRayAttacks(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
			chess::u64 attacks { chess::util::constants::attackRays[direction][squareFrom] };
			chess::u64 blocker { (attacks & occupiedSquares) | 0x8000000000000000 };
			attacks ^= chess::util::constants::attackRays[direction][chess::util::ctz64(blocker)];
			return attacks;
		}

		template <chess::attackRayDirection direction>
		[[nodiscard]] inline constexpr chess::u64 negativeRayAttacks(const chess::u8 squareFrom, const chess::u64 occupiedSquares) noexcept {
			chess::u64 attacks { chess::util::constants::attackRays[direction][squareFrom] };
			chess::u64 blocker { (attacks & occupiedSquares) | 0x1 };
			attacks ^= chess::util::constants::attackRays[direction][63U - chess::util::clz64(blocker)];
			return attacks;
		}
	}    // namespace util

	// More stores the
	struct moveData {
		chess::u8 originIndex;
		chess::u8 destinationIndex;
		chess::u16 flags;
		[[nodiscard]] inline constexpr chess::boardAnnotations capturedPiece() const noexcept { return static_cast<chess::boardAnnotations>((flags >> 0) & 0x000F); }
		[[nodiscard]] inline constexpr chess::boardAnnotations movePiece() const noexcept { return static_cast<chess::boardAnnotations>((flags >> 4) & 0x000F); }
		[[nodiscard]] inline constexpr chess::boardAnnotations promotionPiece() const noexcept { return static_cast<chess::boardAnnotations>((flags >> 8) & 0x000F); }
		[[nodiscard]] inline constexpr chess::u64 originSquare() const noexcept { return 1ULL << originIndex; }
		[[nodiscard]] inline constexpr chess::u64 destinationSquare() const noexcept { return 1ULL << destinationIndex; }
		[[nodiscard]] inline constexpr chess::u16 moveFlags() const noexcept { return flags & 0xFF00; }
		[[nodiscard]] inline std::string toString() const noexcept {
			auto result { chess::util::squareToAlgebraic(static_cast<chess::squareAnnotations>(this->originIndex)) + chess::util::squareToAlgebraic(static_cast<chess::squareAnnotations>(this->destinationIndex)) };
			if (char promotion { chess::util::pieceToChar(this->promotionPiece(), true) }; promotion == '\0')
				return result;
			else
				return result + promotion;
		}
	};

	template <class T = chess::moveData, size_t sz = 1024>
	class staticVector {
		 private:
		std::array<T, ((sz - sizeof(T*)) / sizeof(T))> moves;
		T* insertLocation;

		 public:
		staticVector() :
			moves {}, insertLocation { moves.data() } {}
		staticVector(const staticVector<T, sz>& other) :
			moves { other.moves }, insertLocation { (this->moves.data() - other.moves.data()) + other.insertLocation } {}
		inline void append(const T& moveToInsert) noexcept { *(insertLocation++) = moveToInsert; }
		inline void pop() noexcept { --insertLocation; }
		[[nodiscard]] inline T& operator[](std::size_t index) noexcept { return moves[index]; }
		[[nodiscard]] inline T* begin() noexcept { return moves.data(); }
		[[nodiscard]] inline T* end() noexcept { return insertLocation; }
		[[nodiscard]] inline chess::u64 size() const noexcept { return static_cast<chess::u64>(insertLocation - moves.data()); }
	};

	struct position {
		// Most to least information dense
		std::array<chess::u64, 16> bitboards;
		std::array<chess::boardAnnotations, 64> pieceAtIndex;

		chess::u8 flags;
		chess::u8 halfMoveClock;
		chess::u64 enPassantTargetSquare;
		chess::u64 zobristHash;

		[[nodiscard]] staticVector<moveData> moves() const noexcept;
		[[nodiscard]] position move(moveData desiredMove) const noexcept;
		[[nodiscard]] chess::u64 attacks(const chess::boardAnnotations turn) const noexcept;
		[[nodiscard]] chess::u64 attackers(const chess::squareAnnotations square, const chess::boardAnnotations attackingColor) const noexcept;
		template <chess::boardAnnotations piece, class returnType>
		[[nodiscard]] inline returnType pieceMoves(const u8 squareFrom) const noexcept { return returnType {}; };
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::boardAnnotations::bishop>(const chess::u8 squareFrom) const noexcept {
			const auto occupied { this->occupied() };
			return chess::util::positiveRayAttacks<chess::northWest>(squareFrom, occupied) | chess::util::positiveRayAttacks<chess::northEast>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::southWest>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::southEast>(squareFrom, occupied);
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::boardAnnotations::rook>(const chess::u8 squareFrom) const noexcept {
			const auto occupied { this->occupied() };
			return chess::util::positiveRayAttacks<chess::north>(squareFrom, occupied) | chess::util::positiveRayAttacks<chess::west>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::south>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::east>(squareFrom, occupied);
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::boardAnnotations::knight>(const chess::u8 squareFrom) const noexcept {
			return chess::util::constants::knightJumps[squareFrom];
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::boardAnnotations::queen>(const chess::u8 squareFrom) const noexcept {
			return this->pieceMoves<chess::boardAnnotations::rook, chess::u64>(squareFrom) | this->pieceMoves<chess::boardAnnotations::bishop, chess::u64>(squareFrom);
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::boardAnnotations::king>(const chess::u8 squareFrom) const noexcept {
			return chess::util::constants::kingAttacks[squareFrom];
		}
		template <>
		[[nodiscard]] inline std::array<chess::u64, 3> pieceMoves<chess::boardAnnotations::pawn>(const chess::u8 squareFrom) const noexcept {
			// Pawn push
			chess::u64 singlePawnPush = this->turn() ? ((1ULL << squareFrom) << 8) & this->empty() : ((1ULL << squareFrom) >> 8) & this->empty();
			chess::u64 doublePawnPush = this->turn() ? (singlePawnPush << 8) & this->empty() & 0xFF000000ULL : (singlePawnPush >> 8) & this->empty() & 0xFF00000000ULL;
			// Pawn capture
			chess::u64 pawnCapture = this->turn() ? chess::util::constants::pawnAttacks[1][squareFrom] & (this->bitboards[chess::boardAnnotations::black] | this->enPassantTargetSquare) : chess::util::constants::pawnAttacks[0][squareFrom] & (this->bitboards[chess::boardAnnotations::white] | this->enPassantTargetSquare);
			return { singlePawnPush, doublePawnPush, pawnCapture };
		}

		[[nodiscard]] std::string ascii() const noexcept;
		[[nodiscard]] inline constexpr chess::boardAnnotations turn() const noexcept { return static_cast<chess::boardAnnotations>(flags & 0x08); }    // 0 - 1 (1 bit)
		[[nodiscard]] inline constexpr chess::u64 occupied() const noexcept { return bitboards[chess::boardAnnotations::white] | bitboards[chess::boardAnnotations::black]; }
		[[nodiscard]] inline constexpr chess::u64 empty() const noexcept { return ~occupied(); }
		[[nodiscard]] inline constexpr bool valid() const noexcept { return flags & 0x04; }    // 0 - 1 (1 bit)
		[[nodiscard]] inline constexpr bool castleWK() const noexcept { return flags & 0x80; }
		[[nodiscard]] inline constexpr bool castleWQ() const noexcept { return flags & 0x40; }
		[[nodiscard]] inline constexpr bool castleBK() const noexcept { return flags & 0x20; }
		[[nodiscard]] inline constexpr bool castleBQ() const noexcept { return flags & 0x10; }
		inline constexpr void setZobrist() noexcept {
			this->zobristHash = 0;
			for (size_t index = 0; index < 64; index++) {
				this->zobristHash ^= chess::util::constants::zobristBitStrings[index][pieceAtIndex[index]];
			}
		}

		[[nodiscard]] std::string toFen() const noexcept;
		[[nodiscard]] std::string toFen(std::size_t turnCount) const noexcept;

		[[nodiscard]] static position fromFen(const std::string& fen) noexcept;
		[[nodiscard]] static bool validateFen(const std::string& fen) noexcept;

		inline static constexpr void nextTurn(position& toModify) noexcept { toModify.flags ^= 0x08; };    // Switches the turn on a position
	};

	struct game {
		std::vector<position> gameHistory;

		game(const std::string& fen) noexcept :
			gameHistory { { chess::position::fromFen(fen) } } {}

		[[nodiscard]] staticVector<moveData> moves() const noexcept;
		[[nodiscard]] inline constexpr u8 result() const noexcept { return 0; };    // unused
		[[nodiscard]] inline constexpr bool finished() const noexcept { return this->threeFoldRep() || this->moves().size() == 0; }
		[[nodiscard]] constexpr bool threeFoldRep() const noexcept {
			size_t count = 0;
			for (size_t i = 0; i < gameHistory.size(); i += 2) {
				if (gameHistory[i].zobristHash == gameHistory.back().zobristHash)
					count++;
			}
			return count >= 3;
		}
		[[nodiscard]] inline const position& currentPosition() const noexcept { return gameHistory.back(); }
		void move(const moveData desiredMove) noexcept;
		void move(const std::string& uciMove) noexcept;
		bool undo() noexcept;
	};

	[[nodiscard]] inline game defaultGame() noexcept { return game("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); };

	namespace debugging {
		std::string bitboardToString(u64 bitBoard);
	}

}    // namespace chess
