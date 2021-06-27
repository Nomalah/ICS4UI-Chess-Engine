/*
staticvector
    array of type T
    length
    operator[]()
    begin()
    end()
    append(obj){
        array[length++] = obj
    }

position
    white & black bitboards
        knight
        pawn
        king
        queen
        rook
        bishop
    hash <- for comparing for 3 fold repatition
    50 move rule
    ascii()
    moves() <- returns a static vector to avoid dynamic allocation
    move() <- returns new position

game newGame()
    default position

game 
    list of positions
    finished()
    checkmate()
    draw()
    result()
    move(){
        list.push(currentPosition.move())
    }
    undo() {
        list.pop()
    }

bot
    evaluate(position){
        count piece values
        add bias towards center squares
        return evaluation
    }
    best move(game){
        minimax(alpha, beta, depth){
            if depth is at lowest value
                return evaluation
            for every legal move in game
                make move
                result = minimax(beta, alpha, depth - 1)
                undo move
                if result better then best
                    update best move
            return best move
        }

        return best move from minimax(-infinity, infinity)
    }

// Everything should be handled in the game class, the programmer using the library should not need extra code, 
// It should be very simplified for use.
main loop
    game = newGame()
    while !game.finished()
        get user move
        if user move in game.moves()
            game.make(user move)
        else
            get user move again
        print game.position.ascii

        if !game.finished()
            get bot move
            game.make(bot move)
            print game.position.ascii
            
    print game.result
*/