#ifndef COLOROUTPUT_H
#define COLOROUTPUT_H

#include <ostream>

namespace Color
{
    enum Code {
		FG_BLACK	= 30,
        FG_RED      = 31,
        FG_GREEN    = 32,
		FG_ORANGE	= 33,
        FG_BLUE     = 34,
		FG_MAGENTA	= 35,
		FG_CYAN		= 36,
		FG_GREY		= 37,
		FG_DGREY	= 90,
		FG_LRED		= 91,
		FG_LGREEN	= 92,
		FG_YELLOW	= 93,
		FG_LBLUE	= 94,
		FG_LMAG		= 95,
		FG_LCYAN	= 96,
		FG_WHITE	= 97,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
		BG_YELLOW 	= 43,
        BG_BLUE     = 44,
		BG_CYAN		= 45,
        BG_DEFAULT  = 49
    };
    class Modifier
    {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod)
		{
            return os << "\033[" << mod.code << "m";
        }
    };
}

/*
Color::Modifier fRed(Color::FG_RED);
Color::Modifier fLTRed(Color::FG_LRED);
Color::Modifier fOrange(Color::FG_ORANGE);
Color::Modifier fYellow(Color::FG_YELLOW);
Color::Modifier fGreen(Color::FG_GREEN);
Color::Modifier fLTGreen(Color::FG_LGREEN);
Color::Modifier fBlue(Color::FG_BLUE);
Color::Modifier fLTBlue(Color::FG_LBLUE);
Color::Modifier fCyan(Color::FG_CYAN);
Color::Modifier fLTCyan(Color::FG_LCYAN);
Color::Modifier fMagenta(Color::FG_MAGENTA);
Color::Modifier fLTMagenta(Color::FG_LMAG);
Color::Modifier fGrey(Color::FG_GREY);
Color::Modifier fDGrey(Color::FG_DGREY);
Color::Modifier fBlack(Color::FG_BLACK);
Color::Modifier fWhite(Color::FG_WHITE);
Color::Modifier fDefault(Color::FG_DEFAULT);
*/



#endif
