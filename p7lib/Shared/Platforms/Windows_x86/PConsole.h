////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// 2012-2020 (c) Baical                                                        /
//                                                                             /
// This library is free software; you can redistribute it and/or               /
// modify it under the terms of the GNU Lesser General Public                  /
// License as published by the Free Software Foundation; either                /
// version 3.0 of the License, or (at your option) any later version.          /
//                                                                             /
// This library is distributed in the hope that it will be useful,             /
// but WITHOUT ANY WARRANTY; without even the implied warranty of              /
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           /
// Lesser General Public License for more details.                             /
//                                                                             /
// You should have received a copy of the GNU Lesser General Public            /
// License along with this library.                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <conio.h>
#include "AList.h"
#include "PString.h"
#include "IConsole.h"
#include "RBTree.h"

#pragma warning( disable : 4482 )  

//http://invisible-island.net/ncurses/
//https://github.com/fffaraz/awesome-cpp
//http://www.leonerd.org.uk/code/libtickit/


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Is_Key_Hit
static tBOOL Is_Key_Hit(void)
{
    return (_kbhit()) ? TRUE : FALSE;
}//Is_Key_Hit


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get_Char
static tXCHAR Get_Char()
{
    return _getwch();
}//Get_Char


////////////////////////////////////////////////////////////////////////////////
struct stKeyMap
{
    tUINT16 wKey;
    tUINT16 wValue;
    stKeyMap(tUINT16 i_wKey, tUINT16 i_wValue) : wKey(i_wKey), wValue(i_wValue) {}
};

////////////////////////////////////////////////////////////////////////////////
//CDesc_Tree
class CKeysTree:
    public CRBTree<stKeyMap*, tUINT16>
{
public:
    ////////////////////////////////////////////////////////////////////////////
    //~CKeysTree
    virtual ~CKeysTree()
    {
        Clear();
    }//~CRBTree

protected:
    //////////////////////////////////////////////////////////////////////////// 
    virtual tBOOL Data_Release(stKeyMap* i_pData)
    {
        delete i_pData;
        return TRUE; //memory used from pool, not necessary to return
    }// Data_Release

    //////////////////////////////////////////////////////////////////////////// 
    //Return
    //TRUE  - if (i_pKey < i_pData::key)
    //FALSE - otherwise
    tBOOL Is_Key_Less(tUINT16 i_pKey, stKeyMap *i_pData) 
    {
        return (i_pKey < i_pData->wKey);
    }

    //////////////////////////////////////////////////////////////////////////// 
    //Return
    //TRUE  - if (i_pKey == i_pData::key)
    //FALSE - otherwise
    tBOOL Is_Qual(tUINT16 i_pKey, stKeyMap *i_pData) 
    {
        return (i_pKey == i_pData->wKey);
    }
};//CDesc_Tree


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CConsole
    : public IConsole
{
    tUINT16   m_pFg[eColorsMax];
    tUINT16   m_pBg[eColorsMax];
    size_t    m_szText;
    tXCHAR   *m_pText;
    HANDLE    m_hStdOut;
    HANDLE    m_hStdIn;
    tXCHAR   *m_pEmpty;
    size_t    m_szEmpty;
    CKeysTree m_pKeys;
public:
    CConsole()
    {
        m_szText = 32768;
        m_pText  = (tXCHAR*)malloc(m_szText * sizeof(tXCHAR));

        m_szEmpty = 0;
        m_pEmpty  = NULL;
        AllocEmptyString(1024);

        m_pFg[eBlack]        = 0;
        m_pFg[eBlue]         = FOREGROUND_BLUE;
        m_pFg[eGreen]        = FOREGROUND_GREEN;
        m_pFg[eCyan]         = FOREGROUND_BLUE|FOREGROUND_GREEN;
        m_pFg[eRed]          = FOREGROUND_RED;
        m_pFg[eMagenta]      = FOREGROUND_BLUE|FOREGROUND_RED;
        m_pFg[eBrown]        = FOREGROUND_GREEN|FOREGROUND_RED;
        m_pFg[eLightGray]    = FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
        m_pFg[eDarkGray]     = FOREGROUND_INTENSITY;
        m_pFg[eLightBlue]    = FOREGROUND_BLUE|FOREGROUND_INTENSITY;
        m_pFg[eLightGreen]   = FOREGROUND_GREEN|FOREGROUND_INTENSITY;
        m_pFg[eLightCyan]    = FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY;
        m_pFg[eLightRed]     = FOREGROUND_RED|FOREGROUND_INTENSITY;
        m_pFg[eLightMagenta] = FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_INTENSITY;
        m_pFg[eYellow]       = FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY;
        m_pFg[eWhite]        = FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_INTENSITY;

        m_pBg[eBlack]        = 0;
        m_pBg[eBlue]         = BACKGROUND_BLUE;
        m_pBg[eGreen]        = BACKGROUND_GREEN;
        m_pBg[eCyan]         = BACKGROUND_BLUE|BACKGROUND_GREEN;
        m_pBg[eRed]          = BACKGROUND_RED;
        m_pBg[eMagenta]      = BACKGROUND_BLUE|BACKGROUND_RED;
        m_pBg[eBrown]        = BACKGROUND_GREEN|BACKGROUND_RED;
        m_pBg[eLightGray]    = BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_RED;
        m_pBg[eDarkGray]     = BACKGROUND_INTENSITY;
        m_pBg[eLightBlue]    = BACKGROUND_BLUE|BACKGROUND_INTENSITY;
        m_pBg[eLightGreen]   = BACKGROUND_GREEN|BACKGROUND_INTENSITY;
        m_pBg[eLightCyan]    = BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_INTENSITY;
        m_pBg[eLightRed]     = BACKGROUND_RED|BACKGROUND_INTENSITY;
        m_pBg[eLightMagenta] = BACKGROUND_BLUE|BACKGROUND_RED|BACKGROUND_INTENSITY;
        m_pBg[eYellow]       = BACKGROUND_GREEN|BACKGROUND_RED|BACKGROUND_INTENSITY;
        m_pBg[eWhite]        = BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_RED|BACKGROUND_INTENSITY;

        m_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        m_hStdIn  = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(m_hStdIn, ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

        m_pKeys.Push(new stKeyMap(38, eKeyArrowUp),    38);
        m_pKeys.Push(new stKeyMap(40, eKeyArrowDown),  40);
        m_pKeys.Push(new stKeyMap(37, eKeyArrowLeft),  37);
        m_pKeys.Push(new stKeyMap(39, eKeyArrowRight), 39);
        m_pKeys.Push(new stKeyMap(35, eKeyEnd),        35);
        m_pKeys.Push(new stKeyMap(36, eKeyHome),       36);
        m_pKeys.Push(new stKeyMap(33, eKeyPageUp),     33);
        m_pKeys.Push(new stKeyMap(34, eKeyPageDown),   34);

        m_pKeys.Push(new stKeyMap(112, eKeyF1),        112);
        m_pKeys.Push(new stKeyMap(113, eKeyF2),        113);
        m_pKeys.Push(new stKeyMap(114, eKeyF3),        114);
        m_pKeys.Push(new stKeyMap(115, eKeyF4),        115);
        m_pKeys.Push(new stKeyMap(116, eKeyF5),        116);
        m_pKeys.Push(new stKeyMap(117, eKeyF6),        117);
        m_pKeys.Push(new stKeyMap(118, eKeyF7),        118);
        m_pKeys.Push(new stKeyMap(119, eKeyF8),        119);
        m_pKeys.Push(new stKeyMap(120, eKeyF9),        120);
        m_pKeys.Push(new stKeyMap(121, eKeyF10),       121);
        m_pKeys.Push(new stKeyMap(122, eKeyF11),       122);
        m_pKeys.Push(new stKeyMap(123, eKeyF12),       123);
        m_pKeys.Push(new stKeyMap(45,  eKeyInsert),    45);
        m_pKeys.Push(new stKeyMap(46,  eKeyDel),       46);
        m_pKeys.Push(new stKeyMap(9,   eKeyTab),       9);
        m_pKeys.Push(new stKeyMap(8,   eKeyBackSpace), 8);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CConsole() 
    {
        if (m_pText)
        {
            free(m_pText);
            m_pText = NULL;
        }

        if (m_pEmpty)
        {
            free(m_pEmpty);
            m_pEmpty = NULL;
        }

        m_pKeys.Clear();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL SetCursor(tUINT32 i_uX, tUINT32 i_uY)
    {
        COORD l_cCoord = {(SHORT)i_uX, (SHORT)i_uY};
        return SetConsoleCursorPosition(m_hStdOut, l_cCoord);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tUINT32 GetCursorX()
    {
        CONSOLE_SCREEN_BUFFER_INFO l_sCsbi = {0};
        if (GetConsoleScreenBufferInfo(m_hStdOut, &l_sCsbi))
        {
            return l_sCsbi.dwCursorPosition.X;
        }

        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL GetInput(sInput &o_rInput)
    {
        INPUT_RECORD l_stRecord = {};
        DWORD        l_dwRead   = 0;

        GetNumberOfConsoleInputEvents(m_hStdIn, &l_dwRead);

        if (!l_dwRead)
        {
            return FALSE;
        }

        ReadConsoleInput(m_hStdIn, &l_stRecord, 1, &l_dwRead);

        if (!l_dwRead)
        {
            return FALSE;
        }

        if (FOCUS_EVENT == l_stRecord.EventType)
        {
            o_rInput.eEvent = eFocus;
            o_rInput.stFocus.bFocus = l_stRecord.Event.FocusEvent.bSetFocus;
        }
        else if (KEY_EVENT == l_stRecord.EventType)
        {
            o_rInput.eEvent = eKey;
            o_rInput.stKey.bDown     = l_stRecord.Event.KeyEvent.bKeyDown;
            o_rInput.stKey.uControls = l_stRecord.Event.KeyEvent.dwControlKeyState;
            o_rInput.stKey.wCount    = l_stRecord.Event.KeyEvent.wRepeatCount;

            stKeyMap *l_pKey = m_pKeys.Find(l_stRecord.Event.KeyEvent.wVirtualKeyCode);
            if (l_pKey)
            {
                o_rInput.stKey.wKey = l_pKey->wValue;
            }
            else
            {
                o_rInput.stKey.wKey = l_stRecord.Event.KeyEvent.wVirtualKeyCode;
            }
        }
        else if (MOUSE_EVENT == l_stRecord.EventType)
        {
            o_rInput.eEvent = eMouse;
            o_rInput.stMouse.uButton = l_stRecord.Event.MouseEvent.dwButtonState;
            o_rInput.stMouse.uX      = l_stRecord.Event.MouseEvent.dwMousePosition.X;
            o_rInput.stMouse.uY      = l_stRecord.Event.MouseEvent.dwMousePosition.Y;
        }
        else if (WINDOW_BUFFER_SIZE_EVENT == l_stRecord.EventType)
        {
            o_rInput.eEvent = eSize;
            o_rInput.stSize.uW  = l_stRecord.Event.WindowBufferSizeEvent.dwSize.X;
            o_rInput.stSize.uH  = l_stRecord.Event.WindowBufferSizeEvent.dwSize.Y;
        }
        else
        {
            return FALSE;
        }

        return TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tUINT32 GetCursorY()
    {
        CONSOLE_SCREEN_BUFFER_INFO l_sCsbi = {0};
        if (GetConsoleScreenBufferInfo(m_hStdOut, &l_sCsbi))
        {
            return l_sCsbi.dwCursorPosition.Y;
        }

        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tUINT32 GetWidth()
    {
        CONSOLE_SCREEN_BUFFER_INFO l_sCsbi = {0};
        if (GetConsoleScreenBufferInfo(m_hStdOut, &l_sCsbi))
        {
            return l_sCsbi.srWindow.Right - l_sCsbi.srWindow.Left + 1; 
        }
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tUINT32 GetHeight()
    {
        CONSOLE_SCREEN_BUFFER_INFO l_sCsbi = {0};
        if (GetConsoleScreenBufferInfo(m_hStdOut, &l_sCsbi))
        {
            return l_sCsbi.srWindow.Bottom - l_sCsbi.srWindow.Top + 1;
        }
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL SetTextAttr(eColor i_eFg, eColor i_eBg)
    {
        if (    (i_eFg < 0)
             || (i_eFg >= eColorsMax)
             || (i_eBg < 0)
             || (i_eBg >= eColorsMax)
           )
        {
            return FALSE;
        }

        return SetConsoleTextAttribute(m_hStdOut, m_pFg[i_eFg] | m_pBg[i_eBg]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL Clear() 
    {
        COORD l_sCoordScreen   = { 0, 0 };    // home for the cursor 
        DWORD l_dwCharsWritten = 0;
        DWORD l_dwConSize      = 0;
        CONSOLE_SCREEN_BUFFER_INFO l_sCsbi = {0}; 

        // Get the number of character cells in the current buffer. 
        if( !GetConsoleScreenBufferInfo( m_hStdOut, &l_sCsbi ))
        {
           return FALSE;
        }

        l_dwConSize = l_sCsbi.dwSize.X * l_sCsbi.dwSize.Y;

        // Fill the entire screen with blanks.
        if( !FillConsoleOutputCharacterW( m_hStdOut,        // Handle to console screen buffer 
                                         L' ',              // Character to write to the buffer
                                         l_dwConSize,         // Number of cells to write 
                                         l_sCoordScreen,       // Coordinates of first cell 
                                         &l_dwCharsWritten ))  // Receive number of characters written
        {
           return FALSE;
        }

        // Get the current text attribute.
        if( !GetConsoleScreenBufferInfo( m_hStdOut, &l_sCsbi ))
        {
           return FALSE;
        }

        // Set the buffer's attributes accordingly.
        if( !FillConsoleOutputAttribute( m_hStdOut,         // Handle to console screen buffer 
                                         l_sCsbi.wAttributes, // Character attributes to use
                                         l_dwConSize,        // Number of cells to set attribute 
                                         l_sCoordScreen,      // Coordinates of first cell 
                                         &l_dwCharsWritten )) // Receive number of characters written
        {
           return FALSE;
        }

        // Put the cursor at its home coordinates.
        return SetConsoleCursorPosition( m_hStdOut, l_sCoordScreen );   
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL Clear(tUINT32 i_uX, tUINT32 i_uY, tUINT32 i_uW, tUINT32 i_uH, eColor i_eBg)
    {
        tUINT32 l_uiW = GetWidth();
        tUINT32 l_uiH = GetHeight();

        if (    (i_uX > l_uiW)
             || (i_uY > l_uiH)
             || (!i_uW)
             || (!i_uH)
           )
        {
            return FALSE;
        }

        if ((i_uX + i_uW) > l_uiW)
        {
            i_uW = l_uiW - i_uX;
        }

        if ((i_uY + i_uH) > l_uiH)
        {
            i_uH = l_uiH - i_uY;
        }

        if (i_uW > m_szEmpty)
        {
            if (!AllocEmptyString((size_t)i_uW))
            {
                return FALSE;
            }
        }

        if (!SetTextAttr(eBlack, i_eBg))
        {
            return FALSE;
        }

        tUINT32 l_uiX = GetCursorX();
        tUINT32 l_uiY = GetCursorY();
        tUINT32 l_uiYOld = l_uiY;
        while (i_uH--)
        {
            SetCursor(i_uX, i_uY++);
            WriteTextB(m_pEmpty, i_uW);
        }

        SetCursor(l_uiX, l_uiYOld);

        return TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    size_t WriteTextB(const tXCHAR *i_pText, size_t i_szText)
    {
        DWORD l_dwWritten = 0;
        return WriteConsole(m_hStdOut, i_pText, (DWORD)i_szText, &l_dwWritten, NULL);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    size_t WriteTextB(eColor i_eFg, 
                      eColor i_eBg, 
                      size_t           i_szLenMin, 
                      size_t           i_szLenMax, 
                      size_t           i_szOffset,
                      tXCHAR          *i_pText, 
                      size_t           i_szText,    
                      size_t           i_szTextMax
                     )
    {
        if (    (i_szLenMin > i_szLenMax)
             || (!i_pText)
             || (8 >= i_szTextMax)
             || (i_szText >= i_szTextMax)
           )
        {
            return 0;
        }

        if (!SetTextAttr(i_eFg, i_eBg))
        {
            return 0;
        }

        tXCHAR *l_pText = i_pText; 

        if (!i_szText)
        {
            i_szText = PStrLen(i_pText);
        }

        //if there is offset, recalculate all inputs
        if (i_szOffset)
        {
            if (i_szOffset < i_szText) //set sext offset and recalculate all
            {
                l_pText      = l_pText + i_szOffset;
                i_szText    -= i_szOffset;
                i_szTextMax -= i_szOffset;
                i_szOffset   = 0;
                *l_pText = TM('<');
            }
            else //if (i_szOffset >= i_szText)
            {
                l_pText    = m_pText;
                *l_pText   = TM('<');
                i_szText   = 1;
                i_szTextMax= m_szText;
            }
        }


        if (i_szText > i_szLenMax)
        {
            i_szText = i_szLenMax;
            l_pText[i_szText-1] = TM('>');
        }
        else if (i_szText < i_szLenMin)
        {
            if (i_szTextMax < (i_szLenMin + 1))
            {
                PStrNCpy(m_pText, m_szText, l_pText, i_szText);
                l_pText     = m_pText;
                i_szTextMax = m_szText;
            }

            tXCHAR *l_pIter = l_pText + i_szText;

            while (i_szText < i_szLenMin)
            {
                *l_pIter = TM(' ');
                l_pIter ++;
                i_szText++;
            }
        }

        if (i_szText < i_szTextMax)
        {
            l_pText[i_szText] = 0;
        }
        else
        {
            l_pText[i_szText-1] = 0;
        }

        DWORD l_dwWritten = 0;
        if (!WriteConsole(m_hStdOut, l_pText, (DWORD)i_szText, &l_dwWritten, NULL))
        {
            return 0;
        }

        return (size_t)l_dwWritten;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    size_t WriteTextF(eColor i_eFg, 
                      eColor i_eBg, 
                      size_t           i_szLenMin, 
                      size_t           i_szLenMax, 
                      size_t           i_szOffset,
                      const tXCHAR    *i_pFormat, 
                      va_list          i_pVA
                     )
    {
        int l_iReturn = PVsnprintf(m_pText, m_szText, i_pFormat, i_pVA);

        if (0 > l_iReturn)
        {
            return 0;
        }

        return WriteTextB(i_eFg, i_eBg, i_szLenMin, i_szLenMax, i_szOffset, m_pText, (size_t)l_iReturn, m_szText);
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL Present()
    {
        return TRUE;
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    tBOOL AllocEmptyString(size_t i_szEmpty)
    {
        if (m_pEmpty)
        {
            free(m_pEmpty);
        }

        if (i_szEmpty > 4096)
        {
            i_szEmpty = 4096;
        }

        m_szEmpty = i_szEmpty;
        m_pEmpty  = (tXCHAR*)malloc(m_szEmpty * sizeof(tXCHAR));

        if (!m_pEmpty)
        {
            return FALSE;
        }

        tXCHAR *l_pEmpty = m_pEmpty;
        size_t  l_szIterator = m_szEmpty;

        while (l_szIterator--)
        {
            *l_pEmpty = TM(' ');
            l_pEmpty++;
        }

        return TRUE;
    }
};
