#ifndef GAMES_H
#define GAMES_H

#include <QString>


//! GameSec
/*!
    Utilities for the games
*/
class GameSec
{
public:
    GameSec();

    static void crisscrossStyleSheet( QString& stylesheet, const int& theme_id );
    static void snakeStyleSheet( QString& stylesheet, const int& theme_id );
};

#endif // GAMES_H
