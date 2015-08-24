      SUBROUTINE SAV_MODEL_SUB(ng, Istr, Iend,                            &
     &                        LBi, UBi, LBj, UBj, IminS, ImaxS, pmonth,   & 
     &                        wtemp, PARz, DINwcr_loc, DINsed_loc,        &
     &                        agb_loc, bgb_loc)
!
!***********************************************************************
!****************************************** John C. Warner *************
!****************************************** Neil K. Ganju **************
!****************************************** Jeremy Testa ***************
!****************************************** Tarandeep S. Kalra *********
!                                                                      !
!  This routine computes equilibrium partial pressure of CO2 (pCO2)    !
!  in the surface seawater.                                            !
!                                                                      !
!  On Input:                                                           !
!                                                                      !
!     Istr       Starting tile index in the I-direction.               !
!     Iend       Ending   tile index in the I-direction.               !
!     LBi        I-dimension lower bound.                              !
!     UBi        I-dimension upper bound.                              !
!     LBj        J-dimension lower bound.                              !
!     UBj        J-dimension upper bound.                              !
!     IminS      I-dimension lower bound for private arrays.           !
!     ImaxS      I-dimension upper bound for private arrays.           !
!     t          Water temperature (Celsius).                          !
!     PArout     PAR at depth (mu E m-2 s-1).                          !
!     DINwcr_loc Dissolved Inorganic N in water col. (mu M)            !
!     DINsed_loc Dissolved Inorganic N in sediment col. (mu M)         !
!     agb_loc    Vector of above ground biomass  (mmol N m-2)          !
!     bgb_loc    Vector of below ground biomass  (mmol N m-2)          !
!                                                                      !
!  Estuarine SAV Model                                                 !                                                             
!  Jeremy Testa, May 2015, Chesapeake Biological Laboratory            !                                                           
!                                                                      !
!  Reference:                                                          !
!                                                                      !
!      Madden, C. J., Kemp, W. M. June 1996: Ecosystem Model of an     !
!      Estuarine Submersed Plant Community: Calibration and Simulation !
!      of Eutrophication Responses: Estuarine Research Foundation      !
!      Vol. 19, No. 2B, p. 457-474                                     !
!                                                                      !
!***********************************************************************
!
      USE mod_kinds
      USE mod_biology
!
      implicit none
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, LBi, UBi, LBj, UBj, IminS, ImaxS
      integer, intent(in) :: Istr, Iend
      real(r8), intent(in) :: pmonth
#  ifdef ASSUMED_SHAPE
      real(r8), intent(in) :: wtemp(IminS:)
      real(r8), intent(in) :: PARz(IminS:)
      real(r8), intent(inout) :: DINwcr_loc(IminS:)
      real(r8), intent(inout) :: DINsed_loc(IminS:)
      real(r8), intent(inout) :: Agb_loc(IminS:)
      real(r8), intent(inout) :: Bgb_loc(IminS:)
#  else
      real(r8), intent(in) :: wtemp(IminS:ImaxS)
      real(r8), intent(in) :: PARz(IminS:ImaxS)
      real(r8), intent(in) :: DINwcr_loc(IminS:ImaxS)
      real(r8), intent(in) :: DINsed_loc(IminS:ImaxS)
      real(r8), intent(inout) :: Agb_loc(IminS:ImaxS)
      real(r8), intent(inout) :: Bgb_loc(IminS:ImaxS)
#  endif

!
!
!  Local variable declarations.
!
      integer :: i, thresh, thresh2 
      integer, parameter  :: one=1
      real(r8) :: ua, day_year
      real(r8) :: llim, knt, nlim
      real(r8) :: lmba, pp, agm, agar 
      real(r8) :: agbr, sears, agbg, bgag
      real(r8) :: bgr, bgm, ppbm 
      real(r8) :: shtdens, shtLen
      real(r8) :: cff,cff1
      real(r8) :: temp
      real(r8) :: dtdays 
      real(r8), parameter :: eps=1.0e-10_r8
      real(r8), parameter :: gr2mmol=1000.0_r8/14.007_r8 
      real(r8), parameter :: C2N_ratio=30.0_r8 ! Move this to the input file 
!
!     Initialize local variables and arrays 
!
      lmba  = 1.0_r8 
        
      DO i=Istr, Iend
        agb_loc(i)=5.0_r8
        bgb_loc(i)=30.0_r8
        DINsed_loc(i)=0.0_r8
      END DO
!
!-----------------------------------------------------------------------
!     pmonth contains the month information calculated in spect_fennel.h
!-----------------------------------------------------------------------
!
      day_year = (pmonth - 52.0_r8)*365.0_r8
      temp=gr2mmol/C2N_ratio
      dtdays=dt(ng)*sec2day 
!
      knt=knwc(ng)/knsed(ng)
      DO i=Istr, Iend
        llim=PARz(i)/(PARz(i)+ki(ng))
        nlim=(DINwcr_loc(i)+(knt*DINsed_loc(i)))/                        &
     &       (knwc(ng)+(DINwcr_loc(i)+(knt*DINsed_loc(i))))
!
!-----------------------------------------------------------------------
!    SAV Growth Rate
!-----------------------------------------------------------------------
!
        IF ( GMODopt(ng).eq.1 ) THEN 
          ua=lmba*scl(ng)*(EXP(thta(ng)*(wtemp(i)-Topt(ng)))-1.0_r8)
        ELSE
          ua=scl2(ng)*thta2(ng)**(wtemp(i)-Topt(ng))    
        ENDIF    
    
        IF ( wtemp(i).gt.Tcrit(ng) ) THEN 
          thresh=1 
        ELSE
          thresh=0
        ENDIF

        IF ( day_year.gt.273.0_r8 ) THEN 
          thresh2=1 
        ELSE
          thresh2=0
        ENDIF

!
!-----------------------------------------------------------------------
!  Self-shading effect on growth and primary production rate 
!-----------------------------------------------------------------------
! 
        lmba=1.0_r8-(agb_loc(i)/lmbamx(ng))**2
        pp=agb_loc(i)*(ua*MIN(llim,nlim)) 
!
!-----------------------------------------------------------------------
!  Above ground mortality and above ground active respiration
!-----------------------------------------------------------------------
!
        agm=(kmag(ng)*agb_loc(i))**2
        agar=pp*arsc(i)*EXP(arc(ng)*wtemp(i))
!
!-----------------------------------------------------------------------
!  Above ground basal respiration and seasonal root storage of carbon
!-----------------------------------------------------------------------
!
        agbr=agb_loc(i)*(bsrc(ng)*EXP(rc(ng)*wtemp(i)))
        sears=agb_loc(i)*RtSttl(ng)*thresh2
!
!-----------------------------------------------------------------------
!  Updating Nitrogen in water column with N uptake by plant and N 
!  released from SAV respiration and mortality 
!  (temp--> converts gram Carbon units to mmol Nitrogen units)
!-----------------------------------------------------------------------
!
        DINwcr_loc(i)=DINwcr_loc(i)+(agm+agar+agbr-pp)*temp*dtdays 
!
!-----------------------------------------------------------------------
!  Translocation of above ground biomass to below ground
!-----------------------------------------------------------------------
!
        agbg=pp*DOWNt(ng)
!
!-----------------------------------------------------------------------
!  Translocation of below ground biomass to above ground
!-----------------------------------------------------------------------
!
        bgag=bgb_loc(i)*trns(ng)*thresh
!
!-----------------------------------------------------------------------
!  Below ground biomass respiration and Below ground biomass mortality
!-----------------------------------------------------------------------
!
        bgr=bgb_loc(i)*bsrc(ng)*EXP(rc(ng)*wtemp(i))
        bgm=bgb_loc(i)*(0.01_r8*EXP(km(ng)*wtemp(i)))
!
        DINsed_loc(i)=DINsed_loc(i)+(bgr+bgm)*temp*dtdays 
!
!-----------------------------------------------------------------------
!  Compute new AGB biomass (g C m-2)
!-----------------------------------------------------------------------
!
         cff=pp
!
!-----------------------------------------------------------------------
!  If pp>(available nutrients), no growth    
!  assumes no new growth if growth exceeds available nutrients      
!-----------------------------------------------------------------------
!  
        IF(cff.gt.DINwcr_loc(i))THEN 
          cff1=pp+bgag-agm-agar-agbr-sears-agbg
        ELSE
          cff1=bgag-agm-agar-agbr-sears-agbg
        ENDIF 
          agb_loc(i)=agb_loc(i)+cff1*dtdays 
!
!-----------------------------------------------------------------------
!  Compute new BGB Biomass  (g C m-2)
!-----------------------------------------------------------------------
!
        bgb_loc(i)=bgb_loc(i)+(sears+agbg-bgag-bgm+bgr)*dtdays 
!
!-----------------------------------------------------------------------
!  Compute Primary Production per unit biomass
!-----------------------------------------------------------------------
!
        ppbm=(pp/(agb_loc(i)+eps))
!
!-----------------------------------------------------------------------
!  Observed stems per square meter (average of 6 cores)
!-----------------------------------------------------------------------
!
        shtdens=1100.0_r8
!
!-----------------------------------------------------------------------
!  Empirical computation of shoot length from above ground biomass (cm)
!-----------------------------------------------------------------------
!
        shtLen=2.27_r8*agb_loc(i)
!
     END DO
 
!
!-----------------------------------------------------------------------
!  (temp--> converts gram Carbon units to mmol Nitrogen units)
!-----------------------------------------------------------------------
!
     DO i=Istr, Iend
       agb_loc(i)=agb_loc(i)*temp
       bgb_loc(i)=bgb_loc(i)*temp 
     END DO 

     RETURN
     END SUBROUTINE SAV_MODEL_SUB
