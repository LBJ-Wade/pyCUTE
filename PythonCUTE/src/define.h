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

#ifndef _CUTE_DEFINE_
#define _CUTE_DEFINE_

#ifdef _HAVE_MPI
#include <mpi.h>
#endif //_HAVE_MPI

extern char fnameData[128];
extern char fnameData2[128];
extern char fnameRandom[128];
extern char fnameRandom2[128];
extern char fnameOut[128];
extern char fnameMask[128];
extern char fnamedNdz[128];

extern int corr_type;

extern double omega_M;
extern double omega_L;
extern double weos;

extern double aperture_los;

extern int use_pm;

extern int fact_n_rand;
extern int gen_ran;
extern int reuse_ran;

extern int logbin;
extern int n_logint;
extern int nb_red;
extern double i_red_interval;
extern double red_0;
extern int nb_dz;
extern double i_dz_max;
extern int nb_theta;
extern double i_theta_max;
extern double log_th_max;
extern int nb_r;
extern double i_r_max;
extern double log_r_max;
extern int nb_rl;
extern int nb_rt;
extern int nb_mu;
extern double i_rl_max;
extern double i_rt_max;

extern int n_side_cth;
extern int n_side_phi;
extern int n_boxes2D;
extern int n_side[3];
extern int n_boxes3D;
extern double l_box[3];

extern int cute_verbose;

/*                MACROS            */
// Other possible macros
//_DEBUG, _VERBOSE, _TRUE_ACOS

#define DTORAD 0.017453292519943295 // x deg = x*DTORAD rad
#define MAX(a, b) (((a) > (b)) ? (a) : (b)) //Maximum of two numbers
#define MIN(a, b) (((a) < (b)) ? (a) : (b)) //Minimum of two numbers
#define ABS(a)   (((a) < 0) ? -(a) : (a)) //Absolute value
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x))) //min(max(a,low),high)

/////////////////////////////
//Histogram options for CUDA
//<1-D
#define NB_HISTO_1D 256 //#bins for 1-D histograms
//2-D
#ifdef _HISTO_2D_128    //128x128 option for 2D histograms in CUDA
#define NB_HISTO_2D 128 //#bins for 2-D histograms
#define NB_X_BATCH 32   //#columns loaded in each batch (4096/NB_HISTO_2D)
#define NTH_RWS_2D 4    //#thread rows (1024/NB_HISTO_2D)
#endif
#ifdef _HISTO_2D_64     //64x64 option for 2D histograms in CUDA
#define NB_HISTO_2D 64
#define NB_X_BATCH 64
#define NTH_RWS_2D 8
#endif
////////////////////////////

#ifndef RED_COSMO_MAX
#define RED_COSMO_MAX 4.0 //# maximum redshift
#endif

#ifdef _WITH_WEIGHTS
#define N_RW 2
#define N_POS 4
typedef double histo_t;
typedef double np_t;
#ifdef _HAVE_MPI
#define HISTO_T_MPI MPI_DOUBLE
#endif //_HAVE_MPI
#else //_WITH_WEIGHTS
#define N_RW 1
#define N_POS 3
typedef unsigned long long histo_t;
typedef int np_t;
#ifdef _HAVE_MPI
#define HISTO_T_MPI MPI_UNSIGNED_LONG_LONG
#endif //_HAVE_MPI
#endif //_WITH_WEIGHTS

//Cell for angular 2PCF
typedef struct {
  int bounds[4];
  double pos[3];
} Cell2DInfo;

typedef struct {
  np_t np;
  Cell2DInfo *ci;
} Cell2D;


//Box for angular 2PCF
typedef struct {
  int bounds[4];
  double *pos;
} Box2DInfo;

typedef struct {
  int np;
  Box2DInfo *bi;
} Box2D;


//Pixel for radial 2PCF
typedef struct {
  int bounds[4];
  double *pos;
  double *redshifts;
} RadialPixelInfo;

typedef struct {
  int np;
  RadialPixelInfo *pi;
} RadialPixel;


//Cell for radial 2PCF
typedef struct {
  int bounds[4];
  double pos[3];
  double *redweight;
} RadialCellInfo;

typedef struct {
  int np;
  RadialCellInfo *ci;
} RadialCell;


//Box for 3D 2PCFs
typedef struct {
  int np;
  double *pos;
} Box3D; //3D cell


//Mask cube
typedef struct {
  double z0,zf;
  double cth0,cthf;
  double phi0,phif;
} MaskRegion; //Mask region (cube in z,cos(theta),phi)


//Catalog
typedef struct {
  int np;
  double *red,*cth,*phi;
#ifdef _WITH_WEIGHTS
  double *weight;
#endif //_WITH_WEIGHTS
#ifdef _CUTE_AS_PYTHON_MODULE
  np_t sum_w, sum_w2;
#endif
} Catalog; //Catalog (double precision)

#ifdef _CUTE_AS_PYTHON_MODULE

typedef struct {
  int nx, ny, nz;
  double *x, *y, *z, *corr, 
         *D1D1, *D1D2, *D1R1, *D1R2,
         *D2D2, *D2R1, *D2R2, 
         *R1R1, *R1R2, 
         *R2R2;
} Result;

extern Result *global_result;
extern Catalog *global_galaxy_catalog;
extern Catalog *global_galaxy_catalog2;
extern Catalog *global_random_catalog;
extern Catalog *global_random_catalog2;

#endif

typedef struct {
  int np;
  float *pos;
} Catalog_f; //Catalog (single precision)

#endif //_CUTE_DEFINE_
