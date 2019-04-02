/*
  This file is part of the MKR GSM library.
  Copyright (C) 2017  Arduino AG (http://www.arduino.cc/)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <time.h>

#include "Modem.h"

#include "GSM.h"

enum
{
  READY_STATE_CHECK_SIM,
  READY_STATE_WAIT_CHECK_SIM_RESPONSE,
  READY_STATE_UNLOCK_SIM,
  READY_STATE_WAIT_UNLOCK_SIM_RESPONSE,
  READY_STATE_SET_PREFERRED_MESSAGE_FORMAT,
  READY_STATE_WAIT_SET_PREFERRED_MESSAGE_FORMAT_RESPONSE,
  READY_STATE_SET_HEX_MODE,
  READY_STATE_WAIT_SET_HEX_MODE,
  READY_STATE_SET_AUTOMATIC_TIME_ZONE,
  READY_STATE_WAIT_SET_AUTOMATIC_TIME_ZONE_RESPONSE,
  READY_STATE_ENABLE_DTMF_DETECTION,
  READY_STATE_WAIT_ENABLE_DTMF_DETECTION_RESPONSE,
  READY_STATE_CHECK_REGISTRATION,
  READY_STATE_WAIT_CHECK_REGISTRATION_RESPONSE,
  READY_STATE_SET_REPORTING_CALL_STATUS,
  READY_STATE_WAIT_SET_REPORTING_CALL_STATUS,
  READY_STATE_DONE
};

GSM::GSM() : _state(ERROR),
             _readyState(0),
             _pin(NULL),
             _timeout(0)
{
}

GSM3_NetworkStatus_t GSM::begin(const char *pin, bool restart, bool synchronous)
{
  MODEM.begin(restart);
  _state = IDLE;
  return _state;
}

int GSM::isAccessAlive()
{
  String response;
  MODEM.send("AT+CREG?");
  if (MODEM.waitForResponse(100, &response) == 1)
  {
    int status = response.charAt(response.length() - 1) - '0';
    if (status == 1 || status == 5 || status == 8)
    {
      return 1;
    }
  }
  return 0;
}

int GSM::ready()
{
  if (_state == ERROR)
  {
    return 2;
  }
  int ready = MODEM.ready();
  if (ready == 0)
  {
    return 0;
  }

  switch (_readyState)
  {
  case READY_STATE_CHECK_SIM:
  {
    MODEM.setResponseDataStorage(&_response);
    MODEM.send("AT+CPIN?");
    _readyState = READY_STATE_WAIT_CHECK_SIM_RESPONSE;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_CHECK_SIM_RESPONSE:
  {
    if (ready > 1)
    {
      // error => retry
      _readyState = READY_STATE_CHECK_SIM;
      ready = 0;
    }
    else
    {
      if (_response.endsWith("READY"))
      {
        _readyState = READY_STATE_SET_PREFERRED_MESSAGE_FORMAT;
        ready = 0;
      }
      else if (_response.endsWith("SIM PIN"))
      {
        _readyState = READY_STATE_UNLOCK_SIM;
        ready = 0;
      }
      else
      {
        _state = ERROR;
        ready = 2;
      }
    }
    break;
  }

  case READY_STATE_UNLOCK_SIM:
  {
    if (_pin != NULL)
    {
      MODEM.setResponseDataStorage(&_response);
      MODEM.sendf("AT+CPIN=\"%s\"", _pin);
      _readyState = READY_STATE_WAIT_UNLOCK_SIM_RESPONSE;
      ready = 0;
    }
    else
    {
      _state = ERROR;
      ready = 2;
    }
    break;
  }

  case READY_STATE_WAIT_UNLOCK_SIM_RESPONSE:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      _readyState = READY_STATE_SET_PREFERRED_MESSAGE_FORMAT;
      ready = 0;
    }
    break;
  }

  case READY_STATE_SET_PREFERRED_MESSAGE_FORMAT:
  {
    MODEM.send("AT+CMGF=1"); //TODO
    _readyState = READY_STATE_WAIT_SET_PREFERRED_MESSAGE_FORMAT_RESPONSE;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_SET_PREFERRED_MESSAGE_FORMAT_RESPONSE:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      _readyState = READY_STATE_SET_HEX_MODE;
      ready = 0;
    }
    break;
  }

  case READY_STATE_SET_HEX_MODE:
  {
    MODEM.send("AT+UDCONF=1,1"); //TODO
    _readyState = READY_STATE_WAIT_SET_HEX_MODE;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_SET_HEX_MODE:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      _readyState = READY_STATE_SET_AUTOMATIC_TIME_ZONE;
      ready = 0;
    }
    break;
  }

  case READY_STATE_SET_AUTOMATIC_TIME_ZONE:
  {
    MODEM.send("AT+CTZU=1"); //TODO
    _readyState = READY_STATE_WAIT_SET_AUTOMATIC_TIME_ZONE_RESPONSE;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_SET_AUTOMATIC_TIME_ZONE_RESPONSE:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      _readyState = READY_STATE_ENABLE_DTMF_DETECTION;
      ready = 0;
    }
    break;
  }

  case READY_STATE_ENABLE_DTMF_DETECTION:
  {
    MODEM.send("AT+UDTMFD=1,2"); //TODO
    _readyState = READY_STATE_WAIT_ENABLE_DTMF_DETECTION_RESPONSE;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_ENABLE_DTMF_DETECTION_RESPONSE:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      _readyState = READY_STATE_CHECK_REGISTRATION;
      ready = 0;
    }
    break;
  }

  case READY_STATE_CHECK_REGISTRATION:
  {
    MODEM.setResponseDataStorage(&_response);
    MODEM.send("AT+CREG?");
    _readyState = READY_STATE_WAIT_CHECK_REGISTRATION_RESPONSE;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_CHECK_REGISTRATION_RESPONSE:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      int status = _response.charAt(_response.length() - 1) - '0';

      if (status == 0 || status == 4)
      {
        _readyState = READY_STATE_CHECK_REGISTRATION;
        ready = 0;
      }
      else if (status == 1 || status == 5 || status == 8)
      {
        _readyState = READY_STATE_SET_REPORTING_CALL_STATUS;
        _state = GSM_READY;
        ready = 0;
      }
      else if (status == 2)
      {
        _readyState = READY_STATE_CHECK_REGISTRATION;
        _state = CONNECTING;
        ready = 0;
      }
      else if (status == 3)
      {
        _state = ERROR;
        ready = 2;
      }
    }
    break;
  }

  case READY_STATE_SET_REPORTING_CALL_STATUS:
  {
    MODEM.send("AT+UCALLSTAT=1");
    _readyState = READY_STATE_WAIT_SET_REPORTING_CALL_STATUS;
    ready = 0;
    break;
  }

  case READY_STATE_WAIT_SET_REPORTING_CALL_STATUS:
  {
    if (ready > 1)
    {
      _state = ERROR;
      ready = 2;
    }
    else
    {
      _readyState = READY_STATE_DONE;
      ready = 1;
    }
    break;
  }

  case READY_STATE_DONE:
    break;
  }

  return ready;
}

void GSM::setTimeout(unsigned long timeout)
{
  _timeout = timeout;
}

GSM3_NetworkStatus_t GSM::status()
{
  return _state;
}

void GSM::getVersion(String &s)
{
  MODEM.send("ATI");
  //TODO parse
}