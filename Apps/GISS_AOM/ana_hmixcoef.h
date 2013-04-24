      SUBROUTINE ana_hmixcoef (ng, tile, model)
!
!! svn $Id$
!!================================================= Hernan G. Arango ===
!! Copyright (c) 2002-2013 The ROMS/TOMS Group                         !
!!   Licensed under a MIT/X style license                              !
!!   See License_ROMS.txt                                              !
!!======================================================================
!                                                                      !
!  This routine rescales horizontal mixing coefficients according      !
!  to the grid size.  Also,  if applicable,  increases horizontal      !
!  in sponge areas.                                                    !
!                                                                      !
!  WARNING:   All biharmonic coefficients are assumed to have the      !
!             square root taken and have  m^2 s^-1/2 units.  This      !
!             will allow multiplying the  biharmonic  coefficient      !
!             to harmonic operator.                                    !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_grid
      USE mod_mixing
      USE mod_ncparam
      USE mod_scalars
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model
!
!  Local variable declarations.
!
#include "tile.h"

      CALL ana_hmixcoef_tile (ng, model, Istr, Iend, Jstr, Jend,        &
     &                        LBi, UBi, LBj, UBj,                       &
#ifdef SOLVE3D
# ifdef TS_DIF2
     &                        MIXING(ng) % diff2,                       &
# endif
# ifdef TS_DIF4
     &                        MIXING(ng) % diff4,                       &
# endif
#endif
#ifdef UV_VIS2
     &                        MIXING(ng) % visc2_p,                     &
     &                        MIXING(ng) % visc2_r,                     &
#endif
#ifdef UV_VIS4
     &                        MIXING(ng) % visc4_p,                     &
     &                        MIXING(ng) % visc4_r,                     &
#endif
     &                        GRID(ng) % grdscl,                        &
     &                        GRID(ng) % xr,                            &
     &                        GRID(ng) % yr)
!
! Set analytical header file name used.
!
      IF (Lanafile) THEN
        WRITE (ANANAME( 8),'(a,a)') TRIM(Adir), '/ana_hmixcoef.h'
      END IF

      RETURN
      END SUBROUTINE ana_hmixcoef
!
!***********************************************************************
      SUBROUTINE ana_hmixcoef_tile (ng, model, Istr, Iend, Jstr, Jend,  &
     &                              LBi, UBi, LBj, UBj,                 &
#ifdef SOLVE3D
# ifdef TS_DIF2
     &                              diff2,                              &
# endif
# ifdef TS_DIF4
     &                              diff4,                              &
# endif
#endif
#ifdef UV_VIS2
     &                              visc2_p,                            &
     &                              visc2_r,                            &
#endif
#ifdef UV_VIS4
     &                              visc4_p,                            &
     &                              visc4_r,                            &
#endif
     &                              grdscl, xr, yr)
!***********************************************************************
!
      USE mod_param
      USE mod_scalars
!
      USE exchange_2d_mod
#ifdef DISTRIBUTE
      USE mp_exchange_mod, ONLY : mp_exchange2d
# ifdef SOLVE3D
      USE mp_exchange_mod, ONLY : mp_exchange3d
# endif
#endif
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, model, Iend, Istr, Jend, Jstr
      integer, intent(in) :: LBi, UBi, LBj, UBj

#ifdef ASSUMED_SHAPE
      real(r8), intent(in) :: grdscl(LBi:,LBj:)
      real(r8), intent(in) :: xr(LBi:,LBj:)
      real(r8), intent(in) :: yr(LBi:,LBj:)
# ifdef SOLVE3D
#  ifdef TS_DIF2
      real(r8), intent(inout) :: diff2(LBi:,LBj:,:)
#  endif
#  ifdef TS_DIF4
      real(r8), intent(inout) :: diff4(LBi:,LBj:,:)
#  endif
# endif
# ifdef UV_VIS2
      real(r8), intent(inout) :: visc2_p(LBi:,LBj:)
      real(r8), intent(inout) :: visc2_r(LBi:,LBj:)
# endif
# ifdef UV_VIS4
      real(r8), intent(inout) :: visc4_p(LBi:,LBj:)
      real(r8), intent(inout) :: visc4_r(LBi:,LBj:)
# endif
#else
      real(r8), intent(in) :: grdscl(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: xr(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: yr(LBi:UBi,LBj:UBj)
# ifdef SOLVE3D
#  ifdef TS_DIF2
      real(r8), intent(inout) :: diff2(LBi:UBi,LBj:UBj,NT(ng))
#  endif
#  ifdef TS_DIF4
      real(r8), intent(inout) :: diff4(LBi:UBi,LBj:UBj,NT(ng))
#  endif
# endif
# ifdef UV_VIS2
      real(r8), intent(inout) :: visc2_p(LBi:UBi,LBj:UBj)
      real(r8), intent(inout) :: visc2_r(LBi:UBi,LBj:UBj)
# endif
# ifdef UV_VIS4
      real(r8), intent(inout) :: visc4_p(LBi:UBi,LBj:UBj)
      real(r8), intent(inout) :: visc4_r(LBi:UBi,LBj:UBj)
# endif
#endif
!
!  Local variable declarations.
!
      integer :: IstrR, IendR, JstrR, JendR, IstrU, JstrV
      integer :: Iwrk, i, j, itrc
      real(r8) :: cff, cff1, cff2, fac

#include "set_bounds.h"

#ifdef VISC_GRID
!
!-----------------------------------------------------------------------
!  Scale horizontal viscosity according to the grid size.
!-----------------------------------------------------------------------
!
!! WARNING:  This section is generic for all applications. Please do not
!!           change the code below.
!!            
# ifdef UV_VIS2
      cff=visc2(ng)/grdmax(ng)
      DO j=JstrR,JendR
        DO i=IstrR,IendR
          visc2_r(i,j)=cff*grdscl(i,j)
        END DO
      END DO
      cff=0.25_r8*cff
      DO j=Jstr,JendR
        DO i=Istr,IendR
          visc2_p(i,j)=cff*(grdscl(i,j  )+grdscl(i-1,j  )+              &
     &                      grdscl(i,j-1)+grdscl(i-1,j-1))
        END DO
      END DO
# endif
# ifdef UV_VIS4
      cff=visc4(ng)/(grdmax(ng)**3)
      DO j=JstrR,JendR
        DO i=IstrR,IendR
          visc4_r(i,j)=cff*grdscl(i,j)**3
        END DO
      END DO
      cff=0.25_r8*cff
      DO j=Jstr,JendR
        DO i=Istr,IendR
          visc4_p(i,j)=cff*(grdscl(i,j  )**3+grdscl(i-1,j  )**3+        &
     &                      grdscl(i,j-1)**3+grdscl(i-1,j-1)**3)
        END DO
      END DO
# endif
#endif
#ifdef DIFF_GRID
!
!-----------------------------------------------------------------------
!  Scale horizontal diffusion according to the grid size.
!-----------------------------------------------------------------------
!
!! WARNING:  This section is generic for all applications. Please do not
!!           change the code below.
!!            
# ifdef TS_DIF2
      DO itrc=1,NT(ng)
        cff=tnu2(itrc,ng)/grdmax(ng)
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            diff2(i,j,itrc)=cff*grdscl(i,j)
          END DO
        END DO
      END DO
# endif
# ifdef TS_DIF4
      DO itrc=1,NT(ng)
        cff=tnu4(itrc,ng)/(grdmax(ng)**3)
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            diff4(i,j,itrc)=cff*grdscl(i,j)**3
          END DO
        END DO
      END DO
# endif
#endif
#ifdef SPONGE
!
!-----------------------------------------------------------------------
!  Increase horizontal mixing in the sponge areas.
!-----------------------------------------------------------------------
!
!! User modifiable section.  Please specify the appropiate sponge area
!! by increasing its horizontal mixing coefficients.
!!            

# if defined SCB
!
!  Southern California Bight sponge areas:

#  if defined UV_VIS2
!
!  Increase harmonic vicosity linearly (up to a factor of four, fac=4) 
!  from the interior to the open boundary with a sponge area of 6 grid
!  points. Notice that the sponge area is only applied at the southern,
!  northern and eastern edges and the maximum viscosity occurs at the
!  boundary point.
!
      fac=4.0_r8
      DO j=JstrR,MIN(6,JendR)
        cff=visc2(ng)+REAL(6-j,r8)*(fac*visc2(ng)-visc2(ng))/6.0_r8
        DO i=IstrR,IendR
          visc2_r(i,j)=cff
          visc2_p(i,j)=cff
        END DO
      END DO
      DO j=MAX(JstrR,Mm(ng)+1-6),JendR
        cff=fac*visc2(ng)+                                              &
     &      REAL(Mm(ng)+1-j,r8)*(visc2(ng)-fac*visc2(ng))/6.0_r8
        DO i=IstrR,IendR
          visc2_r(i,j)=cff
          visc2_p(i,j)=cff
        END DO
      END DO
      DO i=IstrR,MIN(6,IendR)
        DO j=MAX(JstrR,i),MIN(Mm(ng)+1-i,JendR)
          cff=visc2(ng)+REAL(6-i,r8)*(fac*visc2(ng)-visc2(ng))/6.0_r8
          visc2_r(i,j)=cff
          visc2_p(i,j)=cff
        END DO
      END DO
#  endif
#  if defined TS_DIF2
!
!  Increase harmonic diffusion linearly (up to a factor of four, fac=4) 
!  from the interior to the open boundary with a sponge area of 6 grid
!  points. Notice that the sponge area is only applied at the southern,
!  northern and eastern edges and the maximum diffusion occurs at the
!  boundary point.
!
      fac=4.0_r8
      DO j=JstrR,MIN(6,JendR)
        cff1=tnu2(itemp,ng)+                                            &
     &       REAL(6-j,r8)*(fac*tnu2(itemp,ng)-tnu2(itemp,ng))/6.0_r8
        cff2=tnu2(isalt,ng)+                                            &
     &       REAL(6-j,r8)*(fac*tnu2(isalt,ng)-tnu2(isalt,ng))/6.0_r8
        DO i=IstrR,IendR
          diff2(i,j,itemp)=cff1
          diff2(i,j,isalt)=cff2
        END DO
      END DO
      DO j=MAX(JstrR,Mm(ng)+1-6),JendR
        cff1=fac*tnu2(itemp,ng)+                                        &
     &       REAL(Mm(ng)+1-j,r8)*(tnu2(itemp,ng)-                       &
     &                            fac*tnu2(itemp,ng))/6.0_r8
        cff2=fac*tnu2(isalt,ng)+                                        &
     &       REAL(Mm(ng)+1-j,r8)*(tnu2(isalt,ng)-                       &
     &                            fac*tnu2(isalt,ng))/6.0_r8
        DO i=IstrR,IendR
          diff2(i,j,itemp)=cff1
          diff2(i,j,isalt)=cff2
        END DO
      END DO
      DO i=IstrR,MIN(6,IendR)
        DO j=MAX(JstrR,i),MIN(Mm(ng)+1-i,JendR)
          cff1=tnu2(itemp,ng)+                                          &
     &         REAL(6-i,r8)*(fac*tnu2(itemp,ng)-tnu2(itemp,ng))/6.0_r8
          cff2=tnu2(isalt,ng)+                                          &
     &         REAL(6-i,r8)*(fac*tnu2(isalt,ng)-tnu2(isalt,ng))/6.0_r8
          diff2(i,j,itemp)=cff1
          diff2(i,j,isalt)=cff2
        END DO
      END DO
#  endif
# elif defined CGOA || defined SEBS || defined BERING || defined NEP4
!
! Northeast Pacific sponge areas
!
      Iwrk = 10
#  if defined UV_VIS2       DO i=IstrR,IendR
        DO j=JstrR,MIN(Iwrk,JendR) #   if defined NEP4
          cff = 250.*0.5_r8*(1.0_r8+COS(pi*REAL(j,r8)/REAL(Iwrk,r8)))
#   else
          cff = 100.*0.5_r8*(1.0_r8+COS(pi*REAL(j,r8)/REAL(Iwrk,r8)))
#   endif
          visc2_r(i,j) = max(cff, visc2_r(i,j))
          visc2_p(i,j) = max(cff, visc2_p(i,j))
        END DO
      END DO
      DO i=IstrR,MIN(Iwrk,IendR)
        DO j=MAX(JstrR,i),JendR
#   if defined NEP4
          cff = 250.*0.5_r8*(1.0_r8+COS(pi*REAL(i,r8)/REAL(Iwrk,r8)))
#   else
          cff = 100.*0.5_r8*(1.0_r8+COS(pi*REAL(i,r8)/REAL(Iwrk,r8)))
#   endif
          visc2_r(i,j) = max(cff, visc2_r(i,j))
          visc2_p(i,j) = max(cff, visc2_p(i,j))
        END DO
      END DO
#  endif
#  ifdef SOLVE3D
#   if defined TS_DIF2
      DO itrc=1,NT(ng)
        DO j=JstrR,MIN(Iwrk,JendR)
#    if defined NEP4
          cff = 100. * (1.0_r8+COS(pi*REAL(j,r8)/REAL(Iwrk,r8)))
#    else
          cff = 30. * (1.0_r8+COS(pi*REAL(j,r8)/REAL(Iwrk,r8)))
#    endif
          DO i=IstrR,IendR
            diff2(i,j,itrc)=max(cff, diff2(i,j,itrc))
          END DO
        END DO
        DO i=IstrR,MIN(Iwrk,IendR)
          DO j=MAX(JstrR,i),JendR
#    if defined NEP4
            cff = 100. * (1.0_r8+COS(pi*REAL(i,r8)/REAL(Iwrk,r8)))
#    else
            cff = 30. * (1.0_r8+COS(pi*REAL(i,r8)/REAL(Iwrk,r8)))
#    endif
            diff2(i,j,itrc) = max(cff, diff2(i,j,itrc))
          END DO
        END DO
      END DO
#   endif
#  endif
# elif defined JAVA_1
!
! Sponges on three sides
!
      Iwrk = 10
#  if defined UV_VIS2
      DO i=IstrR,IendR
        DO j=JstrR,MIN(Iwrk,JendR)
          cff =
1000.*0.5_r8*(1.0_r8+COS(pi*REAL(j,r8)/REAL(Iwrk,r8)))
          visc2_r(i,j) = max(cff, visc2_r(i,j))
          visc2_p(i,j) = max(cff, visc2_p(i,j))
        END DO
      END DO
      DO i=IstrR,MIN(Iwrk,IendR)
        DO j=MAX(JstrR,i),JendR
          cff =
1000.*0.5_r8*(1.0_r8+COS(pi*REAL(i,r8)/REAL(Iwrk,r8)))
          visc2_r(i,j) = max(cff, visc2_r(i,j))
          visc2_p(i,j) = max(cff, visc2_p(i,j))
        END DO
      END DO
      DO i=MAX(Lm(ng)+1-Iwrk,IstrR),IendR
        ifoo = Lm(ng)+1-i
        DO j=MAX(JstrR,ifoo),JendR
          cff = 1000.*0.5_r8*                                           &
     &             (1.0_r8+COS(pi*REAL(ifoo,r8)/REAL(Iwrk,r8)))
          visc2_r(i,j) = max(cff, visc2_r(i,j))
          visc2_p(i+1,j) = max(cff, visc2_p(i+1,j))
        END DO
      END DO
#  endif
#  ifdef SOLVE3D
#   if defined TS_DIF2
      DO itrc=1,NT(ng)
        DO j=JstrR,MIN(Iwrk,JendR)
          cff = 500. * (1.0_r8+COS(pi*REAL(j,r8)/REAL(Iwrk,r8)))
          DO i=IstrR,IendR
            diff2(i,j,itrc)=max(cff, diff2(i,j,itrc))
          END DO
        END DO
        DO i=IstrR,MIN(Iwrk,IendR)
          DO j=MAX(JstrR,i),JendR
            cff = 500. * (1.0_r8+COS(pi*REAL(i,r8)/REAL(Iwrk,r8)))
            diff2(i,j,itrc) = max(cff, diff2(i,j,itrc))
          END DO
        END DO
        DO i=MAX(Lm(ng)+1-Iwrk,IstrR),IendR
          ifoo = Lm(ng)+1-i
          DO j=MAX(JstrR,ifoo),JendR
            cff = 500. * (1.0_r8+COS(pi*REAL(ifoo,r8)/REAL(Iwrk,r8)))
            diff2(i,j,itrc) = max(cff, diff2(i,j,itrc))
          END DO
        END DO
      END DO
#   endif
#  endif


# else
!!
!!  Specify your application sponge here.
!!
# endif
#endif
!
!-----------------------------------------------------------------------
!  Exchange boundary data.
!-----------------------------------------------------------------------
!
!! WARNING:  This section is generic for all applications. Please do not
!!           change the code below.
!!            
!
!  Exchange boundary data.
!
      IF (EWperiodic(ng).or.NSperiodic(ng)) THEN
#ifdef UV_VIS2
        CALL exchange_r2d_tile (ng, tile,                               &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        visc2_r)
        CALL exchange_p2d_tile (ng, tile,                               &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        visc2_p)
#endif
#ifdef UV_VIS4
        CALL exchange_r2d_tile (ng, tile,                               &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        visc4_r)
        CALL exchange_p2d_tile (ng, tile,                               &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        visc4_p)
#endif
#ifdef SOLVE3D
# ifdef TS_DIF2
        DO itrc=1,NT(ng)
          CALL exchange_r2d_tile (ng, tile,                             &
     &                          LBi, UBi, LBj, UBj,                     &
     &                          diff2(:,:,itrc))
        END DO
# endif
# ifdef TS_DIF4
        DO itrc=1,NT(ng)
          CALL exchange_r2d_tile (ng, tile,                             &
     &                          LBi, UBi, LBj, UBj,                     &
     &                          diff4(:,:,itrc))
        END DO
# endif
#endif
      END IF
#ifdef DISTRIBUTE
# ifdef UV_VIS2
      CALL mp_exchange2d (ng, tile, model, 2,                           &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    NghostPoints,                                 &
     &                    EWperiodic(ng), NSperiodic(ng),               &
     &                    visc2_r, visc2_p)
# endif
# ifdef UV_VIS4
      CALL mp_exchange2d (ng, tile, model, 2,                           &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    NghostPoints,                                 &
     &                    EWperiodic(ng), NSperiodic(ng),               &
     &                    visc4_r, visc4_p)
# endif
# ifdef SOLVE3D
#  ifdef TS_DIF2
      CALL mp_exchange3d (ng, tile, model, 1,                           &
     &                    LBi, UBi, LBj, UBj, 1, NT(ng),                &
     &                    NghostPoints,                                 &
     &                    EWperiodic(ng), NSperiodic(ng),               &
     &                    diff2)
#  endif
#  ifdef TS_DIF4
      CALL mp_exchange3d (ng, tile, model, 1,                           &
     &                    LBi, UBi, LBj, UBj, 1, NT(ng),                &
     &                    NghostPoints,                                 &
     &                    EWperiodic(ng), NSperiodic(ng),               &
     &                    diff4)
#  endif
# endif
#endif
      RETURN
      END SUBROUTINE ana_hmixcoef_tile