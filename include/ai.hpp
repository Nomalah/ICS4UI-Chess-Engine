#ifndef NMLH_CHESS_AI_H
#define NMLH_CHESS_AI_H

#include "chess.hpp"
#include <algorithm>
#include <limits>
#include <cmath>
#include <memory>

#define SWAP(a, b)       \
	do {                 \
		auto _c { (a) }; \
		(a) = (b);       \
		(b) = _c;        \
	} while (0)

namespace chess::ai {
#ifdef AI_MAX_PLY
	constexpr int searchPly = AI_MAX_PLY;
#else
	constexpr int searchPly = 5;
#endif
	constexpr int maxQSearchPly = -40;
	static_assert(searchPly - maxQSearchPly < ((1024 - sizeof(moveData*)) / sizeof(moveData)), "Ply Depth Too Large");

	inline bool isACapture(chess::u16 flag) {
		return ((flag & 0xF000) == 0x1000) || ((flag & 0xF000) == 0x6000) || ((flag & 0xF000) == 0x7000);
	}

	namespace {
		template <size_t sz>
		constexpr bool isPowerOfTwo() {
			// constexpr popcount
			size_t num { sz };
			unsigned int numOfOnes = 0;
			while (num && numOfOnes <= 1) {
				numOfOnes += num & 1;
				num >>= 1;
			}
			return numOfOnes == 1;
		}
	};    // namespace

	template <size_t sz>
	class transpositionTable {
		static_assert(isPowerOfTwo<sz>(), "Transposition table requires a power of two size");
		// https://github.com/SebLague/Chess-AI/blob/main/Assets/Scripts/Core/TranspositionTable.cs
		// 24 bytes
	public:
		struct tableEntry {
			u64 key;
			chess::moveData move;
			int posEval;
			int depth;
			int nodeType;
		};

	private:
		const chess::game& boardPos;
		std::unique_ptr<std::array<tableEntry, sz>> tableData;
		const decltype(tableData->data()) tableDataBegin;
		size_t overwrites = 0;

	public:
		transpositionTable(const chess::game& board) :
			boardPos { board }, tableData { std::make_unique<std::array<tableEntry, sz>>() }, tableDataBegin { tableData->data() }, overwrites { 0 } {}

		static constexpr int lookupFailed { std::numeric_limits<int>::min() };
		static constexpr int unused { 0 };
		static constexpr int exact { 1 };
		static constexpr int lowerBound { 2 };
		static constexpr int upperBound { 3 };

		size_t getOverwrites() {
			return overwrites;
		}
		void clear() {
			std::fill(tableData->begin(), tableData->end(), { 0, { 0, 0, 0 }, 0, 0, 0 });
		}

		[[nodiscard]] inline u64 index() const noexcept {
			return boardPos.currentPosition().zobristHash & (sz - 1);
		}

		[[nodiscard]] inline chess::moveData getStoredMove() const noexcept {
			return tableDataBegin[index()].move;
		}

		void storeEval(int depth, int posEval, int evalType, chess::moveData move) {
			if (tableDataBegin[index()].nodeType != unused)
				overwrites++;

			if (depth >= tableDataBegin[index()].depth) {
				tableDataBegin[index()] = { .key      = boardPos.currentPosition().zobristHash,
					                        .move     = move,
					                        .posEval  = posEval,
					                        .depth    = depth,
					                        .nodeType = evalType };
			}
		}

		int lookupEval(int depth, int alpha, int beta) {
			tableEntry lookupEntry { tableDataBegin[index()] };
			if (lookupEntry.key == boardPos.currentPosition().zobristHash) {
				if (lookupEntry.depth >= depth) {
					if (lookupEntry.nodeType == exact) {
						return lookupEntry.posEval;
					}

					if (lookupEntry.nodeType == upperBound && lookupEntry.posEval <= alpha) {
						return lookupEntry.posEval;
					}

					if (lookupEntry.nodeType == lowerBound && lookupEntry.posEval >= beta) {
						return lookupEntry.posEval;
					}
				}
			}
			return lookupFailed;
		}
	};

	struct botWeights {
		struct {
			int pieceValues[16];
			int hashMove;
			int notHashMove;
			int promotionMultiplier;
			int captureMultiplier;
			int kingsideCastling;
			int queensideCastling;
			int enPassant;
			int pawnDoublePush;
			int defaultMove;
		} moveOrdering;

		struct {
			int pieceValues[16];
		} evaluate;
	};

	class bot {
	private:
		botWeights internalWeights;

	public:
		bot(const botWeights& setWeights) :
			internalWeights { setWeights } {}
		// Order moves to induce more beta cutoffs
		template <size_t TTsz>
		void orderMoves(moveList& moveList, const transpositionTable<TTsz>& TT) const noexcept {
			std::array<int, chess::constants::maxMoves> moveEvaluationHeuristicList {};
			const auto hashMove { TT.getStoredMove() };
			auto evaluateInsertLocation { moveEvaluationHeuristicList.begin() };
			for (const auto moveToEvaluate : moveList) {
				*evaluateInsertLocation = ((hashMove == moveToEvaluate) ? this->internalWeights.moveOrdering.hashMove : this->internalWeights.moveOrdering.notHashMove);
				if (moveToEvaluate.moveFlags() >= 0x0100 && moveToEvaluate.moveFlags() <= 0x0F00) {
                    // Promotion
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.promotionPiece()] * this->internalWeights.moveOrdering.promotionMultiplier;
                } else if (moveToEvaluate.moveFlags() >= 0x1100 && moveToEvaluate.moveFlags() <= 0x1F00) {
                    // Promotion & Capture
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.promotionPiece()] * this->internalWeights.moveOrdering.promotionMultiplier;
                    *evaluateInsertLocation += (this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.capturedPiece()] - this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.movePiece()]) * this->internalWeights.moveOrdering.captureMultiplier;
                } else if (moveToEvaluate.moveFlags() == 0x1000) {
                    // Capture logic
                    *evaluateInsertLocation += (this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.capturedPiece()] - this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.movePiece()]) * this->internalWeights.moveOrdering.captureMultiplier;
                } else if (moveToEvaluate.moveFlags() == 0x2000) {
                    // Kingside castling
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.kingsideCastling;
                } else if (moveToEvaluate.moveFlags() == 0x3000) {
                    // Queenside castling
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.queensideCastling;
                } else if (moveToEvaluate.moveFlags() == 0x4000) {
                    // En passant
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.enPassant;
                } else if (moveToEvaluate.moveFlags() == 0x5000) {
                    // En passant capture
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.enPassant;
                    *evaluateInsertLocation += (this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.capturedPiece()] - this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.movePiece()]) * this->internalWeights.moveOrdering.captureMultiplier;
                } else if (moveToEvaluate.moveFlags() == 0x6000) {
                    // Pawn double push
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.pawnDoublePush;
                } else if (moveToEvaluate.moveFlags() == 0x7000) {
                    // Pawn double push capture
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.pawnDoublePush;
                    *evaluateInsertLocation += (this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.capturedPiece()] - this->internalWeights.moveOrdering.pieceValues[moveToEvaluate.movePiece()]) * this->internalWeights.moveOrdering.captureMultiplier;
                } else {
                    // Default move
                    *evaluateInsertLocation += this->internalWeights.moveOrdering.defaultMove;
				}
				++evaluateInsertLocation;
			}

			// Sort based on the evaluation of moves
			for (int i = 0; i < moveList.size(); i++) {
				for (int j = i + 1; j < moveList.size(); j++) {
					if (moveEvaluationHeuristicList[i] < moveEvaluationHeuristicList[j]) {
						SWAP(moveEvaluationHeuristicList[i], moveEvaluationHeuristicList[j]);
						SWAP(moveList[i], moveList[j]);
					}
				}
			}
		}

		[[nodiscard]] int evaluate(const chess::position& toEvaluate) const noexcept {
			using namespace chess::util;
			using namespace chess::constants;
			static auto pieceValue = [&toEvaluate, this](chess::u8 piece) -> int {
				static constexpr int pieceValues[] = { 0, 100, 300, 310, 500, 900, 0, 0, 0, 100, 300, 310, 500, 900, 0 };
				return popcnt64(toEvaluate.bitboards[piece]) * pieceValues[piece];
			};

			static auto positionalValue = [&toEvaluate](chess::u8 piece) -> int {
				static constexpr chess::u64 positionValueCenter      = 0x0000001818000000;
				static constexpr chess::u64 positionValueLargeCenter = 0x00003C3C3C3C0000;
				return popcnt64(toEvaluate.bitboards[piece] & positionValueCenter) * 50 + popcnt64(toEvaluate.bitboards[piece] & positionValueLargeCenter) * 25;
			};

			int result = 0;
			result += pieceValue(whitePawn);
			result += pieceValue(whiteKnight);
			result += pieceValue(whiteBishop);
			result += pieceValue(whiteRook);
			result += pieceValue(whiteQueen);
			result += positionalValue(white);
			result -= pieceValue(blackPawn);
			result -= pieceValue(blackKnight);
			result -= pieceValue(blackBishop);
			result -= pieceValue(blackRook);
			result -= pieceValue(blackQueen);
			result -= positionalValue(black);
			return toEvaluate.turn() ? result - toEvaluate.halfMoveClock * 4 : -result + toEvaluate.halfMoveClock * 4;
		}
	};

	chess::moveData bestMove(chess::game& gameToTest, const chess::ai::bot& botToUse) {
#ifdef AI_DEBUG
		static size_t totalNodes = 0;
#endif
		[[maybe_unused]] size_t nodes = 0;
		struct minimaxOutput {
			int eval;
			chess::moveData reccomendedMove;
		};

		//transpositionTable<(1 << 24)> TT { gameToTest };

		auto alphaBeta = [&gameToTest, &nodes, /*&TT,*/ &botToUse](const auto alphaBeta, int alpha, const int beta, const int ply, const bool capture) -> minimaxOutput {
			if ((ply < 1 && !capture) || ply < maxQSearchPly) {
#ifdef AI_DEBUG
				nodes++;
#endif
				return { botToUse.evaluate(gameToTest.currentPosition()), { 0, 0, 0 } };
			}
			auto legalMoves = gameToTest.moves();
			if (legalMoves.size() == 0) {
				return {
					(gameToTest.threeFoldRep() || gameToTest.currentPosition().halfMoveClock >= 50)
						? -500
						: (gameToTest.currentPosition().turn()
					           ? (gameToTest.currentPosition().inCheck<white>() ? -20000 - ply * 128 : -500)
					           : (gameToTest.currentPosition().inCheck<black>() ? -20000 - ply * 128 : -500)),
					{ 0, 0, 0 }
				};
			}

			/*const int ttVal = TT.lookupEval(ply, alpha, beta);
			if (ttVal != decltype(TT)::lookupFailed) {
				return { ttVal, TT.getStoredMove() };
			}

			int evalType      = decltype(TT)::upperBound;
            */
			moveData bestMove = { 0, 0, 0 };
			//botToUse.orderMoves(legalMoves, TT);    // In place sorting of legalMoves
			for (auto legalMove : legalMoves) {
				gameToTest.move(legalMove);
				int posEval = -alphaBeta(alphaBeta, -beta, -alpha, ply - 1, isACapture(legalMove.flags)).eval;
				if (ply == searchPly) {
					std::cerr << legalMove.toString() << " : " << posEval << '\n';
				}
				gameToTest.undo();

				if (posEval >= beta) {
					//TT.storeEval(ply, beta, decltype(TT)::lowerBound, bestMove);
					return { beta, bestMove };
				}
				if (posEval > alpha) {
					//evalType = decltype(TT)::exact;
					alpha    = posEval;
					bestMove = legalMove;
				}
			}
			//TT.storeEval(ply, alpha, evalType, bestMove);
			return { alpha, bestMove };
		};

		minimaxOutput result { 0, { 0, 0, 0 } };
		result = alphaBeta(alphaBeta, std::numeric_limits<short>::min(), std::numeric_limits<short>::max(), searchPly, false);
		std::cerr << "resulteval:" << result.eval << "\n";

#ifdef AI_DEBUG
		std::cout << "Evaluated " << nodes << " nodes, Eval: " << result.eval << " Move:" << result.reccomendedMove.toString() << std::endl;
		totalNodes += nodes;
		std::cout << "Total Nodes Thus Far:" << totalNodes << std::endl;
#endif
		return result.reccomendedMove;
	}
}    // namespace chess::ai

#endif    // NMLH_CHESS_AI_H