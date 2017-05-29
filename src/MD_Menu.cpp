// Implementation file for MD_Menu library
//
// See the main header file MD_Menu.h for more information

#include <MD_Menu.h>
#include <MD_Menu_lib.h>

/**
 * \file
 * \brief Main code file for MD_Menu library
 */
MD_Menu::MD_Menu(cbUserNav cbNav, cbUserDisplay cbDisp,
                mnuHeader_t *mnuHdr, uint8_t mnuHdrCount,
                mnuItem_t *mnuItm, uint8_t mnuItmCount,
                mnuInput_t *mnuInp, uint8_t mnuInpCount) :
                _inMenu(false), _currMenu(0), _inEdit(false),
                _mnuHdr(mnuHdr), _mnuHdrCount(mnuHdrCount),
                _mnuItm(mnuItm), _mnuItmCount(mnuItmCount),
                _mnuInp(mnuInp), _mnuInpCount(mnuInpCount)
{
  setUserNavCallback(cbNav);
  setUserDisplayCallback(cbDisp);
  setMenuWrap(false);
}

void MD_Menu::loadMenu(mnuId_t id)
// Load a menu header definition to the current stack position
{
  mnuId_t idx = 0;
  mnuHeader_t mh;

  if (id != -1)   // look for a menu with that id and load it up
  {
    for (uint8_t i = 0; i < _mnuHdrCount; i++)
    {
      memcpy_P(&mh, &_mnuHdr[i], sizeof(mnuHeader_t));
      if (mh.id == id)
      {
        idx = i;  // found it!
        break;
      }
    }
  }

  // we either found the item or we will load the first one by default
  memcpy_P(&_mnuStack[_currMenu], &_mnuHdr[idx], sizeof(mnuHeader_t));
}

MD_Menu::mnuItem_t* MD_Menu::loadItem(mnuId_t id)
// Find a copy the input item to the class private buffer
{
  for (uint8_t i = 0; i < _mnuItmCount; i++)
  {
    memcpy_P(&_mnuBufItem, &_mnuItm[i], sizeof(mnuItem_t));
    if (_mnuBufItem.id == id)
      return(&_mnuBufItem);
  }

  return(nullptr);
}

MD_Menu::mnuInput_t* MD_Menu::loadInput(mnuId_t id)
// Find a copy the input item to the class private buffer
{
  for (uint8_t i = 0; i < _mnuItmCount; i++)
  {
    memcpy_P(&_mnuBufInput, &_mnuInp[i], sizeof(mnuInput_t));
    if (_mnuBufInput.id == id)
      return(&_mnuBufInput);
  }

  return(nullptr);
}

uint8_t MD_Menu::listCount(PROGMEM char *p)
// Return a count of the items in the list
{
  uint8_t count = 0;
  char c;

  if (p != nullptr)
  {
    if (pgm_read_byte(p) != '\0')   // not empty list
    {
      do
      {
        c = pgm_read_byte(p++);
        if (c == LIST_SEPARATOR) count++;
      } while (c != '\0');

      // if the list is not empty, then the last element is 
      // terminated by '\0' and we have not counted it, so 
      // add it now
      count++;
    }
  }

  return(count);
}

char *MD_Menu::listItem(PROGMEM char *p, uint8_t idx, char *buf, uint8_t bufLen)
// Find the idx'th item in the list and return in fixed width, padded
// with trailing spaces. 
{
  // fill the buffer with '\0' so we know that string will
  // always be terminted within this buffer
  memset(buf, '\0', bufLen);

  if (p != nullptr)
  {
    char *psz;
    char c;

    // skip items before the one we want
    while (idx > 0)
    {
      do
        c = pgm_read_byte(p++);
      while (c != '\0' && c != LIST_SEPARATOR);
      idx--;
    }

    // copy the next item over
    psz = buf;
    for (uint8_t i = 0; i < bufLen - 1; psz++, i++)
    {
      *psz = pgm_read_byte(p++);
      if (*psz == LIST_SEPARATOR) *psz = '\0';
      if (*psz == '\0') break;
    }

    // Pad out any short string with trailing spaces
    // The trailing buffer is already filled with '\0'
    while (strlen(buf) < bufLen - 1)
      *psz++ = ' ';
  }

  return(buf);
}

void MD_Menu::strPreamble(char *psz, mnuInput_t *mInp)
// Create the start to a variable CB_DISP
{
  strcpy(psz, mInp->label);
  strcat(psz, FLD_PROMPT);
  strcat(psz, FLD_DELIM_L);
}

void MD_Menu::strPostamble(char *psz, mnuInput_t *mInp)
// Attach the tail of the variable CB_DISP
{
  strcat(psz, FLD_DELIM_R);
}

bool MD_Menu::processList(userNavAction_t nav, mnuInput_t *mInp)
// Processing for List based input
// Return true when the edit cycle is completed
{
  bool endFlag = false;
  bool update = false;

  switch (nav)
  {
  case NAV_NULL:    // this is to initialise the CB_DISP
  {
    uint8_t size = listCount(mInp->pList);

    if (size == 0)
    {
      MD_PRINTS("\nEmpty list selection!");
      endFlag = true;
    }
    else
    {
      _pValue = mInp->cbVR(mInp->id, mInp->idx, true);

      if (_pValue == nullptr)
      {
        MD_PRINTS("\nList cbVR(GET) == NULL!");
        endFlag = true;
      }
      else
      {
        _iValue = *((uint8_t*)_pValue);
        if (_iValue >= size - 1)   // index set incorrectly
          _iValue = 0;
        update = true;
      }
    }
  }
  break;

  case NAV_DEC:
    {
      uint8_t listSize = listCount(mInp->pList);
      if (_iValue > 0)
      {
        _iValue--;
        update = true;
      }
      else if (_iValue == 0 && _wrapMenu)
      {
        _iValue = listSize - 1;
        update = true;
      }
    }
    break;

  case NAV_INC:
    {
      uint8_t listSize = listCount(mInp->pList);

      if (_iValue < listSize - 1)
      {
        _iValue++;
        update = true;
      }
      else if (_iValue == listSize - 1 && _wrapMenu)
      {
        _iValue = 0;
        update = true;
      }
    }
    break;

  case NAV_SEL:
    *((uint8_t*)_pValue) = _iValue;
    mInp->cbVR(mInp->id, mInp->idx, false);
    endFlag = true;
    break;
  }

  if (update)
  {
    char szItem[mInp->fieldWidth + 1];
    char sz[INP_PRE_SIZE(mInp) + sizeof(szItem) + INP_POST_SIZE(mInp) + 1];

    strPreamble(sz, mInp);
    strcat(sz, listItem(mInp->pList, _iValue, szItem, sizeof(szItem)));
    strPostamble(sz, mInp);

    CB_DISP(DISP_L1, sz);
  }

  return(endFlag);
}

bool MD_Menu::processBool(userNavAction_t nav, mnuInput_t *mInp)
// Processing for Boolean (true/false) value input
// Return true when the edit cycle is completed
{
  bool endFlag = false;
  bool update = false;

  switch (nav)
  {
  case NAV_NULL:    // this is to initialise the CB_DISP
    {
      _pValue = mInp->cbVR(mInp->id, mInp->idx, true);

      if (_pValue == nullptr)
      {
        MD_PRINTS("\nBool cbVR(GET) == NULL!");
        endFlag = true;
      }
      else
      {
        _bValue = *((bool *)_pValue);
        update = true;
      }
    }
    break;

  case NAV_INC:
  case NAV_DEC:
    _bValue = !_bValue;
    update = true;
    break;

  case NAV_SEL:
    *((bool *)_pValue) = _bValue;
    mInp->cbVR(mInp->id, mInp->idx, false);
    endFlag = true;
    break;
  }

  if (update)
  {
    char sz[INP_PRE_SIZE(mInp) + strlen(INP_BOOL_T) + INP_POST_SIZE(mInp) + 1];

    strPreamble(sz, mInp);
    strcat(sz, _bValue ? INP_BOOL_T : INP_BOOL_F);
    strPostamble(sz, mInp);

    CB_DISP(DISP_L1, sz);
  }

  return(endFlag);
}

char *ltostr(char *buf, uint8_t bufLen, int32_t v, uint8_t base)
// Convert a long to a string right justified with leading spaces
// in the base specified (up to 16).
{
  char *ptr = buf + bufLen - 1; // the last element of the buffer
  bool sign = (v < 0);
  uint32_t t = 0, res = 0;
  uint32_t value = (sign ? -v : v);

  if (buf == nullptr) return(nullptr);

  *ptr = '\0'; // terminate the string as we will be moving backwards

  // now successively deal with the remainder digit 
  // until we have value == 0
  do
  {
    t = value / base;
    res = value - (base * t);
    if (res < 10)
      *--ptr = '0' + res;
    else if ((res >= 10) && (res < 16))
      *--ptr = 'A' - 10 + res;
    value = t;
  } while (value != 0 && ptr != buf);

  if (ptr != buf)      // if there is still space
  {
    if (sign) *--ptr = '-';  // put in the sign ...
    while (ptr != buf)       // ... and pad with leading spaces
      *--ptr = ' ';
  }
  else if (value != 0 || sign) // insufficient space - show this
      *ptr = INP_NUMERIC_OFLOW;

  return(buf);
}

bool MD_Menu::processInt(userNavAction_t nav, mnuInput_t *mInp, uint16_t incDelta)
// Processing for Integer (all sizes) value input
// Return true when the edit cycle is completed
{
  bool endFlag = false;
  bool update = false;

  switch (nav)
  {
  case NAV_NULL:    // this is to initialise the CB_DISP
    {
      _pValue = mInp->cbVR(mInp->id, mInp->idx, true);

      if (_pValue == nullptr)
      {
        MD_PRINTS("\nInt cbVR(GET) == NULL!");
        endFlag = true;
      }
      else
      {
        switch (mInp->action)
        {
        case INP_INT8:  _iValue = *((int8_t*)_pValue);  break;
        case INP_INT16: _iValue = *((int16_t*)_pValue); break;
        case INP_INT32: _iValue = *((int32_t*)_pValue); break;
        }
        update = true;
      }
    }
    break;

  case NAV_INC:
    if (_iValue + incDelta < mInp->range[1])
      _iValue += incDelta;
    else
      _iValue = mInp->range[1];
    update = true;
    break;

  case NAV_DEC:
    if (_iValue - incDelta > mInp->range[0])
      _iValue -= incDelta;
    else
      _iValue = mInp->range[0];
    update = true;
    break;

  case NAV_SEL:
    switch (mInp->action)
    {
    case INP_INT8:  *((int8_t*)_pValue) = _iValue;  break;
    case INP_INT16: *((int16_t*)_pValue) = _iValue; break;
    case INP_INT32: *((int32_t*)_pValue) = _iValue; break;
    }
    mInp->cbVR(mInp->id, mInp->idx, false);
    endFlag = true;
    break;
  }

  if (update)
  {
    char sz[INP_PRE_SIZE(mInp) + mInp->fieldWidth + INP_POST_SIZE(mInp) + 1];

    strPreamble(sz, mInp);
    ltostr(sz + strlen(sz), mInp->fieldWidth + 1, _iValue, mInp->base);
    strPostamble(sz, mInp);

    CB_DISP(DISP_L1, sz);
  }

  return(endFlag);
}

bool MD_Menu::processRun(userNavAction_t nav, mnuInput_t *mInp)
// Processing for Run user code input field.
// When the field is selected, run the user variable code. For all other
// input do nothing. Return true when the element has run user code.
{
  if (nav == NAV_NULL)    // initialise the CB_DISP
  {
    char sz[INP_PRE_SIZE(mInp) + INP_POST_SIZE(mInp) + 1];

    strcpy(sz, FLD_DELIM_L);
    strcat(sz, mInp->label);
    strcat(sz, FLD_DELIM_R);

    CB_DISP(DISP_L1, sz);
  }
  else if (nav == NAV_SEL)
  {
    mInp->cbVR(mInp->id, mInp->idx, false);
    return(true);
  }

  return(false);
}

void MD_Menu::handleInput(bool bNew)
{
  bool ended = false;
  uint16_t incDelta = 1;
  mnuItem_t *mi;
  mnuInput_t *me;

  if (bNew)
  {
    CB_DISP(DISP_CLEAR, nullptr);
    mi = loadItem(_mnuStack[_currMenu].idItmCurr);
    CB_DISP(DISP_L0, mi->label);
    me = loadInput(mi->actionId);

    switch (me->action)
    {
    case INP_LIST: ended = processList(NAV_NULL, me); break;
    case INP_BOOL: ended = processBool(NAV_NULL, me); break;
    case INP_INT8:
    case INP_INT16:
    case INP_INT32: ended = processInt(NAV_NULL, me, incDelta); break;
    case INP_RUN: ended = processRun(NAV_NULL, me); break;
    }

    _inEdit = true;
  }
  else
  {
    userNavAction_t nav = (_cbNav != nullptr ? _cbNav(incDelta) : NAV_NULL);

    if (nav == NAV_ESC)
      ended = true;
    else if (nav != NAV_NULL)
    {
      mi = loadItem(_mnuStack[_currMenu].idItmCurr);
      me = loadInput(mi->actionId);

      switch (me->action)
      {
      case INP_LIST: ended = processList(nav, me); break;
      case INP_BOOL: ended = processBool(nav, me); break;
      case INP_INT8:
      case INP_INT16:
      case INP_INT32: ended = processInt(nav, me, incDelta); break;
      case INP_RUN: ended = processRun(nav, me); break;
      }
    }
  }

  if (ended)
  {
    _inEdit = false;
    handleMenu(true);
  }
}

void MD_Menu::handleMenu(bool bNew)
{
  bool update = false;

  if (bNew)
  {
    CB_DISP(DISP_CLEAR, nullptr);
    CB_DISP(DISP_L0, _mnuStack[_currMenu].label);
    if (_mnuStack[_currMenu].idItmCurr == 0)
      _mnuStack[_currMenu].idItmCurr = _mnuStack[_currMenu].idItmStart;
    update = true;
  }
  else
  {
    uint16_t incDelta = 1;
    userNavAction_t nav = (_cbNav != nullptr ? _cbNav(incDelta) : NAV_NULL);

    switch (nav)
    {
    case NAV_DEC:
      if (_mnuStack[_currMenu].idItmCurr != _mnuStack[_currMenu].idItmStart)
      {
        _mnuStack[_currMenu].idItmCurr--;
        update = true;
      }
      else if (_wrapMenu)
      {
        _mnuStack[_currMenu].idItmCurr = _mnuStack[_currMenu].idItmEnd;
        update = true;
      }
      break;

    case NAV_INC:
      if (_mnuStack[_currMenu].idItmCurr != _mnuStack[_currMenu].idItmEnd)
      {
        _mnuStack[_currMenu].idItmCurr++;
        update = true;
      }
      else if (_wrapMenu)
      {
        _mnuStack[_currMenu].idItmCurr = _mnuStack[_currMenu].idItmStart;
        update = true;
      }
      break;

    case NAV_SEL:
      {
        mnuItem_t *mi = loadItem(_mnuStack[_currMenu].idItmCurr);

        switch (mi->action)
        {
        case MNU_MENU:
          if (_currMenu < MNU_STACK_SIZE - 1)
          {
            _currMenu++;
            loadMenu(mi->actionId);
            handleMenu(true);
          }
          break;

        case MNU_INPUT:
          if (loadInput(mi->actionId) != nullptr)
            handleInput(true);
          else
            MD_PRINTS("\nInput definition not found");
          break;
        }
      }
      break;

    case NAV_ESC:
      if (_currMenu == 0)
        _inMenu = false;
      else
      {
        _currMenu--;
        handleMenu(true);  // just one level of recursion;
      }
      break;
    }
  }

  if (update) // update L1 on the CB_DISP
  {
    mnuItem_t *mi = loadItem(_mnuStack[_currMenu].idItmCurr);

    if (mi != nullptr)
    {
      char sz[strlen(mi->label) + strlen(MNU_DELIM_L) + strlen(MNU_DELIM_R) + 1]; // temporary string

      strcpy(sz, MNU_DELIM_L);
      strcat(sz, mi->label);
      strcat(sz, MNU_DELIM_R);

      CB_DISP(DISP_L1, sz);
    }
  }
}

bool MD_Menu::runMenu(bool bStart)
{
  // check if we need to process anything
  if (!_inMenu && !bStart)
    return(false);

  _inMenu = true; // we now are running a menu!

  if (bStart)   // start the menu again
  {
    MD_PRINTS("\nrunMenu: Starting menu");
    _currMenu = 0;
    loadMenu();
    handleMenu(true);
  }
  else    // keep running current menu
  {
    if (_inEdit)
      handleInput();
    else
      handleMenu();

    if (!_inMenu)
    {
      CB_DISP(DISP_CLEAR, nullptr);
      MD_PRINTS("\nrunMenu: Ending Menu");
    }
  }

  return(_inMenu);
}
