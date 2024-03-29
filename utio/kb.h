// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// kb.h
//

#ifndef KB_H_70C9E93B05A3B0D527D942E351FEEE4B
#define KB_H_70C9E93B05A3B0D527D942E351FEEE4B

#include "ti.h"
#include "termios.h"
#include <bitset>

namespace utio {

using namespace std;

/// \class CKeyboard kb.h utio/kb.h
///
/// \brief Keyboard code preprocessor for terminals.
///
/// Takes raw codes from the input port and translates them into keycode events.
///
class CKeyboard {
public:
    typedef bitset<mksbit_Last>		metastate_t;
    typedef CTerminfo::keystrings_t	keymap_t;
public:
			CKeyboard (void);
		       ~CKeyboard (void);
    void		Open (const CTerminfo& rti);
    void		Close (void);
    void		EnterUIMode (void);
    void		LeaveUIMode (void);
    void		LoadKeymap (const CTerminfo& rti);

    // Return the size of the matching sequence
    size_t		DecodeKey   
     (const char* inBuf, 
      size_t inBufSize,
      wchar_t& kv, 
      metastate_t& kf
      ) const;

    void		SetKeyboardEntry (uint8_t table, uint8_t keycode, uint16_t value, uint16_t* oldValue = NULL);
    inline bool		IsInUIMode (void) const	{ return (m_bTermInUIMode); }
#if UTIO_WANT_GETKEY
    wchar_t		GetKey (metastate_t* pMeta = NULL, bool bBlock = true) const;
    bool		WaitForKeyData (long timeout = 0) const;
private:
    void		ReadKeyData (void) const;
#endif
private:
    keymap_t		m_Keymap;
#if UTIO_WANT_GETKEY
    mutable string	m_Keydata;
#endif
    struct termios	m_InitialTermios;
    uint16_t		m_KbeScrollForward;
    uint16_t		m_KbeScrollBackward;
    bool		m_bTermInUIMode;
};

} // namespace utio

#endif


