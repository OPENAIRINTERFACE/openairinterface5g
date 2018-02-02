/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

// Matthias Ihmig, based on code from Maxime Guillaud - created 28.01.2013
// see http://www.gnu.org/software/octave/doc/interpreter/Dynamically-Linked-Functions.html#Dynamically-Linked-Functions
// and http://wiki.octave.org/wiki.pl?CodaTutorial
// and http://octave.sourceforge.net/coda/c58.html
// compilation: see Makefile

#include <octave/oct.h>

#include "pcie_interface.h"
#include "oarf.h"

extern "C" {
#include "openair0_lib.h"
}

#define FCNNAME "oarf_config_exmimo"

#define TRACE 1

static bool any_bad_argument(const octave_value_list &args)
{
  octave_value v,w;
  int i;

  if (args.length()!=17)
    {
      error(FCNNAME);
      error("Wrong number of parameters! Did you add the card number as first parameter?");
      error("syntax: oarf_config_exmimo(card,freqrx,freq_tx,tdd_config,syncmode,rxgain,txgain,eNB_flag,rf_mode,rx_dc,rf_local,rf_vcolocal,rffe_rxg_low,rffe_rxg_final,autocal,resampling_factor)");
      return true;
    }

  v=args(1);
  for (i=0;i<v.columns();i++) {
    if ((real(v.row_vector_value()(i)) < 0.0) || 
	(floor(real(v.row_vector_value()(i))) != real(v.row_vector_value()(i))) || 
	(real(v.row_vector_value()(i)) > 3.9e9))
      {
	error(FCNNAME);
	error("rx freqband %d must be 0-3.9e9",i);
	return true;
      }
  }

  v=args(2);
  for (i=0;i<v.columns();i++) {
    if ((real(v.row_vector_value()(i)) < 0.0) || 
	(floor(real(v.row_vector_value()(i))) != real(v.row_vector_value()(i))) || 
	(real(v.row_vector_value()(i)) > 3.9e9))
      {
	error(FCNNAME);
	error("tx freqband %d must be 0-3.9e9",i);
	return true;
      }
  }

  if ((!args(3).is_real_scalar()))
    {
      error(FCNNAME);
      error("tdd_config must be an integer.");
      return true;
    }

  if ((!args(4).is_real_scalar()))
    {
      error(FCNNAME);
      error("multicard_syncmode must be an integer.");
      return true;
    }

  v=args(5);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((real(args(5).row_vector_value()(i))<0.0) || (real(args(5).row_vector_value()(i))>63.0))
            {
	      error(FCNNAME);
	      error("rx gain must be between 0 and 63. (got %f).",args(5).row_vector_value()(i));
	      return true;
            }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rxgain must be 4 (got %d)\n",v.columns());
  }

  v=args(6);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((real(args(6).row_vector_value()(i))<0.0) || (real(args(6).row_vector_value()(i))>63.0))
            {
	      error(FCNNAME);
	      error("tx gain must be between 0 and 63. (got %f).",args(6).row_vector_value()(i));
	      return true;
            }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for txgain must be 4 (got %d)\n",v.columns());
  }


  if ((!args(7).is_real_scalar()) || (args(7).scalar_value()<0.0) || (args(7).scalar_value()>1)) {
    error(FCNNAME);
    error("eNB_flag must be between 0 and 1 (got %f).",args(7).scalar_value());
    return true;
  }

  v = args(8);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>(double)((uint32_t)(1<<31)))) {
	    error(FCNNAME);
	    error("rf_mode %d must be between 0 and 2^31 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rf_mode must be 4\n");
  }

  v = args(9);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>(double)((uint32_t)(1<<16)))) {
	    error(FCNNAME);
	    error("rx_dc %d must be between 0 and 2^16 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rf_mode must be 4\n");
  }  

  v = args(10);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>(double)((uint32_t)(1<<24)))) {
	    error(FCNNAME);
	    error("rf_local %d must be between 0 and 2^24 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rf_local must be 4\n");
  }

  v = args(11);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>(double)((uint32_t)(1<<12)))) {
	    error(FCNNAME);
	    error("rf_vcocal %d must be between 0 and 2^12 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rf_vcocal must be 4\n");
  }

  v = args(12);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>63.0)) {
	    error(FCNNAME);
	    error("rffe_rxg_low %d must be between 0 and 63 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rffe_rxg_low must be 4\n");
  }

  v = args(13);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>63.0)) {
	    error(FCNNAME);
	    error("rffe_rxg_final %d must be between 0 and 63 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rffe_rxg_final must be 4\n");
  }

  v = args(14);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>8.0)) {
	    error(FCNNAME);
	    error("rffe_band %d must be between 0 and 8 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for rffe_band must be 4\n");
  }

  v = args(15);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
	  if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>2.0)) {
	    error(FCNNAME);
	    error("autocal %d must be 0 or 2 (got %f).",i,v.row_vector_value()(i));
	    return true;
	  }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for autocal must be 4\n");
  }

  v = args(16);
  if (v.columns() == 4)
    {
      for (i=0;i<v.columns();i++)
        {
          if ((v.row_vector_value()(i)<0.0) || (v.row_vector_value()(i)>2.0)) {
            error(FCNNAME);
            error("resampling_factor %d must be 0, 1 or 2 (got %f).",i,v.row_vector_value()(i));
            return true;
          }
        }
    }
  else {
    error(FCNNAME);
    error("number of columns for resampling_factor must be 4\n");
  }


  if ( !args(0).is_real_scalar() )
    {
      error(FCNNAME);
      error("card_id must be scalar (count starting from 0).\nUse card_id = -1 to send command to all cards.\nUse card_id = -2 to update config in memory, but do not send DUMP_CONFIG ioctl.");
      return true;
    }

  return false;
}



DEFUN_DLD (oarf_config_exmimo, args, nargout,"configure the openair interface - returns 0 if successful")
{
  int ret;
  int ant;
  int a,b;
  int no_dump_config = 0;
  if (any_bad_argument(args))
    return octave_value_list();

  int card             = args(0).int_value();
  RowVector freqrx     = args(1).row_vector_value();
  RowVector freqtx     = args(2).row_vector_value();
  const int tdd_config = args(3).int_value();
  const int multicard_syncmode = args(4).int_value();
  RowVector rxgain     = args(5).row_vector_value();
  RowVector txgain     = args(6).row_vector_value();
  const int eNB_flag   = args(7).int_value();
  RowVector rf_mode    = args(8).row_vector_value();
  RowVector rf_dc      = args(9).row_vector_value();
  RowVector rf_local   = args(10).row_vector_value();
  RowVector rf_vcocal  = args(11).row_vector_value();
  RowVector rffe_rxg_low   = args(12).row_vector_value();
  RowVector rffe_rxg_final = args(13).row_vector_value();
  RowVector rffe_band      = args(14).row_vector_value();
  RowVector autocal        = args(15).row_vector_value();
  RowVector resampling_factor = args(16).row_vector_value();
  int rffe_band_int;
    
  exmimo_config_t *p_exmimo_config;
  exmimo_id_t *p_exmimo_id;

  octave_value returnvalue;

  ret = openair0_open();
  if ( ret != 0 )
    {
      error(FCNNAME);
      if (ret == -1)
	error("Error opening /dev/openair0");
      if (ret == -2)
	error("Error mapping bigshm");
      if (ret == -3)
	error("Error mapping RX or TX buffer");
      if (ret == -5)
        error("Error Firmware/Software do not match");
      return octave_value(ret);
    }

  if (card == -2) {
    no_dump_config = 1;
    card = -1;
  }

  if (card <-1 || card >= openair0_num_detected_cards) {
    warning("Invalid card number %d! Must be between 0 and %d. Or -1 for all card. Will assume -1.", openair0_num_detected_cards-1);
    card = -1;
  }

  if (card == -1) {
    a = 0;
    b = openair0_num_detected_cards;
  } else {
    a = card;
    b = card+1;
  }
    
  //printf("Number of cards: %d\n", openair0_num_detected_cards);
    
  for (card = a; card < b; card++)
    {
      p_exmimo_config = openair0_exmimo_pci[card].exmimo_config_ptr;
      p_exmimo_id     = openair0_exmimo_pci[card].exmimo_id_ptr;
            
      p_exmimo_config->framing.eNB_flag   = eNB_flag;
      p_exmimo_config->framing.tdd_config = tdd_config;
      p_exmimo_config->framing.multicard_syncmode = multicard_syncmode;
//      p_exmimo_config->framing.resampling_factor = 2;

      for (ant=0; ant<4; ant++)
        {
	  p_exmimo_config->rf.do_autocal[ant] = autocal(ant);
	  p_exmimo_config->rf.rf_freq_rx[ant] = freqrx(ant);
	  p_exmimo_config->rf.rf_freq_tx[ant] = freqtx(ant);
	  p_exmimo_config->rf.rx_gain[ant][0] = (uint32_t) rxgain(ant);
	  p_exmimo_config->rf.tx_gain[ant][0] = (uint32_t) txgain(ant);
	  p_exmimo_config->rf.rf_mode[ant] = rf_mode(ant);
	  
	  p_exmimo_config->rf.rf_local[ant] = rf_local(ant);
	  p_exmimo_config->rf.rf_rxdc[ant] = rf_dc(ant);
	  p_exmimo_config->rf.rf_vcocal[ant] = rf_vcocal(ant);
	  
	  p_exmimo_config->rf.rffe_gain_txlow[ant] = 31;
	  p_exmimo_config->rf.rffe_gain_txhigh[ant] = 31;
	  p_exmimo_config->rf.rffe_gain_rxfinal[ant] = rffe_rxg_final(ant);
	  p_exmimo_config->rf.rffe_gain_rxlow[ant] = rffe_rxg_low(ant);
          p_exmimo_config->framing.resampling_factor[ant] = resampling_factor(ant); 
	  rffe_band_int = (int) rffe_band(ant);
	  switch (rffe_band_int) {
	  case 0:
	    p_exmimo_config->rf.rffe_band_mode[ant] = TVWS_TDD;
	    break;	    
	  case 1:
	    p_exmimo_config->rf.rffe_band_mode[ant] = DD_FDD;
	    break;	    
	  case 2:
	    p_exmimo_config->rf.rffe_band_mode[ant] = DD_TDD;
	    break;	    
	  case 3:
	    p_exmimo_config->rf.rffe_band_mode[ant] = B19G_TDD;
	    break;	    
	  case 4:
	    p_exmimo_config->rf.rffe_band_mode[ant] = B24G_TDD;
	    break;	    
	  case 5:
	    p_exmimo_config->rf.rffe_band_mode[ant] = B26G_TDD;
	    break;	    
	  case 6:
	    p_exmimo_config->rf.rffe_band_mode[ant] = B26G_FDD;
	    break;	    
	  case 7:
	    p_exmimo_config->rf.rffe_band_mode[ant] = B35G_TDD;
	    break;	    
	  case 8:
	    p_exmimo_config->rf.rffe_band_mode[ant] = B50G_TDD;
	    break;	    
	  default:
	    p_exmimo_config->rf.rffe_band_mode[ant] = TVWS_TDD;
	    break;	    
	  }
	}

      if (no_dump_config == 0)
	returnvalue = openair0_dump_config( card );
        
      if (0)
	printf("Card %d: ExpressMIMO %d, HW Rev. 0x%d, SW Rev 0x%d, SVN Rev %d, Builddate %d,  %d antennas\n", card, p_exmimo_id->board_exmimoversion,
	       p_exmimo_id->board_hwrev, p_exmimo_id->board_swrev,
	       p_exmimo_id->system_id.bitstream_id, p_exmimo_id->system_id.bitstream_build_date,
	       openair0_num_antennas[card]);
    }
    
  openair0_close();

  return octave_value(returnvalue);
}

