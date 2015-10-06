/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*! \file oml.h
* \brief Data structure for OCG of OpenAir emulator
* \author N. Nikaein and A. Hafsaoui
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/


#ifndef __OML_H__
#define __OML_H__


#include <math.h>
#include <stdlib.h>
#include  "../OTG/otg.h"

#define PI 3.14159265


/*! \fn void set_taus_seed(unsigned int seed_type);
* \brief initialize seeds used for the generation of taus random values
* \param[in] initial value
* \param[out]
* \note
* @ingroup  _oml
*/
void set_taus_seed(unsigned int seed_type);

/*! \fn inline unsigned int taus(unsigned int comp);
* \brief compute random number
* \param[in] integer
* \param[out]
* \note
* @ingroup  _oml
*/
unsigned int taus(unsigned int comp);


/*! \fn void init_seeds(int seed);
* \brief init values for wichman_hill algo
* \param[in] seed
* \param[out]
* \note
* @ingroup  _oml
*/
void init_seeds(int seed);

/*! \fn double wichman_hill() ;
* \brief generates uniform random number with wichman_hill algo
* \param[in]
* \param[out] random number: wichman_hill
* \note
* @ingroup  _oml
*/
double wichman_hill(void) ;

/*! \fn double uniform_rng();
* \brief generates uniform random number with algo: wichman_hill / random() / Taus
* \param[in]
* \param[out] random number
* \note
* @ingroup  _oml
*/
double uniform_rng(void);

/*! \fn double uniform_dist(double min, double max);
* \brief
* \param[in] double min
* \param[in] double max
* \param[out] uniform number
* \note
* @ingroup  _oml
*/
double uniform_dist(int min, int max);

/*! \fn double gaussian_dist(double mean, double std_dev);
* \brief
* \param[in] double mean
* \param[in] double standard deviation
* \param[out] exponential gaussian number
* \note
* @ingroup  _oml
*/
double gaussian_dist(double mean, double std_dev);

/*! \fn double exponential_dist(double lambda);
* \brief
* \param[in] double lambda
* \param[out] exponential random number
* \note
* @ingroup  _oml
*/

double exponential_dist(double lambda);

/*! \fn double poisson_dist(double lambda);
* \brief generates random numbers for the poisson distribution
* \param[in] lambda used for poisson distrib configuration
* \param[out] poisson random number
* \note
* @ingroup  _oml
*/
double poisson_dist(double lambda);

/*! \fn double weibull_dist(double scale, double shape);
* \brief generates random numbers for the Weibull distribution with scale parameter, and shape parameter.
* \param[in] scale parameter, and shape parameter.
* \param[out] weibull random number
* \note Formula (http://www.xycoon.com/wei_random.htm)
* @ingroup  _oml
*/
double weibull_dist(double scale, double shape);

/*! \fn double pareto_dist(double scale, double shape);
* \brief enerates random numbers for the pareto distribution with scale parameter, and shape parameter.
* \param[in] double scale
* \param[in] double shape
* \param[out] pareto random number
* \note Formula (http://www.xycoon.com/par_random.htm)
* @ingroup  _oml
*/
double pareto_dist(double scale, double shape);

/*! \fn double gamma_dist(double scale, double shape);
* \brief generates random numbers for the gamma distribution with scale parameter, and shape parameter.
* \param[in] double scale
* \param[in] double shape
* \param[out] gamma random number
* \note  Formula (http://www.xycoon.com/gamma_random.htm)
* @ingroup  _oml
*/
double gamma_dist(double scale, double shape);

/*! \fn double cauchy_dist(double scale, double shape);
* \brief generates random numbers for the cauchy distribution with scale parameter, and shape parameter.
* \param[in] double scale
* \param[in] double shape
* \param[out] cauchy random number
* \note Formula(http://www.xycoon.com/nor_relationships3.htm)
* @ingroup  _oml
*/
double cauchy_dist(double scale, double shape);


/*! \fn double double lognormal_dist(double mean, double std_dev)
* \brief generates random numbers for the log normal distribution with mean parameter and standard deviation parameter.
* \param[in] double mean
* \param[in] double std_dev
* \param[out] lognormal random number
* \note Formula(http://www.xycoon.com/nor_relationships3.htm)
* @ingroup  _oml
*/
double lognormal_dist(double mean, double std_dev);


#endif

