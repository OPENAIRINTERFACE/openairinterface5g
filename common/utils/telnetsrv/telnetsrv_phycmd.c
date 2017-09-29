#define _GNU_SOURCE 
#include <string.h>
#include <pthread.h>


#define TELNETSERVERCODE
#include "telnetsrv.h"
#define TELNETSRV_PHYCMD_MAIN
#include "telnetsrv_phycmd.h"
char *prnbuff;
extern int dump_eNB_stats(PHY_VARS_eNB *eNB, char* buffer, int length);

void init_phytelnet()
{
   if (PHY_vars_eNB_g != NULL)
      printf("init_phytelnet: phy var at 0x%08lx\n",(unsigned long int)PHY_vars_eNB_g);
   else 
      fprintf(stderr,"init_phytelnet: phy var not found...\n");



phy_vardef[TELNETVAR_PHYCC0].varvalptr  = &(PHY_vars_eNB_g[0][0]);
phy_vardef[TELNETVAR_PHYCC1].varvalptr  = &(PHY_vars_eNB_g[0][1]);
prnbuff=malloc(get_phybsize() );
if (prnbuff == NULL)
   {
   fprintf(stderr,"Error %s on malloc in init_phytelnet()\n",strerror(errno));
   }
}

void display_uestatshead( telnet_printfunc_t prnt)
{
prnt("cc  ue  rnti Dmcs Umcs tao  tau   Dbr  Dtb   \n");
}

void dump_uestats(int debug, telnet_printfunc_t prnt, uint8_t prntflag)
{

int p;

        p=dump_eNB_stats(PHY_vars_eNB_g[0][0], prnbuff, 0);
	if(prntflag>=1)
	   prnt("%s\n",prnbuff);
	if(debug>=1)
	   prnt("%i bytes printed\n",p);	   


}

void display_uestats(int debug, telnet_printfunc_t prnt, int ue)
{
   for (int cc=0; cc<1 ; cc++)
       {
 
       if ((PHY_vars_eNB_g[0][cc]->dlsch[ue][0]->rnti>0)&&
          (PHY_vars_eNB_g[0][cc]->UE_stats[ue].mode == PUSCH))
          {
          prnt("%02i %04i %04hx %04i %04i %04i %-04i %04i %06i\n",cc, ue,
               PHY_vars_eNB_g[0][cc]->UE_stats[ue].crnti,
	       PHY_vars_eNB_g[0][cc]->dlsch[ue][0]->harq_processes[0]->mcs,0,
//	       PHY_vars_eNB_g[0][cc]->ulsch[ue]->harq_processes[0]->mcs,
	       PHY_vars_eNB_g[0][cc]->UE_stats[ue].UE_timing_offset,
	       PHY_vars_eNB_g[0][cc]->UE_stats[ue].timing_advance_update,
	       PHY_vars_eNB_g[0][cc]->UE_stats[ue].dlsch_bitrate/1000,
	       PHY_vars_eNB_g[0][cc]->UE_stats[ue].total_TBS/1000
              );
	  }
       }
}

void display_phycounters(char *buf, int debug, telnet_printfunc_t prnt)
{    
   prnt("  DLSCH kb      DLSCH kb/s\n");

   dump_uestats(debug, prnt,0);
   prnt("  %09i     %06i\n",
       PHY_vars_eNB_g[0][0]->total_transmitted_bits/1000,
       PHY_vars_eNB_g[0][0]->total_dlsch_bitrate/1000);
}

int dump_phyvars(char *buf, int debug, telnet_printfunc_t prnt)
{
   
   

   if (debug > 0)
       prnt("phy interface module received %s\n",buf);
   if (strcasestr(buf,"phycnt") != NULL)
       {
       display_phycounters(buf, debug, prnt);
       }
   if (strcasestr(buf,"uestat") != NULL)
      {
      char *cptr=strcasestr(buf+sizeof("uestat"),"UE");
      display_uestatshead(prnt);
      if (cptr != NULL)
         {
	 int ueidx = strtol( cptr+sizeof("UE"), NULL, 10);
	 if (ueidx < NUMBER_OF_UE_MAX && ueidx >= 0)
	    {
	    display_uestats(debug, prnt,ueidx);
	    }
	 } /* if cptr != NULL */
      else
         {
	 for (int ue=0; ue<NUMBER_OF_UE_MAX ; ue++)
	     {
	     display_uestats(debug, prnt,ue);
	     }
	 } /* else cptr != NULL */
      } /* uestat */
   if (strcasestr(buf,"uedump") != NULL)
       {
       dump_uestats(debug, prnt,1);
       }      
   return 0;
}



telnetshell_cmddef_t phy_cmdarray[] = {
   {"disp","[phycnt,uedump,uestat UE<x>]", dump_phyvars},

   {"","",NULL},
};


/*-------------------------------------------------------------------------------------*/
void add_phy_cmds()
{

   init_phytelnet();
   add_telnetcmd("phy", phy_vardef, phy_cmdarray);
}
