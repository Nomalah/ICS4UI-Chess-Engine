#ifndef NMLH_CHESS_TYPES_HPP
#define NMLH_CHESS_TYPES_HPP

namespace chess {
	// Types used in the chess engine
	typedef unsigned char u8;
	typedef unsigned short u16;
	typedef unsigned int u32;
	typedef unsigned long long u64;
	static_assert(sizeof(chess::u8) == 1 && sizeof(chess::u16) == 2 && sizeof(chess::u32) == 4 && sizeof(chess::u64) == 8);

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

	enum piece : chess::u8
	{
		noColor     = 0x0,
		pawn        = 0x1,
		knight      = 0x2,
		bishop      = 0x3,
		rook        = 0x4,
		queen       = 0x5,
		king        = 0x6,

		black       = 0x0,
		blackPawn   = 0x1,
		blackKnight = 0x2,
		blackBishop = 0x3,
		blackRook   = 0x4,
		blackQueen  = 0x5,
		blackKing   = 0x6,

		white       = 0x8,
		whitePawn   = 0x9,
		whiteKnight = 0xA,
		whiteBishop = 0xB,
		whiteRook   = 0xC,
		whiteQueen  = 0xD,
		whiteKing   = 0xE,

		empty       = 0x7,
		occupied    = 0xF
	};

	enum square : chess::u8
	{
		// clang-format off
            h1, g1, f1, e1, d1, c1, b1, a1,
            h2, g2, f2, e2, d2, c2, b2, a2,
            h3, g3, f3, e3, d3, c3, b3, a3,
            h4, g4, f4, e4, d4, c4, b4, a4,
            h5, g5, f5, e5, d5, c5, b5, a5,
            h6, g6, f6, e6, d6, c6, b6, a6,
            h7, g7, f7, e7, d7, c7, b7, a7,
            h8, g8, f8, e8, d8, c8, b8, a8,
		// clang-format on
	};

	constexpr chess::square operator++(chess::square& targetSquare) noexcept {
		return targetSquare = static_cast<chess::square>(targetSquare + 1);
	}

	enum moveFlags : chess::u16
	{
		quiet                 = 0x0000,
		quietPromotionBegin   = 0x0100,
		quietPromotionEnd     = 0x0F00,
		capture               = 0x1000,
		capturePromotionBegin = 0x1100,
		capturePromotionEnd   = 0x1F00,
		whiteKingsideCastle   = 0x2000,
		whiteQueensideCastle  = 0x3000,
		blackKingsideCastle   = 0x4000,
		blackQueensideCastle  = 0x5000,
		whiteEnPassant        = 0x6000,
		blackEnPassant        = 0x7000,
		whiteDoublePush       = 0x8000,
		blackDoublePush       = 0x9000
	};
}    // namespace chess

#endif    // NMLH_CHESS_TYPES_HPP