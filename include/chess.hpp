#ifndef NMLH_CHESS_HPP
#define NMLH_CHESS_HPP

#include <array>
#include <string>
#include <vector>
#include <algorithm>

#include "chess_types.hpp"
#include "chess_constants.hpp"
#include "chess_utils.hpp"

namespace chess {
	struct moveData {
		chess::u16 flags;
		chess::u8 originIndex;
		chess::u8 destinationIndex;
		[[nodiscard]] constexpr chess::piece capturedPiece() const noexcept { return static_cast<chess::piece>((flags >> 0) & 0x000F); }
		[[nodiscard]] constexpr chess::piece movePiece() const noexcept { return static_cast<chess::piece>((flags >> 4) & 0x000F); }
		[[nodiscard]] constexpr chess::piece promotionPiece() const noexcept { return static_cast<chess::piece>((flags >> 8) & 0x000F); }
		[[nodiscard]] constexpr chess::u64 originSquare() const noexcept { return 1ULL << originIndex; }
		[[nodiscard]] constexpr chess::u64 destinationSquare() const noexcept { return 1ULL << destinationIndex; }
		[[nodiscard]] constexpr chess::u16 moveFlags() const noexcept { return flags & 0xFF00; }
		[[nodiscard]] inline std::string toString() const noexcept {
			auto result { chess::util::squareToAlgebraic(static_cast<chess::square>(this->originIndex)) + chess::util::squareToAlgebraic(static_cast<chess::square>(this->destinationIndex)) };
			if (this->promotionPiece() == 0)
				return result;
			else
				return result + chess::util::pieceToChar(chess::util::getPieceOf(this->promotionPiece()));
		}
	};
	[[nodiscard]] constexpr bool operator==(const moveData lhs, const moveData rhs) {
		return lhs.originIndex == rhs.originIndex && lhs.destinationIndex == rhs.destinationIndex && lhs.flags == rhs.flags;
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
		std::array<chess::piece, 64> pieceAtIndex;

		chess::u8 flags;
		chess::u8 halfMoveClock;
		chess::u64 enPassantTargetBitboard;
		chess::u64 zobristHash;
		chess::u16 fullMoveClock;

		template <chess::piece allyColor>
		[[nodiscard]] staticVector<moveData> moves() const noexcept;
		[[nodiscard]] position move(moveData desiredMove) const noexcept;
		template <chess::piece attackingColor>
		[[nodiscard]] chess::u64 attacks() const noexcept;
		template <chess::piece attackingColor>
		[[nodiscard]] chess::u64 attackers(const chess::square targetSquare) const noexcept {
			using namespace chess::util;
			return (this->pieceMoves<bishop>(targetSquare, this->bitboards[occupied]) & (this->bitboards[constructPiece(bishop, attackingColor)] | this->bitboards[constructPiece(queen, attackingColor)])) |
			       (this->pieceMoves<rook>(targetSquare, this->bitboards[occupied]) & (this->bitboards[constructPiece(rook, attackingColor)] | this->bitboards[constructPiece(queen, attackingColor)])) |
			       (this->pieceMoves<knight>(targetSquare, this->bitboards[occupied]) & this->bitboards[constructPiece(knight, attackingColor)]) |
			       (constants::pawnAttacks[~attackingColor >> 3][targetSquare] & this->bitboards[constructPiece(pawn, attackingColor)]);
		}
		template <chess::piece defendingColor>
		[[nodiscard]] bool inCheck() const noexcept {
			return static_cast<bool>(this->attackers<~defendingColor>(chess::util::ctz64(this->bitboards[chess::util::constructPiece(chess::piece::king, defendingColor)])));
		}
		template <chess::piece targetPiece>
		[[nodiscard]] inline chess::u64 pieceMoves(const u8 squareFrom, const u64 occupied) const noexcept { return chess::u64 {}; };
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::piece::bishop>(const chess::u8 squareFrom, const u64 occupied) const noexcept {
			return chess::util::positiveRayAttacks<chess::northWest>(squareFrom, occupied) | chess::util::positiveRayAttacks<chess::northEast>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::southWest>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::southEast>(squareFrom, occupied);
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::piece::rook>(const chess::u8 squareFrom, const u64 occupied) const noexcept {
			return chess::util::positiveRayAttacks<chess::north>(squareFrom, occupied) | chess::util::positiveRayAttacks<chess::west>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::south>(squareFrom, occupied) | chess::util::negativeRayAttacks<chess::east>(squareFrom, occupied);
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::piece::knight>(const chess::u8 squareFrom, const u64 occupied) const noexcept {
			return chess::constants::knightJumps[squareFrom];
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::piece::queen>(const chess::u8 squareFrom, const u64 occupied) const noexcept {
			return this->pieceMoves<chess::piece::rook>(squareFrom, occupied) | this->pieceMoves<chess::piece::bishop>(squareFrom, occupied);
		}
		template <>
		[[nodiscard]] inline chess::u64 pieceMoves<chess::piece::king>(const chess::u8 squareFrom, const u64 occupied) const noexcept {
			return chess::constants::kingAttacks[squareFrom];
		}

		template <chess::piece targetPiece>
		void generatePieceMoves(chess::staticVector<chess::moveData>& legalMoves, const chess::u64 pinnedPieces, const chess::u64 notAlly, const chess::u64 mask = chess::constants::bitboardFull) const noexcept;

		[[nodiscard]] std::string ascii() const noexcept;
		[[nodiscard]] constexpr chess::piece turn() const noexcept { return static_cast<chess::piece>(flags & 0x08); }    // 0 - 1 (1 bit)
		[[nodiscard]] constexpr chess::u64 empty() const noexcept { return bitboards[chess::piece::empty]; }
		[[nodiscard]] constexpr bool valid() const noexcept { return flags & 0x04; }    // 0 - 1 (1 bit)
		[[nodiscard]] constexpr bool castleWK() const noexcept { return flags & 0x80; }
		[[nodiscard]] constexpr bool castleWQ() const noexcept { return flags & 0x40; }
		[[nodiscard]] constexpr bool castleBK() const noexcept { return flags & 0x20; }
		[[nodiscard]] constexpr bool castleBQ() const noexcept { return flags & 0x10; }
		constexpr void setZobrist() noexcept {
			this->zobristHash = 0;
			for (size_t index = 0; index < 64; index++) {
				this->zobristHash ^= chess::constants::zobristBitStrings[index][pieceAtIndex[index]];
			}
		}

		[[nodiscard]] std::string toFen() const noexcept;

		[[nodiscard]] static position fromFen(const std::string& fen) noexcept;
		[[nodiscard]] static bool validateFen(const std::string& fen) noexcept;
	};

	struct game {
		std::vector<position> gameHistory;

		game(const std::string& fen) noexcept :
			gameHistory { { chess::position::fromFen(fen) } } {}

		[[nodiscard]] staticVector<moveData> moves() const noexcept;
		template <chess::piece allyColor>
		[[nodiscard]] staticVector<moveData> moves() const noexcept;
		[[nodiscard]] constexpr u8 result() const noexcept { return 0; };    // unused
		[[nodiscard]] constexpr bool finished() const noexcept { return this->threeFoldRep() || this->moves().size() == 0; }
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

#endif    // NMLH_CHESS_HPP