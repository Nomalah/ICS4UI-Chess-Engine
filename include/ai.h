#pragma once

#include "chess.h"
#include <algorithm>

namespace chess::ai{
#ifdef AI_MAX_PLY
constexpr int searchPly = AI_MAX_PLY;
#else 
constexpr int searchPly = 5;
#endif
constexpr int maxQSearchPly = -20;
static_assert(searchPly > 1, "Ply Depth Too Small");
static_assert(searchPly - maxQSearchPly < ((1024 - sizeof(moveData*))/sizeof(moveData)), "Ply Depth Too Large");



int evaluate(const chess::position& toEvaluate){
    using namespace chess::util;
    using namespace chess::util::constants;
    auto value = [&toEvaluate](chess::u8 piece) -> int {
        static constexpr int pieceValues[] = {0, 100, 300, 310, 500, 900, 0, 0, 0, 100, 300, 310, 500, 900, 0};
        return popcnt64(toEvaluate.bitboards[piece]) * pieceValues[piece];
    };

    auto positionalValue = [&toEvaluate](chess::u8 piece) -> int {
        static constexpr chess::u64 positionValueCenter = 0x0000001818000000;
        static constexpr chess::u64 positionValueLargeCenter = 0x00003C3C3C3C0000;
        //static constexpr int pieceValues[] = {0, -100, -300, -310, -500, -900, 0, 0, 0, 100, 300, 310, 500, 900, 0};
        return popcnt64(toEvaluate.bitboards[piece] & positionValueCenter) * 50 + popcnt64(toEvaluate.bitboards[piece] & positionValueLargeCenter) * 25;
    };


    int result = 0;
    result += value(whitePawn) + positionalValue(whitePawn);
    result += value(whiteKnight) + positionalValue(whiteKnight);
    result += value(whiteBishop) + positionalValue(whiteBishop);
    result += value(whiteRook) + positionalValue(whiteRook);
    result += value(whiteQueen) + positionalValue(whiteQueen);
    result -= value(blackPawn) + positionalValue(blackPawn);
    result -= value(blackKnight) + positionalValue(blackKnight);
    result -= value(blackBishop) + positionalValue(blackBishop);
    result -= value(blackRook) + positionalValue(blackRook);
    result -= value(blackQueen) + positionalValue(blackQueen);
    return toEvaluate.turn() ? result - toEvaluate.halfMoveClock * 4 : -result + toEvaluate.halfMoveClock * 4;
}

inline bool isACapture(chess::u16 flag){
    return ((flag & 0xF000) == 0x1000) || ((flag & 0xF000) == 0x6000) || ((flag & 0xF000) == 0x7000);
}

chess::moveData bestMove(chess::game& gameToTest){
#ifdef AI_DEBUG
    static size_t totalNodes = 0;
#endif
    [[maybe_unused]] size_t nodes = 0;
    struct minimaxOutput {
        int eval;
        chess::moveData reccomendedMove;
    };

    auto alphaBeta = [&gameToTest, &nodes](const auto alphaBeta, int alpha, const int beta, const int ply, const bool capture) -> minimaxOutput {
        if((ply < 1 && !capture) || ply < maxQSearchPly){
#ifdef AI_DEBUG
		nodes++;
#endif
            return {evaluate(gameToTest.currentPosition()), {0, 0, 0}};
        }
        auto legalMoves = gameToTest.moves();
        if(legalMoves.size() == 0){
            return {
                gameToTest.threeFoldRep() ? 0 :
                gameToTest.currentPosition().halfMoveClock >= 50 ? 0 :
                (gameToTest.currentPosition().turn() ? 
                    (gameToTest.currentPosition().squareAttacked(chess::util::ctz64(gameToTest.currentPosition().bitboards[chess::whiteKing])) ? 
                        -1000000000 
                        : 0) 
                    : (gameToTest.currentPosition().squareAttacked(chess::util::ctz64(gameToTest.currentPosition().bitboards[chess::blackKing])) ?
                        1000000000
                        : 0))    
            , {0, 0, 0}};
        }
        moveData bestMove = {0, 0, 0}; 
        for(auto legalMove : legalMoves){
            gameToTest.move(legalMove);
            int posEval = -alphaBeta(alphaBeta, -beta, -alpha, ply - 1, isACapture(legalMove.flags)).eval;
            gameToTest.undo();
            if(posEval >= beta){
                return {beta, bestMove};
            }
            if(posEval > alpha){
                alpha = posEval;
                bestMove = legalMove;
            }
        }
        return {alpha, bestMove};
    };
    auto result = alphaBeta(alphaBeta, -1000000000, 100000000, searchPly, false);
#ifdef AI_DEBUG
    std::cout << "Evaluated " << nodes << " nodes, Eval: " << result.eval << " Move:" << result.reccomendedMove.toString() << std::endl;
    totalNodes += nodes;
    std::cout << "Total Nodes Thus Far:" << totalNodes << std::endl;
#endif
    return result.reccomendedMove;
}
}
