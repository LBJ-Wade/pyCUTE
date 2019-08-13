///////////////////////////////////////////////////////////////////////
//                                                                   //
//   Copyright 2012 David Alonso                                     //
//                                                                   //
//                                                                   //
// This file is part of CUTE.                                        //
//                                                                   //
// CUTE is free software: you can redistribute it and/or modify it   //
// under the terms of the GNU General Public License as published by //
// the Free Software Foundation, either version 3 of the License, or //
// (at your option) any later version.                               //
//                                                                   //
// CUTE is distributed in the hope that it will be useful, but       //
// WITHOUT ANY WARRANTY; without even the implied warranty of        //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU //
// General Public License for more details.                          //
//                                                                   //
// You should have received a copy of the GNU General Public License //
// along with CUTE.  If not, see <http://www.gnu.org/licenses/>.     //
//                                                                   //
///////////////////////////////////////////////////////////////////////

/*********************************************************************/
//                      Correlators with OpenMP                      //
/*********************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _HAVE_OMP
#include <omp.h>
#endif //_HAVE_OMP
#include "define.h"
#include "common.h"

static inline int r2bin(double r2)
{
  int ir;

  if(logbin) {
    if(r2>0)
      ir=(int)(n_logint*(0.5*log10(r2)-log_r_max)+nb_r);
    else
      ir=-1;
  }
  else {
    ir=(int)(sqrt(r2)*i_r_max*nb_r);
  }

  return ir;
}

static inline int th2bin(double cth)
{
  int ith;
  cth=(MIN((1.),(cth)));
  
  if(logbin) {
    if(cth!=1) {
#ifdef _TRUE_ACOS
      cth=log10(acos((MIN(1,cth))));
#else //_TRUE_ACOS
      cth=1-MIN(1,cth);
      cth=0.5*log10(2*cth+0.3333333*cth*cth+
		     0.0888888889*cth*cth*cth);
#endif //_TRUE_ACOS
      ith=(int)(n_logint*(cth-log_th_max)+nb_theta);
    }
    else ith=-1;
  }
  else {
#ifdef _TRUE_ACOS
    cth=acos((MIN(1,cth)));
#else //_TRUE_ACOS
    cth=1-MIN(1,cth);
    cth=sqrt(2*cth+0.333333333*cth*cth+
	      0.08888888889*cth*cth*cth);
#endif //_TRUE_ACOS
    ith=(int)(cth*nb_theta*i_theta_max);
  }
  
  return ith;
}

void auto_angular_cross_bf(int npix_full,int *indices,
			   RadialPixel *pixrad,histo_t *hh)
{
  //////
  // Radial cross-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<(nb_red*(nb_red+1)*nb_theta)/2;i++) 
    hh[i]=0;
  
#pragma omp parallel default(none)			\
  shared(npix_full,indices,pixrad,hh,n_side_phi)	\
  shared(nb_red,nb_theta,ipix_0,ipix_f)		\
  shared(i_red_interval,red_0,i_theta_max)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc((nb_red*(nb_red+1)*nb_theta)/2,sizeof(histo_t));
    double cth_aperture=cos(1./i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=pixrad[ip1].np;
      RadialPixelInfo *pi1=pixrad[ip1].pi;
      int *bounds=pi1->bounds;

      for(ii=0;ii<np1;ii++) {
	int jj,icth;
	double *pos1=&(pi1->pos[N_POS*ii]);
	int iz1=(int)((pi1->redshifts[ii]-red_0)*i_red_interval*nb_red);
	if((iz1>=0)&&(iz1<nb_red)) {
	  for(jj=ii+1;jj<np1;jj++) {
	    int iz2=(int)((pi1->redshifts[jj]-red_0)*i_red_interval*nb_red);
	    if((iz2>=0)&&(iz2<nb_red)) {
	      double *pos2=&(pi1->pos[N_POS*jj]);
	      double prod=pos1[0]*pos2[0]+
		pos1[1]*pos2[1]+pos1[2]*pos2[2];
	      if(prod>cth_aperture) {
		int ith=th2bin(prod);
		if((ith<nb_theta)&&(ith>=0)) {
		  int imin=MIN(iz1,iz2);
		  int imax=MAX(iz1,iz2);
		  int index=ith+nb_theta*((imin*(2*nb_red-imin-1))/2+imax);
		  //		  int index=ith+nb_theta*(iz1+nb_red*iz2);
#ifdef _WITH_WEIGHTS
		  hthread[index]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		  hthread[index]++;
#endif //_WITH_WEIGHTS
		}
	      }
	    }
	  }

	  for(icth=bounds[0];icth<=bounds[1];icth++) {
	    int iphi;
	    int icth_n=icth*n_side_phi;
	    for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	      int iphi_true=(iphi+n_side_phi)%n_side_phi;
	      int ip2=iphi_true+icth_n;
	      if(pixrad[ip2].np>0) {
		if(ip2>ip1) {
		  int np2=pixrad[ip2].np;
		  RadialPixelInfo *pi2=pixrad[ip2].pi;
		  for(jj=0;jj<np2;jj++) {
		    int iz2=(int)((pi2->redshifts[jj]-red_0)*i_red_interval*nb_red);
		    if((iz2>=0)&&(iz2<nb_red)) {
		      double *pos2=&(pi2->pos[N_POS*jj]);
		      double prod=pos1[0]*pos2[0]+
			pos1[1]*pos2[1]+pos1[2]*pos2[2];
		      if(prod>cth_aperture) {
			int ith=th2bin(prod);
			if((ith<nb_theta)&&(ith>=0)) {
			  int imin=MIN(iz1,iz2);
			  int imax=MAX(iz1,iz2);
			  int index=ith+nb_theta*((imin*(2*nb_red-imin-1))/2+imax);
			  //			  int index=ith+nb_theta*(iz1+nb_red*iz2);
#ifdef _WITH_WEIGHTS
			  hthread[index]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			  hthread[index]++;
#endif //_WITH_WEIGHTS
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<(nb_red*(nb_red+1)*nb_theta)/2;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_angular_cross_bf(int npix_full,int *indices,
			    RadialPixel *pixrad1,RadialPixel *pixrad2,
			    histo_t *hh)
{
  //////
  // Radial cross-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<(nb_red*(nb_red+1)*nb_theta)/2;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(npix_full,indices,pixrad1,pixrad2,hh,n_side_phi)	\
  shared(nb_red,nb_theta,ipix_0,ipix_f)		\
  shared(i_red_interval,red_0,i_theta_max)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc((nb_red*(nb_red+1)*nb_theta)/2,sizeof(histo_t));
    double cth_aperture=cos(1./i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=pixrad1[ip1].np;
      RadialPixelInfo *pi1=pixrad1[ip1].pi;
      int *bounds=pi1->bounds;

      for(ii=0;ii<np1;ii++) {
	int icth;
	int iz1=(int)((pi1->redshifts[ii]-red_0)*i_red_interval*nb_red);
	if((iz1>=0)&&(iz1<nb_red)) {
	  double *pos1=&(pi1->pos[N_POS*ii]);
	  for(icth=bounds[0];icth<=bounds[1];icth++) {
	    int iphi;
	    int icth_n=icth*n_side_phi;
	    for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	      int iphi_true=(iphi+n_side_phi)%n_side_phi;
	      int ip2=iphi_true+icth_n;
	      if(pixrad2[ip2].np>0) {
		int jj;
		int np2=pixrad2[ip2].np;
		RadialPixelInfo *pi2=pixrad2[ip2].pi;
		for(jj=0;jj<np2;jj++) {
		  int iz2=(int)((pi2->redshifts[jj]-red_0)*i_red_interval*nb_red);
		  if((iz2>=0)&&(iz2<nb_red)) {
		    double *pos2=&(pi2->pos[N_POS*jj]);
		    double prod=pos1[0]*pos2[0]+
		      pos1[1]*pos2[1]+pos1[2]*pos2[2];
		    if(prod>cth_aperture) {
		      int ith=th2bin(prod);
		      if((ith<nb_theta)&&(ith>=0)) {
			int imin=MIN(iz1,iz2);
			int imax=MAX(iz1,iz2);
			int index=ith+nb_theta*((imin*(2*nb_red-imin-1))/2+imax);
			//			int index=ith+nb_theta*(iz1+nb_red*iz2);
#ifdef _WITH_WEIGHTS
			hthread[index]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			hthread[index]++;
#endif //_WITH_WEIGHTS
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<(nb_red*(nb_red+1)*nb_theta)/2;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void auto_full_bf(int npix_full,int *indices,
		  RadialPixel *pixrad,histo_t *hh)
{
  //////
  // Radial cross-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);
  
  for(i=0;i<nb_red*nb_dz*nb_theta;i++) 
    hh[i]=0;
  
#pragma omp parallel default(none)			\
  shared(npix_full,indices,pixrad,hh,n_side_phi)	\
  shared(nb_red,nb_dz,nb_theta,ipix_0,ipix_f)	\
  shared(i_red_interval,red_0,i_dz_max,i_theta_max)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_red*nb_dz*nb_theta,sizeof(histo_t));
    double cth_aperture=cos(1./i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=pixrad[ip1].np;
      RadialPixelInfo *pi1=pixrad[ip1].pi;
      int *bounds=pi1->bounds;

      for(ii=0;ii<np1;ii++) {
	int jj,icth;
	double *pos1=&(pi1->pos[N_POS*ii]);
	double redshift1=pi1->redshifts[ii];

	for(jj=ii+1;jj<np1;jj++) {
	  double *pos2=&(pi1->pos[N_POS*jj]);
	  double prod=pos1[0]*pos2[0]+
	    pos1[1]*pos2[1]+pos1[2]*pos2[2];
	  if(prod>cth_aperture) {
	    double z_mean=0.5*(redshift1+pi1->redshifts[jj]);
	    int iz_mean=(int)((z_mean-red_0)*i_red_interval*nb_red);
	    if((iz_mean>=0)&&(iz_mean<nb_red)) {
	      double dz=fabs(redshift1-pi1->redshifts[jj]);
	      int idz=(int)(dz*i_dz_max*nb_dz);
	      if((idz<nb_dz)&&(idz>=0)) {
		int ith=th2bin(prod);
		if((ith<nb_theta)&&(ith>=0)) {
		  int index=ith+nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
		  hthread[index]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		  hthread[index]++;
#endif //_WITH_WEIGHTS
		}
	      }
	    }
	  }
	}

	for(icth=bounds[0];icth<=bounds[1];icth++) {
	  int iphi;
	  int icth_n=icth*n_side_phi;
	  for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	    int iphi_true=(iphi+n_side_phi)%n_side_phi;
	    int ip2=iphi_true+icth_n;
	    if(pixrad[ip2].np>0) {
	      if(ip2>ip1) {
		int np2=pixrad[ip2].np;
		RadialPixelInfo *pi2=pixrad[ip2].pi;
		for(jj=0;jj<np2;jj++) {
		  double *pos2=&(pi2->pos[N_POS*jj]);
		  double prod=pos1[0]*pos2[0]+
		    pos1[1]*pos2[1]+pos1[2]*pos2[2];
		  if(prod>cth_aperture) {
		    double z_mean=0.5*(redshift1+pi2->redshifts[jj]);
		    int iz_mean=(int)((z_mean-red_0)*i_red_interval*nb_red);
		    if((iz_mean>=0)&&(iz_mean<nb_red)) {
		      double dz=fabs(redshift1-pi2->redshifts[jj]);
		      int idz=(int)(dz*i_dz_max*nb_dz);
		      if((idz<nb_dz)&&(idz>=0)) {
			int ith=th2bin(prod);
			if((ith<nb_theta)&&(ith>=0)) {
			  int index=ith+nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
			  hthread[index]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			  hthread[index]++;
#endif //_WITH_WEIGHTS
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_red*nb_dz*nb_theta;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_full_bf(int npix_full,int *indices,
		   RadialPixel *pixrad1,RadialPixel *pixrad2,
		   histo_t *hh)
{
  //////
  // Radial cross-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<nb_red*nb_dz*nb_theta;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(npix_full,indices,pixrad1,pixrad2,hh,n_side_phi)	\
  shared(nb_red,nb_dz,nb_theta,ipix_0,ipix_f)		\
  shared(i_red_interval,red_0,i_dz_max,i_theta_max)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_red*nb_dz*nb_theta,sizeof(histo_t));
    double cth_aperture=cos(1./i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=pixrad1[ip1].np;
      RadialPixelInfo *pi1=pixrad1[ip1].pi;
      int *bounds=pi1->bounds;

      for(ii=0;ii<np1;ii++) {
	int icth;
	double *pos1=&(pi1->pos[N_POS*ii]);
	double redshift1=pi1->redshifts[ii];

	for(icth=bounds[0];icth<=bounds[1];icth++) {
	  int iphi;
	  int icth_n=icth*n_side_phi;
	  for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	    int iphi_true=(iphi+n_side_phi)%n_side_phi;
	    int ip2=iphi_true+icth_n;
	    if(pixrad2[ip2].np>0) {
	      int jj;
	      int np2=pixrad2[ip2].np;
	      RadialPixelInfo *pi2=pixrad2[ip2].pi;
	      for(jj=0;jj<np2;jj++) {
		double *pos2=&(pi2->pos[N_POS*jj]);
		double prod=pos1[0]*pos2[0]+
		  pos1[1]*pos2[1]+pos1[2]*pos2[2];
		if(prod>cth_aperture) {
		  double z_mean=0.5*(redshift1+pi2->redshifts[jj]);
		  int iz_mean=(int)((z_mean-red_0)*i_red_interval*nb_red);
		  if((iz_mean>=0)&&(iz_mean<nb_red)) {
		    double dz=fabs(redshift1-pi2->redshifts[jj]);
		    int idz=(int)(dz*i_dz_max*nb_dz);
		    if((idz<nb_dz)&&(idz>=0)) {
		      int ith=th2bin(prod);
		      if((ith<nb_theta)&&(ith>=0)) {
			int index=ith+nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
			hthread[index]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			hthread[index]++;
#endif //_WITH_WEIGHTS
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_red*nb_dz*nb_theta;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void corr_full_pm(RadialCell *cellsD,RadialCell *cellsR,
		  histo_t *DD,histo_t *DR,histo_t *RR)
{
  //////
  // PM angular correlator

  int i,ipix_0,ipix_f,npix_full;
  int *ipix_full;
  for(i=0;i<nb_red*nb_dz*nb_theta;i++) {
    DD[i]=0;
    DR[i]=0;
    RR[i]=0;
  }
  ipix_full=my_calloc(n_boxes2D,sizeof(int));
  npix_full=0;
  for(i=0;i<n_boxes2D;i++) {
    if((cellsD[i].np>0) || (cellsR[i].np)) {
      ipix_full[npix_full]=i;
      npix_full++;
    }
  }
  share_iters(npix_full,&ipix_0,&ipix_f);

#pragma omp parallel default(none)				\
  shared(cellsD,cellsR,DD,DR,RR,n_side_phi,n_boxes2D)		\
  shared(nb_red,nb_dz,nb_theta,ipix_0,ipix_f,ipix_full)	\
  shared(i_red_interval,red_0,i_dz_max,i_theta_max)
  {
    int j;
    histo_t *DDthread=(histo_t *)my_calloc(nb_red*nb_dz*nb_theta,sizeof(histo_t));
    histo_t *DRthread=(histo_t *)my_calloc(nb_red*nb_dz*nb_theta,sizeof(histo_t));
    histo_t *RRthread=(histo_t *)my_calloc(nb_red*nb_dz*nb_theta,sizeof(histo_t));
    double cth_max=cos(1/i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int *bounds;
      double *pos1;
      int icth;
      int ip1=ipix_full[j];
      int nD1=cellsD[ip1].np;
      int nR1=cellsR[ip1].np;
      if(nD1>0) {
	pos1=cellsD[ip1].ci->pos;
	bounds=cellsD[ip1].ci->bounds;
      }
      else if(nR1>0) {
	pos1=cellsR[ip1].ci->pos;
	bounds=cellsR[ip1].ci->bounds;
      }
      else continue;

      int ii;
      //DD, same pixel
      for(ii=0;ii<nD1;ii++) {
	int jj;
	double z1=cellsD[ip1].ci->redweight[N_RW*ii];
	for(jj=ii+1;jj<nD1;jj++) {
	  double zmean=0.5*(z1+cellsD[ip1].ci->redweight[N_RW*jj]);
	  int iz_mean=(int)((zmean-red_0)*i_red_interval*nb_red);
	  if((iz_mean>=0)&&(iz_mean<nb_red)) {
	    double dz=fabs(z1-cellsD[ip1].ci->redweight[N_RW*jj]);
	    int idz=(int)(dz*i_dz_max*nb_dz);
	    if((idz<nb_dz)&&(idz>=0)) {
	      int index=nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
	      DDthread[index]+=cellsD[ip1].ci->redweight[N_RW*ii+1]*
		cellsD[ip1].ci->redweight[N_RW*jj+1];
#else //_WITH_WEIGHTS
	      DDthread[index]++;
#endif //_WITH_WEIGHTS
	    }
	  }
	}
      }

      //RR, same pixel
      for(ii=0;ii<nR1;ii++) {
	int jj;
	double z1=cellsR[ip1].ci->redweight[N_RW*ii];
	for(jj=ii+1;jj<nR1;jj++) {
	  double zmean=0.5*(z1+cellsR[ip1].ci->redweight[N_RW*jj]);
	  int iz_mean=(int)((zmean-red_0)*i_red_interval*nb_red);
	  if((iz_mean>=0)&&(iz_mean<nb_red)) {
	    double dz=fabs(z1-cellsR[ip1].ci->redweight[N_RW*jj]);
	    int idz=(int)(dz*i_dz_max*nb_dz);
	    if((idz<nb_dz)&&(idz>=0)) {
	      int index=nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
	      RRthread[index]+=cellsR[ip1].ci->redweight[N_RW*ii+1]*
		cellsR[ip1].ci->redweight[N_RW*jj+1];
#else //_WITH_WEIGHTS
	      RRthread[index]++;
#endif //_WITH_WEIGHTS
	    }
	  }
	}
      }

      for(icth=bounds[0];icth<=bounds[1];icth++) {
	int iphi;
	int icth_n=icth*n_side_phi;
	for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	  double *pos2;
	  double prod;
	  int iphi_true=(iphi+n_side_phi)%n_side_phi;
	  int ip2=iphi_true+icth_n;
	  int nD2=cellsD[ip2].np;
	  int nR2=cellsR[ip2].np;
	  
	  if(nD2>0)
	    pos2=(cellsD[ip2].ci)->pos;
	  else if(nR2>0)
	    pos2=(cellsR[ip2].ci)->pos;
	  else continue;

	  prod=pos1[0]*pos2[0]+
	    pos1[1]*pos2[1]+pos1[2]*pos2[2];
	  
	  if(prod>cth_max) {
	    int ith=th2bin(prod);
	    if((ith<nb_theta)&&(ith>=0)) {
	      
	      //RR, different pixels
	      if(ip2>ip1) {
		if(nR2>0) {
		  for(ii=0;ii<nR1;ii++) {
		    int jj;
		    double z1=cellsR[ip1].ci->redweight[N_RW*ii];
		    for(jj=0;jj<nR2;jj++) {
		      double zmean=0.5*(z1+cellsR[ip2].ci->redweight[N_RW*jj]);
		      int iz_mean=(int)((zmean-red_0)*i_red_interval*nb_red);
		      if((iz_mean>=0)&&(iz_mean<nb_red)) {
			double dz=fabs(z1-cellsR[ip2].ci->redweight[N_RW*jj]);
			int idz=(int)(dz*i_dz_max*nb_dz);
			if((idz<nb_dz)&&(idz>=0)) {
			  int index=ith+nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
			  RRthread[index]+=cellsR[ip1].ci->redweight[N_RW*ii+1]*
			    cellsR[ip2].ci->redweight[N_RW*jj+1];
#else //_WITH_WEIGHTS
			  RRthread[index]++;
#endif //_WITH_WEIGHTS
			}
		      }
		    }
		  }
		}
	      }
	      
	      for(ii=0;ii<nD1;ii++) {
		int jj;
		double z1=cellsD[ip1].ci->redweight[N_RW*ii];
		
		//DD, different pixels
		if(ip2>ip1) {
		  for(jj=0;jj<nD2;jj++) {
		    double zmean=0.5*(z1+cellsD[ip2].ci->redweight[N_RW*jj]);
		    int iz_mean=(int)((zmean-red_0)*i_red_interval*nb_red);
		    if((iz_mean>=0)&&(iz_mean<nb_red)) {
		      double dz=fabs(z1-cellsD[ip2].ci->redweight[N_RW*jj]);
		      int idz=(int)(dz*i_dz_max*nb_dz);
		      if((idz<nb_dz)&&(idz>=0)) {
			int index=ith+nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
			DDthread[index]+=cellsD[ip1].ci->redweight[N_RW*ii+1]*
			  cellsD[ip2].ci->redweight[N_RW*jj+1];
#else //_WITH_WEIGHTS
			DDthread[index]++;
#endif //_WITH_WEIGHTS
		      }
		    }
		  }
		}

		//DR
		for(jj=0;jj<nR2;jj++) {
		  double zmean=0.5*(z1+cellsR[ip2].ci->redweight[N_RW*jj]);
		  int iz_mean=(int)((zmean-red_0)*i_red_interval*nb_red);
		  if((iz_mean>=0)&&(iz_mean<nb_red)) {
		    double dz=fabs(z1-cellsR[ip2].ci->redweight[N_RW*jj]);
		    int idz=(int)(dz*i_dz_max*nb_dz);
		    if((idz<nb_dz)&&(idz>=0)) {
		      int index=ith+nb_theta*(idz+nb_dz*iz_mean);
#ifdef _WITH_WEIGHTS
		      DRthread[index]+=cellsD[ip1].ci->redweight[N_RW*ii+1]*
			cellsR[ip2].ci->redweight[N_RW*jj+1];
#else //_WITH_WEIGHTS
		      DRthread[index]++;
#endif //_WITH_WEIGHTS
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_red*nb_dz*nb_theta;j++) {
	DD[j]+=DDthread[j];
	DR[j]+=DRthread[j];
	RR[j]+=RRthread[j];
      }
    }

    free(DDthread);
    free(DRthread);
    free(RRthread);
  } //end omp parallel
  free(ipix_full);
}

void auto_rad_bf(int npix_full,int *indices,RadialPixel *pixrad,
		 histo_t *hh)
{
  //////
  // Radial auto-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<nb_dz;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(npix_full,indices,pixrad,hh,n_side_phi,aperture_los)	\
  shared(nb_dz,i_dz_max,ipix_0,ipix_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_dz,sizeof(histo_t));
    double cth_aperture=cos(aperture_los);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=pixrad[ip1].np;
      RadialPixelInfo *pi1=pixrad[ip1].pi;
      int *bounds=pi1->bounds;

      for(ii=0;ii<np1;ii++) {
	int jj,icth;
	double *pos1=&(pi1->pos[N_POS*ii]);
	double redshift1=pi1->redshifts[ii];

	for(jj=ii+1;jj<np1;jj++) {
	  double *pos2=&(pi1->pos[N_POS*jj]);
	  double prod=pos1[0]*pos2[0]+
	    pos1[1]*pos2[1]+pos1[2]*pos2[2];
	  if(prod>cth_aperture) {
	    double dz=fabs(redshift1-pi1->redshifts[jj]);
	    int idz=(int)(dz*i_dz_max*nb_dz);
	    if((idz<nb_dz)&&(idz>=0)) {
#ifdef _WITH_WEIGHTS
	      hthread[idz]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
	      hthread[idz]++;
#endif //_WITH_WEIGHTS
	    }
	  }
	}

	for(icth=bounds[0];icth<=bounds[1];icth++) {
	  int iphi;
	  int icth_n=icth*n_side_phi;
	  for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	    int iphi_true=(iphi+n_side_phi)%n_side_phi;
	    int ip2=iphi_true+icth_n;
	    if(pixrad[ip2].np>0) {
	      if(ip2>ip1) {
		int np2=pixrad[ip2].np;
		RadialPixelInfo *pi2=pixrad[ip2].pi;
		for(jj=0;jj<np2;jj++) {
		  double *pos2=&(pi2->pos[N_POS*jj]);
		  double prod=pos1[0]*pos2[0]+
		    pos1[1]*pos2[1]+pos1[2]*pos2[2];
		  if(prod>cth_aperture) {
		    double dz=fabs(redshift1-pi2->redshifts[jj]);
		    int idz=(int)(dz*i_dz_max*nb_dz);
		    if((idz<nb_dz)&&(idz>=0)) {
#ifdef _WITH_WEIGHTS
		      hthread[idz]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		      hthread[idz]++;
#endif //_WITH_WEIGHTS
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_dz;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_rad_bf(int npix_full,int *indices,
		  RadialPixel *pixrad1,RadialPixel *pixrad2,
		  histo_t *hh)
{
  //////
  // Radial cross-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<nb_dz;i++) 
    hh[i]=0;

#pragma omp parallel default(none)					\
  shared(npix_full,indices,pixrad1,pixrad2,hh,n_side_phi,aperture_los)	\
  shared(nb_dz,i_dz_max,ipix_0,ipix_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_dz,sizeof(histo_t));
    double cth_aperture=cos(aperture_los);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=pixrad1[ip1].np;
      RadialPixelInfo *pi1=pixrad1[ip1].pi;
      int *bounds=pi1->bounds;

      for(ii=0;ii<np1;ii++) {
	int icth;
	double *pos1=&(pi1->pos[N_POS*ii]);
	double redshift1=pi1->redshifts[ii];
	for(icth=bounds[0];icth<=bounds[1];icth++) {
	  int iphi;
	  int icth_n=icth*n_side_phi;
	  for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	    int iphi_true=(iphi+n_side_phi)%n_side_phi;
	    int ip2=iphi_true+icth_n;
	    if(pixrad2[ip2].np>0) {
	      int jj;
	      int np2=pixrad2[ip2].np;
	      RadialPixelInfo *pi2=pixrad2[ip2].pi;
	      for(jj=0;jj<np2;jj++) {
		double *pos2=&(pi2->pos[N_POS*jj]);
		double prod=pos1[0]*pos2[0]+
		  pos1[1]*pos2[1]+pos1[2]*pos2[2];
		if(prod>cth_aperture) {
		  double dz=fabs(redshift1-pi2->redshifts[jj]);
		  int idz=(int)(dz*i_dz_max*nb_dz);
		  if((idz<nb_dz)&&(idz>=0)) {
#ifdef _WITH_WEIGHTS
		    hthread[idz]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		    hthread[idz]++;
#endif //_WITH_WEIGHTS
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_dz;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void auto_ang_bf(int npix_full,int *indices,Box2D *boxes,
		 histo_t *hh)
{
  //////
  // Angular auto-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<nb_theta;i++) 
    hh[i]=0;

#pragma omp parallel default(none)		\
  shared(npix_full,indices,boxes,hh,n_side_phi)	\
  shared(nb_theta,i_theta_max,ipix_0,ipix_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_theta,sizeof(histo_t));
    double cth_max=cos(1/i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=boxes[ip1].np;
      Box2DInfo *bi1=boxes[ip1].bi;
      int *bounds=bi1->bounds;
      for(ii=0;ii<np1;ii++) {
	double *pos1=&(bi1->pos[N_POS*ii]);
	
	int jj;
	for(jj=ii+1;jj<np1;jj++) {
	  double *pos2=&(bi1->pos[N_POS*jj]);
	  double prod=pos1[0]*pos2[0]+
	    pos1[1]*pos2[1]+pos1[2]*pos2[2];
	  if(prod>cth_max) {
	    int ith=th2bin(prod);
	    if((ith<nb_theta)&&(ith>=0)) {
#ifdef _WITH_WEIGHTS
	      hthread[ith]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
	      hthread[ith]++;
#endif //_WITH_WEIGHTS
	    }
	  }
	}

	int icth;
	for(icth=bounds[0];icth<=bounds[1];icth++) {
	  int iphi;
	  int icth_n=icth*n_side_phi;
	  for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	    int iphi_true=(iphi+n_side_phi)%n_side_phi;
	    int ip2=iphi_true+icth_n;
	    if(boxes[ip2].np>0) {
	      if(ip2>ip1) {
		int np2=boxes[ip2].np;
		Box2DInfo *bi2=boxes[ip2].bi;
		for(jj=0;jj<np2;jj++) {
		  double *pos2=&(bi2->pos[N_POS*jj]);
		  double prod=pos1[0]*pos2[0]+
		    pos1[1]*pos2[1]+pos1[2]*pos2[2];
		  if(prod>cth_max) {
		    int ith=th2bin(prod);
		    if((ith<nb_theta)&&(ith>=0)) {
#ifdef _WITH_WEIGHTS
		      hthread[ith]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		      hthread[ith]++;
#endif //_WITH_WEIGHTS
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_theta;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_ang_bf(int npix_full,int *indices,
		  Box2D *boxes1,Box2D *boxes2,
		  histo_t *hh)
{
  //////
  // Angular auto-correlator
  int i,ipix_0,ipix_f;
  share_iters(npix_full,&ipix_0,&ipix_f);

  for(i=0;i<nb_theta;i++) 
    hh[i]=0;

#pragma omp parallel default(none)			\
  shared(npix_full,indices,boxes1,boxes2,hh,n_side_phi)	\
  shared(nb_theta,i_theta_max,ipix_0,ipix_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_theta,sizeof(histo_t));
    double cth_max=cos(1/i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      int ii;
      int ip1=indices[j];
      int np1=boxes1[ip1].np;
      Box2DInfo *bi1=boxes1[ip1].bi;
      int *bounds=bi1->bounds;
      for(ii=0;ii<np1;ii++) {
	int icth;
	double *pos1=&(bi1->pos[N_POS*ii]);
	for(icth=bounds[0];icth<=bounds[1];icth++) {
	  int iphi;
	  int icth_n=icth*n_side_phi;
	  for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	    int iphi_true=(iphi+n_side_phi)%n_side_phi;
	    int ip2=iphi_true+icth_n;
	    if(boxes2[ip2].np>0) {
	      int jj;
	      int np2=boxes2[ip2].np;
	      Box2DInfo *bi2=boxes2[ip2].bi;
	      for(jj=0;jj<np2;jj++) {
		double *pos2=&(bi2->pos[N_POS*jj]);
		double prod=pos1[0]*pos2[0]+
		  pos1[1]*pos2[1]+pos1[2]*pos2[2];
		if(prod>cth_max) {
		  int ith=th2bin(prod);
		  if((ith<nb_theta)&&(ith>=0)) {
#ifdef _WITH_WEIGHTS
		    hthread[ith]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		    hthread[ith]++;
#endif //_WITH_WEIGHTS
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_theta;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void corr_angular_cross_pm(Cell2D *cellsD,Cell2D *cellsD_total,
			   Cell2D *cellsR,Cell2D *cellsR_total,
			   histo_t *DD,histo_t *DR,histo_t *RR)
{
  //////
  // PM angular correlator

  int i,ipix_0,ipix_f,npix_full;
  int *ipix_full;
  for(i=0;i<(nb_red*(nb_red+1)*nb_theta)/2;i++) {
    DD[i]=0;
    DR[i]=0;
    RR[i]=0;
  }
  ipix_full=my_calloc(n_boxes2D,sizeof(int));
  npix_full=0;
  for(i=0;i<n_boxes2D;i++) {
    if((cellsD[i].np>0) || (cellsR[i].np)) {
      ipix_full[npix_full]=i;
      npix_full++;
    }
  }
  share_iters(npix_full,&ipix_0,&ipix_f);

#pragma omp parallel default(none)					\
  shared(cellsD,cellsD_total,cellsR,cellsR_total)			\
  shared(DD,DR,RR,n_side_phi,n_boxes2D)					\
  shared(nb_theta,nb_red,i_theta_max,ipix_0,ipix_f,ipix_full)
  {
    int j;
    histo_t *DDthread=(histo_t *)my_calloc((nb_red*(nb_red+1)*nb_theta)/2,sizeof(histo_t));
    histo_t *DRthread=(histo_t *)my_calloc((nb_red*(nb_red+1)*nb_theta)/2,sizeof(histo_t));
    histo_t *RRthread=(histo_t *)my_calloc((nb_red*(nb_red+1)*nb_theta)/2,sizeof(histo_t));
    double cth_max=cos(1/i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      Cell2DInfo *ci1;
      int *bounds;
      double *pos1;
      int icth;
      int ip1=ipix_full[j];
      if(cellsD_total[ip1].np>0)
	ci1=cellsD_total[ip1].ci;
      else if(cellsR_total[ip1].np>0)
	ci1=cellsR_total[ip1].ci;
      else continue;
      bounds=ci1->bounds;
      pos1=ci1->pos;
      for(icth=bounds[0];icth<=bounds[1];icth++) {
	int iphi;
	int icth_n=icth*n_side_phi;
	for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	  double *pos2;
	  double prod;
	  int iphi_true=(iphi+n_side_phi)%n_side_phi;
	  int ip2=iphi_true+icth_n;
	  
	  if(cellsD_total[ip2].np>0)
	    pos2=(cellsD_total[ip2].ci)->pos;
	  else if(cellsR_total[ip2].np>0)
	    pos2=(cellsR_total[ip2].ci)->pos;
	  else continue;

	  prod=pos1[0]*pos2[0]+
	    pos1[1]*pos2[1]+pos1[2]*pos2[2];
	  
	  if(prod>cth_max) {
	    int iz1;
	    int ith=th2bin(prod);
	    if((ith<nb_theta)&&(ith>=0)) {
	      for(iz1=0;iz1<nb_red;iz1++) {
		int iz2;
		np_t nD1=cellsD[ip1*nb_red+iz1].np;
		np_t nR1=cellsR[ip1*nb_red+iz1].np;
		for(iz2=0;iz2<nb_red;iz2++) {
		  np_t nD2=cellsD[ip2*nb_red+iz2].np;
		  np_t nR2=cellsR[ip2*nb_red+iz2].np;
		  int imin=MIN(iz1,iz2);
		  int imax=MAX(iz1,iz2);
		  //		  int index=ith+nb_theta*((imin*(2*nb_red-imin-1))/2+imax);
		  //		  int index=iz1+nb_red*(iz2+nb_red*ith);		  
		  int index=imax+(imin*(2*nb_red-imin-1))/2+ith*(nb_red*(nb_red+1))/2;
		  DDthread[index]+=nD1*nD2;
		  DRthread[index]+=nD1*nR2;
		  RRthread[index]+=nR1*nR2;
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_theta;j++) {
	int iz1;
	for(iz1=0;iz1<nb_red;iz1++) {
	  int iz2;
	  for(iz2=iz1;iz2<nb_red;iz2++) {
	    int index_a=j+nb_theta*((iz1*(2*nb_red-iz1-1))/2+iz2);
	    int index_b=iz2+(iz1*(2*nb_red-iz1-1))/2+j*(nb_red*(nb_red+1))/2;
	    DD[index_a]+=DDthread[index_b];
	    DR[index_a]+=DRthread[index_b];
	    RR[index_a]+=RRthread[index_b];
	  }
	}
      }
    }

    free(DDthread);
    free(DRthread);
    free(RRthread);
  } //end omp parallel

  for(i=0;i<(nb_red*(nb_red+1)*nb_theta)/2;i++) {
    DD[i]/=2;
    RR[i]/=2;
  }
  free(ipix_full);
}

void corr_ang_pm(Cell2D *cellsD,Cell2D *cellsR,
		 histo_t *DD,histo_t *DR,histo_t *RR)
{
  //////
  // PM angular correlator

  int i,ipix_0,ipix_f,npix_full;
  int *ipix_full;
  for(i=0;i<nb_theta;i++) {
    DD[i]=0;
    DR[i]=0;
    RR[i]=0;
  }
  ipix_full=my_calloc(n_boxes2D,sizeof(int));
  npix_full=0;
  for(i=0;i<n_boxes2D;i++) {
    if((cellsD[i].np>0) || (cellsR[i].np)) {
      ipix_full[npix_full]=i;
      npix_full++;
    }
  }
  share_iters(npix_full,&ipix_0,&ipix_f);

#pragma omp parallel default(none)				\
  shared(cellsD,cellsR,DD,DR,RR,n_side_phi,n_boxes2D)		\
  shared(nb_theta,i_theta_max,ipix_0,ipix_f,ipix_full)
  {
    int j;
    histo_t *DDthread=(histo_t *)my_calloc(nb_theta,sizeof(histo_t));
    histo_t *DRthread=(histo_t *)my_calloc(nb_theta,sizeof(histo_t));
    histo_t *RRthread=(histo_t *)my_calloc(nb_theta,sizeof(histo_t));
    double cth_max=cos(1/i_theta_max);

#pragma omp for nowait schedule(dynamic)
    for(j=ipix_0;j<ipix_f;j++) {
      Cell2DInfo *ci1;
      int *bounds;
      double *pos1;
      int icth;
      int ip1=ipix_full[j];
      np_t nD1=cellsD[ip1].np;
      np_t nR1=cellsR[ip1].np;
      if(nD1>0)
	ci1=cellsD[ip1].ci;
      else if(nR1>0)
	ci1=cellsR[ip1].ci;
      else continue;
      bounds=ci1->bounds;
      pos1=ci1->pos;
      for(icth=bounds[0];icth<=bounds[1];icth++) {
	int iphi;
	int icth_n=icth*n_side_phi;
	for(iphi=bounds[2];iphi<=bounds[3];iphi++) {
	  double *pos2;
	  double prod;
	  int iphi_true=(iphi+n_side_phi)%n_side_phi;
	  int ip2=iphi_true+icth_n;
	  
	  if(cellsD[ip2].np>0)
	    pos2=(cellsD[ip2].ci)->pos;
	  else if(cellsR[ip2].np>0)
	    pos2=(cellsR[ip2].ci)->pos;
	  else continue;

	  prod=pos1[0]*pos2[0]+
	    pos1[1]*pos2[1]+pos1[2]*pos2[2];
	  
	  if(prod>cth_max) {
	    int ith=th2bin(prod);
	    if((ith<nb_theta)&&(ith>=0)) {
	      DDthread[ith]+=nD1*cellsD[ip2].np;
	      DRthread[ith]+=nD1*cellsR[ip2].np;
	      RRthread[ith]+=nR1*cellsR[ip2].np;
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_theta;j++) {
	DD[j]+=DDthread[j];
	DR[j]+=DRthread[j];
	RR[j]+=RRthread[j];
      }
    }

    free(DDthread);
    free(DRthread);
    free(RRthread);
  } //end omp parallel

  for(i=0;i<nb_theta;i++) {
    DD[i]/=2;
    RR[i]/=2;
  }
  free(ipix_full);
}

void auto_mono_bf(int nbox_full,int *indices,Box3D *boxes,
		  histo_t *hh)
{
  //////
  // Monopole auto-correlator
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);   //this splits the filled boxes between the MPI threads

  for(i=0;i<nb_r;i++) 
    hh[i]=0;

#pragma omp parallel default(none)			\
  shared(nbox_full,indices,boxes,hh,n_side,l_box)	\
  shared(i_r_max,nb_r,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_r,sizeof(histo_t));
    double r2_max=1./(i_r_max*i_r_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(1/(i_r_max*dx))+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {    //loop over the boxes assigned to this MPI thread
      int ii;
      int ip1=indices[j];       //ip1 is the 'true' index corresponding to this box

      int np1=boxes[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {   //loop over the particles in box ip1
	double *pos1=&(boxes[ip1].pos[N_POS*ii]);
	
	int jj;
	for(jj=ii+1;jj<np1;jj++) { //loop to calculate pairs within box ip1 without double-counting
	  double r2;
	  double *pos2=&(boxes[ip1].pos[N_POS*jj]);
	  double xr[3];
	  xr[0]=pos1[0]-pos2[0];
	  xr[1]=pos1[1]-pos2[1];
	  xr[2]=pos1[2]-pos2[2];
	  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
	  if(r2<r2_max) {
	    int ir=r2bin(r2);
	    if((ir<nb_r)&&(ir>=0)) {
#ifdef _WITH_WEIGHTS
	      hthread[ir]+=pos1[3]*pos2[3]; //increment by product of weights
#else //_WITH_WEIGHTS
	      hthread[ir]++;                //or increment by 1 if not using weights
#endif //_WITH_WEIGHTS
	    }
	  }
	}

	int iz;
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;     //ip2 is the index corresponding to the 'neighbouring' box
	      if(boxes[ip2].np>0) {     //if box ip2 contains any particles...
		if(ip2>ip1) {               //AND if it is different to ip1 (ip2>ip1 to avoid double-counting)...
		  int np2=boxes[ip2].np;
		  for(jj=0;jj<np2;jj++) {   //...loop over the particles of ip2 calculating distances and counting pairs
		    double r2;
		    double *pos2=&(boxes[ip2].pos[N_POS*jj]);
		    double xr[3];
		    xr[0]=pos1[0]-pos2[0];
		    xr[1]=pos1[1]-pos2[1];
		    xr[2]=pos1[2]-pos2[2];
		    r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		    if(r2<r2_max) {
		      int ir=r2bin(r2);
		      if((ir<nb_r)&&(ir>=0)) {
#ifdef _WITH_WEIGHTS
			hthread[ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			hthread[ir]++;
#endif //_WITH_WEIGHTS
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_r;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_mono_bf(int nbox_full,int *indices,
		   Box3D *boxes1,Box3D *boxes2,
		   histo_t *hh)
{
  //////
  // Monopole cross-correlator
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_r;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(nbox_full,indices,boxes1,boxes2,hh,n_side,l_box)	\
  shared(nb_r,i_r_max,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_r,sizeof(histo_t));
    double r2_max=1./(i_r_max*i_r_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(1/(i_r_max*dx))+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes1[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	int iz;
	double *pos1=&(boxes1[ip1].pos[N_POS*ii]);
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes2[ip2].np>0) {
		int jj;
		int np2=boxes2[ip2].np;
		for(jj=0;jj<np2;jj++) {
		  double r2;
		  double *pos2=&(boxes2[ip2].pos[N_POS*jj]);
		  double xr[3];
		  xr[0]=pos1[0]-pos2[0];
		  xr[1]=pos1[1]-pos2[1];
		  xr[2]=pos1[2]-pos2[2];
		  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		  if(r2<r2_max) {
		    int ir=r2bin(r2);
		    if((ir<nb_r)&&(ir>=0)) {
#ifdef _WITH_WEIGHTS
		      hthread[ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		      hthread[ir]++;
#endif //_WITH_WEIGHTS
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_r;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void auto_3d_ps_bf(int nbox_full,int *indices,Box3D *boxes,
		   histo_t *hh)
{
  //////
  // Monopole auto-correlator
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_rl*nb_rt;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(nbox_full,indices,boxes,hh,n_side,l_box)		\
  shared(i_rt_max,nb_rt,i_rl_max,nb_rl,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_rl*nb_rt,sizeof(histo_t));
    double r2_max=1./(i_rt_max*i_rt_max)+1./(i_rl_max*i_rl_max);
    double rt2_max=1./(i_rt_max*i_rt_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(sqrt(r2_max)/dx)+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	double *pos1=&(boxes[ip1].pos[N_POS*ii]);
	
	int jj;
	for(jj=ii+1;jj<np1;jj++) {
	  double r2;
	  double *pos2=&(boxes[ip1].pos[N_POS*jj]);
	  double xr[3],xcm[3];
	  xr[0]=pos1[0]-pos2[0];
	  xr[1]=pos1[1]-pos2[1];
	  xr[2]=pos1[2]-pos2[2];
	  xcm[0]=0.5*(pos1[0]+pos2[0]);
	  xcm[1]=0.5*(pos1[1]+pos2[1]);
	  xcm[2]=0.5*(pos1[2]+pos2[2]);
	  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
	  if(r2<r2_max) {
	    double rl=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
	      sqrt(xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2]);
	    int irl=(int)(rl*i_rl_max*nb_rl);
	    if((irl<nb_rl)&&(irl>=0)) {
	      double rt2=r2-rl*rl;
	      if(rt2<rt2_max) {
		int irt=(int)(sqrt(rt2)*i_rt_max*nb_rt);
		if((irt<nb_rt)&&(irt>=0)) {
#ifdef _WITH_WEIGHTS
		  hthread[irl+nb_rl*irt]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		  hthread[irl+nb_rl*irt]++;
#endif //_WITH_WEIGHTS
		}
	      }
	    }
	  }
	}

	int iz;
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes[ip2].np>0) {
		if(ip2>ip1) {
		  int np2=boxes[ip2].np;
		  for(jj=0;jj<np2;jj++) {
		    double r2;
		    double *pos2=&(boxes[ip2].pos[N_POS*jj]);
		    double xr[3],xcm[3];
		    xr[0]=pos1[0]-pos2[0];
		    xr[1]=pos1[1]-pos2[1];
		    xr[2]=pos1[2]-pos2[2];
		    xcm[0]=0.5*(pos1[0]+pos2[0]);
		    xcm[1]=0.5*(pos1[1]+pos2[1]);
		    xcm[2]=0.5*(pos1[2]+pos2[2]);
		    r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		    if(r2<r2_max) {
		      double rl=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
			sqrt(xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2]);
		      int irl=(int)(rl*i_rl_max*nb_rl);
		      if((irl<nb_rl)&&(irl>=0)) {
			double rt2=r2-rl*rl;
			if(rt2<rt2_max) {
			  int irt=(int)(sqrt(rt2)*i_rt_max*nb_rt);
			  if((irt<nb_rt)&&(irt>=0)) {
#ifdef _WITH_WEIGHTS
			    hthread[irl+nb_rl*irt]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			    hthread[irl+nb_rl*irt]++;
#endif //_WITH_WEIGHTS
			  }
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_rl*nb_rt;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_3d_ps_bf(int nbox_full,int *indices,
		    Box3D *boxes1,Box3D *boxes2,
		    histo_t *hh)
{
  //////
  // Monopole auto-correlator
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_rl*nb_rt;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(nbox_full,indices,boxes1,boxes2,hh,n_side,l_box)	\
  shared(i_rt_max,nb_rt,i_rl_max,nb_rl,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_rl*nb_rt,sizeof(histo_t));
    double r2_max=1./(i_rt_max*i_rt_max)+1./(i_rl_max*i_rl_max);
    double rt2_max=1./(i_rt_max*i_rt_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(sqrt(r2_max)/dx)+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes1[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	int iz;
	double *pos1=&(boxes1[ip1].pos[N_POS*ii]);
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes2[ip2].np>0) {
		int jj;
		int np2=boxes2[ip2].np;
		for(jj=0;jj<np2;jj++) {
		  double r2;
		  double *pos2=&(boxes2[ip2].pos[N_POS*jj]);
		  double xr[3],xcm[3];
		  xr[0]=pos1[0]-pos2[0];
		  xr[1]=pos1[1]-pos2[1];
		  xr[2]=pos1[2]-pos2[2];
		  xcm[0]=0.5*(pos1[0]+pos2[0]);
		  xcm[1]=0.5*(pos1[1]+pos2[1]);
		  xcm[2]=0.5*(pos1[2]+pos2[2]);
		  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		  if(r2<r2_max) {
		    double rl=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
		      sqrt(xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2]);
		    int irl=(int)(rl*i_rl_max*nb_rl);
		    if((irl<nb_rl)&&(irl>=0)) {
		      double rt2=r2-rl*rl;
		      if(rt2<rt2_max) {
			int irt=(int)(sqrt(rt2)*i_rt_max*nb_rt);
			if((irt<nb_rt)&&(irt>=0)) {
#ifdef _WITH_WEIGHTS
			  hthread[irl+nb_rl*irt]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			  hthread[irl+nb_rl*irt]++;
#endif //_WITH_WEIGHTS
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_rl*nb_rt;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void auto_3d_rm_bf(int nbox_full,int *indices,Box3D *boxes,
		   histo_t *hh)
{
  //////
  // r-mu auto-correlator
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_r*nb_mu;i++) 
    hh[i]=0;

#pragma omp parallel default(none)			\
  shared(nbox_full,indices,boxes,hh,n_side,l_box)	\
  shared(i_r_max,nb_r,nb_mu,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_r*nb_mu,sizeof(histo_t));
    double r2_max=1./(i_r_max*i_r_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(sqrt(r2_max)/dx)+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	double *pos1=&(boxes[ip1].pos[N_POS*ii]);
	
	int jj;
	for(jj=ii+1;jj<np1;jj++) {
	  double r2;
	  double *pos2=&(boxes[ip1].pos[N_POS*jj]);
	  double xr[3],xcm[3];
	  xr[0]=pos1[0]-pos2[0];
	  xr[1]=pos1[1]-pos2[1];
	  xr[2]=pos1[2]-pos2[2];
	  xcm[0]=0.5*(pos1[0]+pos2[0]);
	  xcm[1]=0.5*(pos1[1]+pos2[1]);
	  xcm[2]=0.5*(pos1[2]+pos2[2]);
	  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
	  if(r2<r2_max) {
	    int ir=r2bin(r2);
	    if((ir<nb_r)&&(ir>=0)) {
	      int icth;
	      if(r2==0) icth=0;
	      else {
		double cth=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
		  sqrt((xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2])*r2);
		icth=(int)(cth*nb_mu);
	      }
	      if((icth<nb_mu)&&(icth>=0)) {
#ifdef _WITH_WEIGHTS
		hthread[icth+nb_mu*ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		hthread[icth+nb_mu*ir]++;
#endif //_WITH_WEIGHTS
	      }
	    }
	  }
	}

	int iz;
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes[ip2].np>0) {
		if(ip2>ip1) {
		  int np2=boxes[ip2].np;
		  for(jj=0;jj<np2;jj++) {
		    double r2;
		    double *pos2=&(boxes[ip2].pos[N_POS*jj]);
		    double xr[3],xcm[3];
		    xr[0]=pos1[0]-pos2[0];
		    xr[1]=pos1[1]-pos2[1];
		    xr[2]=pos1[2]-pos2[2];
		    xcm[0]=0.5*(pos1[0]+pos2[0]);
		    xcm[1]=0.5*(pos1[1]+pos2[1]);
		    xcm[2]=0.5*(pos1[2]+pos2[2]);
		    r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		    if(r2<r2_max) {
		      int ir=r2bin(r2);
		      if((ir<nb_r)&&(ir>=0)) {
			int icth;
			if(r2==0) icth=0;
			else {
			  double cth=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
			    sqrt((xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2])*r2);
			  icth=(int)(cth*nb_mu);
			}
			if((icth<nb_mu)&&(icth>=0)) {
#ifdef _WITH_WEIGHTS
			  hthread[icth+nb_mu*ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			  hthread[icth+nb_mu*ir]++;
#endif //_WITH_WEIGHTS
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_r*nb_mu;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void auto_3d_rm_special_bf(int nbox_full,int *indices,Box3D *boxes,
		   histo_t *hh)
{
  //////
  // r-mu auto-correlator with special condition on l-o-s
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_r*nb_mu;i++) 
    hh[i]=0;

#pragma omp parallel default(none)			\
  shared(nbox_full,indices,boxes,hh,n_side,l_box)	\
  shared(i_r_max,nb_r,nb_mu,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_r*nb_mu,sizeof(histo_t));
    double r2_max=1./(i_r_max*i_r_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(sqrt(r2_max)/dx)+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	double *pos1=&(boxes[ip1].pos[N_POS*ii]);
	
	int jj;
	for(jj=ii+1;jj<np1;jj++) {
	  double r2;
	  double *pos2=&(boxes[ip1].pos[N_POS*jj]);
	  double xr[3],xcm[3];
	  xr[0]=pos1[0]-pos2[0];
	  xr[1]=pos1[1]-pos2[1];
	  xr[2]=pos1[2]-pos2[2];
	  // xcm = pos1 so that angle is measured wrt l-o-s direction to first particle, not centre of mass
      xcm[0]=pos1[0];
      xcm[1]=pos1[1];
      xcm[2]=pos1[2];
	  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
	  if(r2<r2_max) {
	    int ir=r2bin(r2);
	    if((ir<nb_r)&&(ir>=0)) {
	      int icth;
	      if(r2==0) icth=0;
	      else {
		double cth=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
		  sqrt((xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2])*r2);
		icth=(int)(cth*nb_mu);
	      }
	      if((icth<nb_mu)&&(icth>=0)) {
#ifdef _WITH_WEIGHTS
		hthread[icth+nb_mu*ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
		hthread[icth+nb_mu*ir]++;
#endif //_WITH_WEIGHTS
	      }
	    }
	  }
	}

	int iz;
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes[ip2].np>0) {
		if(ip2>ip1) {
		  int np2=boxes[ip2].np;
		  for(jj=0;jj<np2;jj++) {
		    double r2;
		    double *pos2=&(boxes[ip2].pos[N_POS*jj]);
		    double xr[3],xcm[3];
		    xr[0]=pos1[0]-pos2[0];
		    xr[1]=pos1[1]-pos2[1];
		    xr[2]=pos1[2]-pos2[2];
		    xcm[0]=0.5*(pos1[0]+pos2[0]);
		    xcm[1]=0.5*(pos1[1]+pos2[1]);
		    xcm[2]=0.5*(pos1[2]+pos2[2]);
		    r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		    if(r2<r2_max) {
		      int ir=r2bin(r2);
		      if((ir<nb_r)&&(ir>=0)) {
			int icth;
			if(r2==0) icth=0;
			else {
			  double cth=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
			    sqrt((xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2])*r2);
			  icth=(int)(cth*nb_mu);
			}
			if((icth<nb_mu)&&(icth>=0)) {
#ifdef _WITH_WEIGHTS
			  hthread[icth+nb_mu*ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			  hthread[icth+nb_mu*ir]++;
#endif //_WITH_WEIGHTS
			}
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_r*nb_mu;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_3d_rm_bf(int nbox_full,int *indices,
		    Box3D *boxes1,Box3D *boxes2,
		    histo_t *hh)
{
  //////
  // 3D r-mu cross-correlator
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_r*nb_mu;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(nbox_full,indices,boxes1,boxes2,hh,n_side,l_box)	\
  shared(nb_r,nb_mu,i_r_max,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_r*nb_mu,sizeof(histo_t));
    double r2_max=1./(i_r_max*i_r_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(sqrt(r2_max)/dx)+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes1[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	int iz;
	double *pos1=&(boxes1[ip1].pos[N_POS*ii]);
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes2[ip2].np>0) {
		int jj;
		int np2=boxes2[ip2].np;
		for(jj=0;jj<np2;jj++) {
		  double r2;
		  double *pos2=&(boxes2[ip2].pos[N_POS*jj]);
		  double xr[3],xcm[3];
		  xr[0]=pos1[0]-pos2[0];
		  xr[1]=pos1[1]-pos2[1];
		  xr[2]=pos1[2]-pos2[2];
		  xcm[0]=0.5*(pos1[0]+pos2[0]);
		  xcm[1]=0.5*(pos1[1]+pos2[1]);
		  xcm[2]=0.5*(pos1[2]+pos2[2]);
		  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		  if(r2<r2_max) {
		    int ir=r2bin(r2);
		    if((ir<nb_r)&&(ir>=0)) {
		      int icth;
		      if(r2==0) icth=0;
		      else {
			double cth=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
			  sqrt((xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2])*r2);
			icth=(int)(cth*nb_mu);
		      }
		      if((icth<nb_mu)&&(icth>=0)) {
#ifdef _WITH_WEIGHTS
			hthread[icth+nb_mu*ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			hthread[icth+nb_mu*ir]++;
#endif //_WITH_WEIGHTS
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_r*nb_mu;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}

void cross_3d_rm_special_bf(int nbox_full,int *indices,
		    Box3D *boxes1,Box3D *boxes2,
		    histo_t *hh)
{
  //////
  // 3D r-mu cross-correlator that uses a special line-of-sight condition
  // designed for the cross-correlation of voids with galaxies
  int i,ibox_0,ibox_f;
  share_iters(nbox_full,&ibox_0,&ibox_f);

  for(i=0;i<nb_r*nb_mu;i++) 
    hh[i]=0;

#pragma omp parallel default(none)				\
  shared(nbox_full,indices,boxes1,boxes2,hh,n_side,l_box)	\
  shared(nb_r,nb_mu,i_r_max,ibox_0,ibox_f)
  {
    int j;
    histo_t *hthread=(histo_t *)my_calloc(nb_r*nb_mu,sizeof(histo_t));
    double r2_max=1./(i_r_max*i_r_max);
    int irange[3];
    
    for(j=0;j<3;j++) {
      double dx=l_box[j]/n_side[j];
      irange[j]=(int)(sqrt(r2_max)/dx)+1;
    }

#pragma omp for nowait schedule(dynamic)
    for(j=ibox_0;j<ibox_f;j++) {
      int ii;
      int ip1=indices[j];

      int np1=boxes1[ip1].np;

      int ix1=ip1%n_side[0];
      int iz1=ip1/(n_side[0]*n_side[1]);
      int iy1=(ip1-ix1-iz1*n_side[0]*n_side[1])/n_side[0];

      int ixmin=MAX(ix1-irange[0],0);
      int ixmax=MIN(ix1+irange[0],n_side[0]-1);
      int iymin=MAX(iy1-irange[1],0);
      int iymax=MIN(iy1+irange[1],n_side[1]-1);
      int izmin=MAX(iz1-irange[2],0);
      int izmax=MIN(iz1+irange[2],n_side[2]-1);

      for(ii=0;ii<np1;ii++) {
	int iz;
	double *pos1=&(boxes1[ip1].pos[N_POS*ii]);
	for(iz=izmin;iz<=izmax;iz++) {
	  int iy;
	  int iz_n=iz*n_side[0]*n_side[1];
	  for(iy=iymin;iy<=iymax;iy++) {
	    int ix;
	    int iy_n=iy*n_side[0];
	    for(ix=ixmin;ix<=ixmax;ix++) {
	      int ip2=ix+iy_n+iz_n;
	      if(boxes2[ip2].np>0) {
		int jj;
		int np2=boxes2[ip2].np;
		for(jj=0;jj<np2;jj++) {
		  double r2;
		  double *pos2=&(boxes2[ip2].pos[N_POS*jj]);
		  double xr[3],xcm[3];
		  xr[0]=pos1[0]-pos2[0];
		  xr[1]=pos1[1]-pos2[1];
		  xr[2]=pos1[2]-pos2[2];
          // xcm = pos1 so that angle is measured wrt l-o-s direction to void centre, not centre of mass
		  xcm[0]=pos1[0];
		  xcm[1]=pos1[1];
		  xcm[2]=pos1[2];
		  r2=xr[0]*xr[0]+xr[1]*xr[1]+xr[2]*xr[2];
		  if(r2<r2_max) {
		    int ir=r2bin(r2);
		    if((ir<nb_r)&&(ir>=0)) {
		      int icth;
		      if(r2==0) icth=0;
		      else {
			double cth=fabs(xr[0]*xcm[0]+xr[1]*xcm[1]+xr[2]*xcm[2])/
			  sqrt((xcm[0]*xcm[0]+xcm[1]*xcm[1]+xcm[2]*xcm[2])*r2);
			icth=(int)(cth*nb_mu);
		      }
		      if((icth<nb_mu)&&(icth>=0)) {
#ifdef _WITH_WEIGHTS
			hthread[icth+nb_mu*ir]+=pos1[3]*pos2[3];
#else //_WITH_WEIGHTS
			hthread[icth+nb_mu*ir]++;
#endif //_WITH_WEIGHTS
		      }
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
    } // end omp for

#pragma omp critical
    {
      for(j=0;j<nb_r*nb_mu;j++)
	hh[j]+=hthread[j];
    }

    free(hthread);
  } //end omp parallel
}
