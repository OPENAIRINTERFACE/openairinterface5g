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

// Maxime Guillaud - created Fri May 12 16:20:04 CEST 2006
// Matthias Ihmig, 2013
// see http://www.gnu.org/software/octave/doc/interpreter/Dynamically-Linked-Functions.html#Dynamically-Linked-Functions
// and http://wiki.octave.org/wiki.pl?CodaTutorial
// and http://octave.sourceforge.net/coda/c58.html
// compilation: see Makefile
// Update: Wed May 23 17:25:39 CEST 2007, fifo acquisition of signal buffer (RK)
//         29.01.2013: adopted to new kernel driver


#include <octave/oct.h>

extern "C" {
#include "openair0_lib.h"
}
#include "oarf.h"

#define FCNNAME "oarf_get_frame"

#define TRACE 1

#define FRAME_LENGTH_COMPLEX_SAMPLES   76800

static bool any_bad_argument(const octave_value_list &args)
{
    octave_value v;
    if (args.length()!=1)
    {
        error(FCNNAME);
        error("syntax: oarf_get_frame(card)");
        return true;
    }

    v=args(0);
    if ((!v.is_real_scalar()) || (v.scalar_value() < -2) || (floor(v.scalar_value()) != v.scalar_value()) || (v.scalar_value() >= MAX_CARDS))
    {
        error(FCNNAME);
        error("card must be 0-19 for a specific card, or -1 to get frame from all cards.\nSet framing.sync_mode =SYNCMODE_MASTER for one card and =SYNCMODE_SLAVE to obtain synchronized frames.\n");
        return true;
    }
    return false;
}

int find_mastercard()
{
    int card=0, found_master=0, i;
    const char *str[] = { "FREE", "MASTER", "SLAVE" };
    
    printf("Card configuration: card");
    for (i=0; i<openair0_num_detected_cards; i++)
        printf(" [%i]=%s,", i, str[ openair0_exmimo_pci[i].exmimo_config_ptr->framing.multicard_syncmode ]);
    printf("\n");

    for (i=0; i<openair0_num_detected_cards; i++)
    {
        if ( openair0_exmimo_pci[i].exmimo_config_ptr->framing.multicard_syncmode == SYNCMODE_MASTER )
        {
            if (found_master >= 1)
            {
                printf("Warning: There is more than one master active! Will put card[%i] back into SYNCMODE_FREE!\n", i);
                openair0_exmimo_pci[i].exmimo_config_ptr->framing.multicard_syncmode == SYNCMODE_FREE;
            }
            else
            {
                card = i;
                found_master++;
            }
        }
    }
    
    if (found_master == 0)
        printf("Warning: No master activated! Will trigger getframe on card 0\n");
    
    return card;
}

DEFUN_DLD (oarf_get_frame, args, nargout,"Get frame")
{
    int numant, no_getframe_ioctl=0;
    
    if (any_bad_argument(args))
        return octave_value_list();
       
    int card = args(0).int_value();
    
    octave_value returnvalue;
    int i,aa,j;
    short *rx_sig[MAX_CARDS * MAX_ANTENNAS];
    int ret;
    int frame_length_samples[MAX_CARDS];

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
        return octave_value(ret);
    }
    
    if (card == -2)
    {
        no_getframe_ioctl = 1;
        card = -1;
    }
    
    if (card <-1 || card >= openair0_num_detected_cards) {
        error("card number must be between 0 and %d. Or -1 for all cards.", openair0_num_detected_cards-1);
	return octave_value(-4);
    }

    if (card == -1) {
        numant = openair0_num_detected_cards * openair0_num_antennas[0];
        card = find_mastercard();
    }
    else
        numant = openair0_num_antennas[card];
    
    if ( no_getframe_ioctl )
    {
        // print mbox counters
        for (i=0; i<numant; i++)
        {
            
            if ( numant == openair0_num_antennas[card] )
                printf("rx_cnt[%d] = %d, tx_cnt[%d] = %d,    ", i, *openair0_exmimo_pci[card].rxcnt_ptr[i],
                                                                i, *openair0_exmimo_pci[card].txcnt_ptr[i]);
            else
                printf("rx_cnt[%d] = %d, tx_cnt[%d] = %d,    ", i, *openair0_exmimo_pci[i / (int)openair0_num_antennas[0]].rxcnt_ptr[i % openair0_num_antennas[0]],
                                                                i, *openair0_exmimo_pci[i / (int)openair0_num_antennas[0]].txcnt_ptr[i % openair0_num_antennas[0]]);
        }
        printf("\n");
    }
  
  for (i=0; i<openair0_num_detected_cards; i++)
  { 
    if (openair0_exmimo_pci[i].exmimo_config_ptr->framing.resampling_factor[0] == 2)
      frame_length_samples[i] = FRAME_LENGTH_COMPLEX_SAMPLES;
    else if (openair0_exmimo_pci[i].exmimo_config_ptr->framing.resampling_factor[0] == 1)
      frame_length_samples[i] = FRAME_LENGTH_COMPLEX_SAMPLES*2;
    else if (openair0_exmimo_pci[i].exmimo_config_ptr->framing.resampling_factor[0] == 0)
      frame_length_samples[i] = FRAME_LENGTH_COMPLEX_SAMPLES*4;
    else
      frame_length_samples[i] = FRAME_LENGTH_COMPLEX_SAMPLES;
  }
    printf("Info : Only resampling_factor of channel 0 is taken into account for copying received frame for all the other chains\n");

    
    ComplexMatrix dx (FRAME_LENGTH_COMPLEX_SAMPLES*4, MAX_ANTENNAS*openair0_num_detected_cards);
    /*
    // set the tx buffer to 0x00010001 to put switch in rx mode
    for (aa=0; aa<numant; aa++) 
      for (i=0; i<FRAME_LENGTH_COMPLEX_SAMPLES; i++) 
	((int*)(openair0_exmimo_pci[card].dac_head[aa]))[i] = 0x00010001;
    */
    if ((openair0_exmimo_pci[card].exmimo_config_ptr->framing.tdd_config & TXRXSWITCH_MASK) != TXRXSWITCH_TESTRX)
      printf("Warning: tdd_config is not set to TXRXSWITCH_TESTRX! You better know what you are doing! :)\n");

    // assign userspace pointers
  
    for (j=0; j<openair0_num_detected_cards;j++)
    {
      for (i=0; i<MAX_ANTENNAS; i++)
      {
          //if ( numant == openair0_num_antennas[j] )
              rx_sig[i+(j*MAX_ANTENNAS)] = (short*) openair0_exmimo_pci[ j ].adc_head[ i ];
          //else
          //    rx_sig[i+(j*MAX_ANTENNAS)] = (short*) openair0_exmimo_pci[ i / (int)openair0_num_antennas[0] ].adc_head[i % openair0_num_antennas[0]];
            
          //  printf("Card %i adc_head[%i] = %p \n",j, i, rx_sig[i+(j*MAX_ANTENNAS)]);
      }
    }

    //  msg("Getting buffer...\n");
    if ( no_getframe_ioctl == 0)
        openair0_get_frame(card);

  for (j=0; j<openair0_num_detected_cards;j++)
    for (i=0; i<frame_length_samples[j]; i++) 
        for (aa=0; aa<MAX_ANTENNAS; aa++) 
            dx(i, aa+j*MAX_ANTENNAS) = Complex( rx_sig[aa+j*MAX_ANTENNAS][i*2], rx_sig[aa+j*MAX_ANTENNAS][i*2+1] );
    
    openair0_close();

    return octave_value (dx);
}


