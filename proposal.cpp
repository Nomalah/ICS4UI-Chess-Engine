/*
namespace chess
    game newGame()
        default position
    
    game 
        white & black bitboards
            knight
            pawn
            king
            queen
            rook
            bishop
        




// Everything should be handled in the game class, the programmer using the library should not need extra code, 
// It should be very simplified for use.
main loop
    game = chess::newGame()
    while !game.end()
        get user move
        convert user move to chess::move
        if user move in game.moves()
            game.make(user move)
        print game.position.ascii
    print game.result
*/