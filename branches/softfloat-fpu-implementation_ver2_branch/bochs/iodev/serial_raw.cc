/////////////////////////////////////////////////////////////////////////
// $Id: serial_raw.cc,v 1.6 2004-01-18 11:58:07 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2004  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//

// Define BX_PLUGGABLE in files that can be compiled into plugins.  For
// platforms that require a special tag on exported symbols, BX_PLUGGABLE 
// is used to know when we are exporting symbols and when we are importing.
#define BX_PLUGGABLE

#include "bochs.h"

#if USE_RAW_SERIAL

#define LOG_THIS bx_devices.pluginSerialDevice->

serial_raw::serial_raw (char *devname)
{
  put ("SERR");
  settype (SERRLOG);
}

serial_raw::~serial_raw (void)
{
  // nothing here yet
}

void 
serial_raw::set_baudrate (int rate)
{
  BX_DEBUG (("set_baudrate %d", rate));
}

void 
serial_raw::set_data_bits (int val)
{
  BX_DEBUG (("set data bits (%d)", val));
}

void 
serial_raw::set_stop_bits (int val)
{
  BX_DEBUG (("set stop bits (%d)", val));
}

void 
serial_raw::set_parity_mode (int x, int y)
{
  BX_DEBUG (("set parity %d %d", x, y));
}

void 
serial_raw::transmit (int val)
{
  BX_DEBUG (("transmit %d", val));
}

void 
serial_raw::send_hangup ()
{
  BX_DEBUG (("send_hangup"));
}

int 
serial_raw::ready_transmit ()
{
  BX_DEBUG (("ready_transmit returning 1"));
  return 1;
}

int 
serial_raw::ready_receive ()
{
  BX_DEBUG (("ready_receive returning 0"));
  return 0;
}

int 
serial_raw::receive ()
{
  BX_DEBUG (("receive returning 0"));
  return (int)'A';
}

#endif