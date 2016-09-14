/*________________________rrc_register.h________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/

#ifndef __MAC_RRC_REGISTER_H__
#    define __MAC_RRC_REGISTER_H__
#include "COMMON/mac_rrc_primitives.h"
/** @defgroup _mac_impl_register RRC Registration interface
 * @ingroup _mac_impl_
 *@{
 */
MAC_RLC_XFACE* mac_rrc_register(RRC_XFACE* RRC_xface);
int mac_rrc_unregister(RRC_XFACE *RRC_xface);
#endif
