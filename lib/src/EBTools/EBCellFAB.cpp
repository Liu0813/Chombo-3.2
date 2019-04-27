#ifdef CH_LANG_CC
/*
 *      _______              __
 *     / ___/ /  ___  __ _  / /  ___
 *    / /__/ _ \/ _ \/  V \/ _ \/ _ \
 *    \___/_//_/\___/_/_/_/_.__/\___/
 *    Please refer to Copyright.txt, in Chombo's root directory.
 */
#endif

//  ANAG, LBNL

#include "EBCellFAB.H"
#include "EBArithF_F.H"
#include "FArrayBox.H"
#include "BoxIterator.H"
#include "EBArith.H"
#include "EBCellFABF_F.H"
#include <float.h>
#include "NamespaceHeader.H"

bool EBCellFAB::s_verbose = false;

/**********************/
/**********************/
EBCellFAB::EBCellFAB():BaseEBCellFAB<Real>()
{
}
/**********************/
/**********************/
void
EBCellFAB::define(const EBISBox& a_ebisBox,
                  const Box& a_region, int a_nVar)
{
  BaseEBCellFAB<Real>::define(a_ebisBox, a_region, a_nVar);

#ifdef CH_USE_SETVAL
  setVal(BaseFabRealSetVal);
#else
  setVal(0.);
#endif
}
/**********************/
/**********************/
EBCellFAB::EBCellFAB(const EBISBox& a_ebisBox,
                     const Box& a_region, int a_nComp)
  :BaseEBCellFAB<Real>(a_ebisBox, a_region, a_nComp)
{
#ifdef CH_USE_SETVAL
  setVal(BaseFabRealSetVal);
#else
  setVal(0.);
#endif
}

/**********************/
/**********************/
EBCellFAB::~EBCellFAB()
{
}

/**********************/
/**********************/
const FArrayBox&
EBCellFAB::getFArrayBox() const
{
  CH_assert(isDefined());
  return (const FArrayBox&)m_regFAB;
}

/**********************/
/**********************/
FArrayBox&
EBCellFAB::getFArrayBox()
{
  CH_assert(isDefined());
  return (FArrayBox&)m_regFAB;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::negate(void)
{
  CH_assert(isDefined());
  (*this) *= -1.0;
  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator+=(const EBCellFAB& a_src)
{
  CH_assert(a_src.nComp() == nComp());

  plus(a_src, 0, 0, nComp());

  return *this;
}

EBCellFAB&
EBCellFAB::plus(const EBCellFAB& a_src,
                int a_srccomp,
                int a_destcomp,
                int a_numcomp)
{
  Box locRegion = a_src.getRegion() & getRegion();
  plus(a_src, locRegion, a_srccomp, a_destcomp, a_numcomp);
  return *this;
}

EBCellFAB& EBCellFAB::plus(const EBCellFAB& a_src,
                           const Box& a_region,
                           int a_srccomp,
                           int a_destcomp,
                           int a_numcomp)
{
  CH_assert(isDefined());
  CH_assert(a_src.isDefined());
  CH_assert(a_srccomp + a_numcomp <= a_src.nComp());
  CH_assert(a_destcomp + a_numcomp <= nComp());
  const Box& locRegion = a_region;
  bool sameRegBox = (a_src.m_regFAB.box() == m_regFAB.box());

  if (!locRegion.isEmpty())
    {
      FORT_ADDTWOFAB(CHF_FRA(m_regFAB),
                     CHF_CONST_FRA(a_src.m_regFAB),
                     CHF_BOX(locRegion),
                     CHF_INT(a_srccomp),
                     CHF_INT(a_destcomp),
                     CHF_INT(a_numcomp));

      m_irrFAB.forall(a_src.m_irrFAB, locRegion, a_srccomp, a_destcomp, a_numcomp, sameRegBox, [](Real& dest, const Real& src){dest+=src;});
      /*
      if (sameRegBox && (locRegion == getRegion() && locRegion == a_src.getRegion()))
        {
          Real* l = m_irrFAB.dataPtr(a_destcomp);
          const Real* r = a_src.m_irrFAB.dataPtr(a_srccomp);
          int nvof = m_irrFAB.numVoFs();
          CH_assert(nvof == a_src.m_irrFAB.numVoFs());
          for (int i=0; i<a_numcomp*nvof; i++)
            l[i]+=r[i];
        }
      else
        {
          const Vector<VolIndex>& vofs = m_irrFAB.getVoFs();
          for(int i=0; i<vofs.size(); i++)
            {
              const VolIndex& vof=vofs[i];
              if(locRegion.contains(vof.gridIndex()))
                {
                  for (int icomp = 0; icomp < a_numcomp; ++icomp)
                    {
                      m_irrFAB(vof, a_destcomp+icomp) +=
                        a_src.m_irrFAB(vof, a_srccomp+icomp);
                    }
                }
            }
          // IntVectSet ivsMulti = a_src.getMultiCells();
          // ivsMulti &= getMultiCells();
          // ivsMulti &= locRegion;
          // IVSIterator ivsit(ivsMulti);
          // for (ivsit.reset(); ivsit.ok(); ++ivsit)
          //   {
          //     const IntVect& iv = ivsit();
          //     Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
          //     for (int ivof = 0; ivof < vofs.size(); ivof++)
          //       {
          //         const VolIndex& vof = vofs[ivof];
          //         for (int icomp = 0; icomp < a_numcomp; ++icomp)
          //           {
          //             m_irrFAB(vof, a_destcomp+icomp) +=
          //               a_src.m_irrFAB(vof, a_srccomp+icomp);
          //           }
          //       }
          //   }
        }
      */
    }
  return *this;
}

/**********************/
/**********************/

void EBCellFAB::clone(const EBCellFAB& a_arg)
{
  CH_TIME("EBCellFAB::clone");
  define(a_arg.m_ebisBox, a_arg.getRegion(), a_arg.nComp());
  copy(a_arg);
}

EBCellFAB&
EBCellFAB::operator-=(const EBCellFAB& a_src)
{
  CH_assert(a_src.nComp() == nComp());

  minus(a_src, 0, 0, nComp());

  return *this;
}


EBCellFAB&
EBCellFAB::minus(const EBCellFAB& a_src,
                 int a_srccomp,
                 int a_destcomp,
                 int a_numcomp)
{
  CH_TIME("EBCellFAB::minus");
  CH_assert(isDefined());
  CH_assert(a_src.isDefined());
  // Dan G. feels strongly that the assert below should NOT be commented out
  // Brian feels that a weaker version of the CH_assert (if possible) is needed
  // Terry is just trying to get his code to work
  //CH_assert(m_ebisBox == a_src.m_ebisBox);

  CH_assert(a_srccomp + a_numcomp <= a_src.nComp());
  CH_assert(a_destcomp + a_numcomp <= nComp());

  Box locRegion = a_src.getRegion() & getRegion();
  bool sameRegBox = (a_src.m_regFAB.box() == m_regFAB.box());

  if (!locRegion.isEmpty())
    {
      FORT_SUBTRACTTWOFAB(CHF_FRA(m_regFAB),
                          CHF_CONST_FRA(a_src.m_regFAB),
                          CHF_BOX(locRegion),
                          CHF_INT(a_srccomp),
                          CHF_INT(a_destcomp),
                          CHF_INT(a_numcomp));
      m_irrFAB.forall(a_src.m_irrFAB, locRegion, a_srccomp, a_destcomp, a_numcomp, sameRegBox, [](Real& dest, const Real& src){dest-=src;});
      /*
      if (sameRegBox && (locRegion == getRegion() && locRegion == a_src.getRegion()))
        {
          Real* l = m_irrFAB.dataPtr(a_destcomp);
          const Real* r = a_src.m_irrFAB.dataPtr(a_srccomp);
          int nvof = m_irrFAB.numVoFs();
          CH_assert(nvof == a_src.m_irrFAB.numVoFs());
          for (int i=0; i<a_numcomp*nvof; i++)
            l[i]-=r[i];
        }
      else
        {
          IntVectSet ivsMulti = a_src.getMultiCells();
          ivsMulti &= getMultiCells();
          ivsMulti &= locRegion;
          IVSIterator ivsit(ivsMulti);
          for (ivsit.reset(); ivsit.ok(); ++ivsit)
            {
              const IntVect& iv = ivsit();
              Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
              for (int ivof = 0; ivof < vofs.size(); ivof++)
                {
                  const VolIndex& vof = vofs[ivof];
                  for (int icomp = 0; icomp < a_numcomp; ++icomp)
                    {
                      m_irrFAB(vof, a_destcomp+icomp) -=
                        a_src.m_irrFAB(vof, a_srccomp+icomp);
                    }
                }
            }
        }
      */
    }
  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator*=(const EBCellFAB& a_src)
{
  CH_assert(a_src.nComp() == nComp());

  mult(a_src, 0, 0, nComp());

  return *this;
}

EBCellFAB&
EBCellFAB::mult(const EBCellFAB& a_src,
                int a_srccomp,
                int a_destcomp,
                int a_numcomp)
{
  CH_TIME("EBCellFAB::mult");
  CH_assert(isDefined());
  CH_assert(a_src.isDefined());
  // Dan G. feels strongly that the assert below should NOT be commented out
  // Brian feels that a weaker version of the CH_assert (if possible) is needed
  // Terry is just trying to get his code to work
  //CH_assert(m_ebisBox == a_src.m_ebisBox);

  CH_assert(a_srccomp + a_numcomp <= a_src.nComp());
  CH_assert(a_destcomp + a_numcomp <= nComp());
  if (s_verbose)
    {
      IntVect ivdebug(D_DECL(964,736,0));
      VolIndex vof(ivdebug, 0);
      pout() << "EBCellFAB phi(vof) = " << (*this)(vof, 0);
      pout() << ",  a(vof) = " << a_src(vof, 0);
      Real aphi = (*this)(vof, 0) * a_src(vof, 0);
      pout() << ",  aphi(calc) = " <<  aphi << endl;
    }
  Box locRegion = a_src.getRegion() & getRegion();
  bool sameRegBox = (a_src.m_regFAB.box() == m_regFAB.box());

  if (!locRegion.isEmpty())
    {
      FORT_MULTIPLYTWOFAB(CHF_FRA(m_regFAB),
                          CHF_CONST_FRA(a_src.m_regFAB),
                          CHF_BOX(locRegion),
                          CHF_INT(a_srccomp),
                          CHF_INT(a_destcomp),
                          CHF_INT(a_numcomp));
      m_irrFAB.forall(a_src.m_irrFAB, locRegion, a_srccomp, a_destcomp, a_numcomp, sameRegBox, [](Real& dest, const Real& src){dest*=src;});
      /*
      if (sameRegBox && (locRegion == getRegion() && locRegion == a_src.getRegion()))
        {
          Real* l = m_irrFAB.dataPtr(a_destcomp);
          const Real* r = a_src.m_irrFAB.dataPtr(a_srccomp);
          int nvof = m_irrFAB.numVoFs();
          CH_assert(nvof == a_src.m_irrFAB.numVoFs());
          for (int i=0; i<a_numcomp*nvof; i++)
            l[i]*=r[i];
        }
      else
        {
          IntVectSet ivsMulti = a_src.getMultiCells();
          ivsMulti &= getMultiCells();
          ivsMulti &= locRegion;
          IVSIterator ivsit(ivsMulti);
          for (ivsit.reset(); ivsit.ok(); ++ivsit)
            {
              const IntVect& iv = ivsit();
              Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
              for (int ivof = 0; ivof < vofs.size(); ivof++)
                {
                  const VolIndex& vof = vofs[ivof];
                  for (int icomp = 0; icomp < a_numcomp; ++icomp)
                    {
                      m_irrFAB(vof, a_destcomp+icomp) *=
                        a_src.m_irrFAB(vof, a_srccomp+icomp);
                    }
                }
            }
        }
      */
    }

  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator/=(const EBCellFAB& a_src)
{
  CH_assert(a_src.nComp() == nComp());

  divide(a_src, 0, 0, nComp());

  return *this;
}

EBCellFAB&
EBCellFAB::divide(const EBCellFAB& a_src,
                  int a_srccomp,
                  int a_destcomp,
                  int a_numcomp)
{

  CH_TIME("EBCellFAB::divide");
  CH_assert(isDefined());
  CH_assert(a_src.isDefined());
  // Dan G. feels strongly that the assert below should NOT be commented out
  // Brian feels that a weaker version of the CH_assert (if possible) is needed
  // Terry is just trying to get his code to work
  //CH_assert(m_ebisBox == a_src.m_ebisBox);

  CH_assert(a_srccomp + a_numcomp <= a_src.nComp());
  CH_assert(a_destcomp + a_numcomp <= nComp());
  bool sameRegBox = (a_src.m_regFAB.box() == m_regFAB.box());

  Box locRegion = a_src.getRegion() & getRegion();
  if (!locRegion.isEmpty())
    {
      FORT_DIVIDETWOFAB(CHF_FRA(m_regFAB),
                        CHF_CONST_FRA(a_src.m_regFAB),
                        CHF_BOX(locRegion),
                        CHF_INT(a_srccomp),
                        CHF_INT(a_destcomp),
                        CHF_INT(a_numcomp));
      m_irrFAB.forall(a_src.m_irrFAB, locRegion, a_srccomp, a_destcomp, a_numcomp, sameRegBox, [](Real& dest, const Real& src){dest/=src;});
      /*
      if (sameRegBox && (locRegion == getRegion() && locRegion == a_src.getRegion()))
        {
          Real* l = m_irrFAB.dataPtr(a_destcomp);
          const Real* r = a_src.m_irrFAB.dataPtr(a_srccomp);
          int nvof = m_irrFAB.numVoFs();
          CH_assert(nvof == a_src.m_irrFAB.numVoFs());
          for (int i=0; i<a_numcomp*nvof; i++)
            l[i]/=r[i];
        }
      else
        {
          IntVectSet ivsMulti = a_src.getMultiCells();
          ivsMulti &= getMultiCells();
          ivsMulti &= locRegion;
          IVSIterator ivsit(ivsMulti);
          for (ivsit.reset(); ivsit.ok(); ++ivsit)
            {
              const IntVect& iv = ivsit();
              Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
              for (int ivof = 0; ivof < vofs.size(); ivof++)
                {
                  const VolIndex& vof = vofs[ivof];
                  for (int icomp = 0; icomp < a_numcomp; ++icomp)
                    {
                      m_irrFAB(vof, a_destcomp+icomp) /=
                        a_src.m_irrFAB(vof, a_srccomp+icomp);
                    }
                }
            }
        }
      */
    }
  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator+=(const Real& a_src)
{
  CH_TIME("EBCellFAB::operator+=");
  CH_assert(isDefined());
  FORT_ADDFABR(CHF_FRA(m_regFAB),
               CHF_CONST_REAL(a_src),
               CHF_BOX(getRegion()));

  Real* l = m_irrFAB.dataPtr(0);
  int nvof = m_irrFAB.numVoFs();
  for (int i=0; i<nComp()*nvof; i++)
    l[i] += a_src;

  // const IntVectSet& ivsMulti = getMultiCells();
  // IVSIterator ivsit(ivsMulti);
  // for (ivsit.reset(); ivsit.ok(); ++ivsit)
  //   {
  //     const IntVect& iv = ivsit();
  //     Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
  //     for (int ivof = 0; ivof < vofs.size(); ivof++)
  //       {
  //         const VolIndex& vof = vofs[ivof];
  //         for (int icomp = 0; icomp < nComp(); ++icomp)
  //           {
  //             m_irrFAB(vof, icomp) += a_src;
  //           }
  //       }
  //   }
  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator-=(const Real& a_src)
{
  CH_TIME("EBCellFAB::operator-=");
  CH_assert(isDefined());
  FORT_SUBTRACTFABR(CHF_FRA(m_regFAB),
                    CHF_CONST_REAL(a_src),
                    CHF_BOX(getRegion()));

  Real* l = m_irrFAB.dataPtr(0);
  int nvof = m_irrFAB.numVoFs();
  for (int i=0; i<nComp()*nvof; i++)
    l[i] -= a_src;

  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator*=(const Real& a_src)
{
  CH_TIME("EBCellFAB::operator*=");
  CH_assert(isDefined());
  FORT_MULTIPLYFABR(CHF_FRA(m_regFAB),
                    CHF_CONST_REAL(a_src),
                    CHF_BOX(getRegion()));

  Real* l = m_irrFAB.dataPtr(0);
  int nvof = m_irrFAB.numVoFs();
  for (int i=0; i<nComp()*nvof; i++)
    l[i] *= a_src;

  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::mult(Real a_src)
{
  *this *= a_src;

  return *this;
}

EBCellFAB& EBCellFAB::mult(Real a_value, int a_startcomp, int a_numcomp)
{
  CH_TIME("EBCellFAB::multcomp");
  Real* l = m_irrFAB.dataPtr(a_startcomp);
  int nvof = m_irrFAB.numVoFs();
  for (int i=0; i<a_numcomp*nvof; i++)
    l[i] *= a_value;
  
  ((FArrayBox&)m_regFAB).mult(a_value, a_startcomp, a_numcomp);
  return *this;
}

void EBCellFAB::setCoveredCellVal(const Real&    a_val,
                                 const int&  a_comp,
                                 const bool& a_doMulti)
{
  CH_TIME("EBCellFAB::setCoveredCellVal");
  //BaseEBCellFAB<Real>::setCoveredCellVal(a_val, a_comp, a_doMulti);
  
  if(a_doMulti)
    {
      BaseEBCellFAB<Real>::setCoveredCellVal(a_val, a_comp, a_doMulti);
    }
  else
    {
      int all;
      const BaseFab<char>& mask = m_ebisBox.getEBGraph().getMask(all);
      if(all == -1) // all covered
        {
          m_regFAB.setVal(a_val, a_comp);
        }
      else if (all == 0)
        {
          FORT_SETCOVERED(CHF_FRA1(m_regFAB, a_comp),
                          CHF_CONST_REAL(a_val),
                          CHF_CONST_FBA1(mask, 0),
                          CHF_BOX(getRegion()));
        }
    }
  
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::operator/=(const Real& a_src)
{
  CH_TIME("EBCellFAB::operator/=");
  CH_assert(isDefined());
  FORT_DIVIDEFABR(CHF_FRA(m_regFAB),
                  CHF_CONST_REAL(a_src),
                  CHF_BOX(getRegion()));

  Real* l = m_irrFAB.dataPtr(0);
  int nvof = m_irrFAB.numVoFs();
  for (int i=0; i<nComp()*nvof; i++)
    l[i] /= a_src;

  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::plus(const EBCellFAB& a_src, Real a_scale)
{
  CH_TIME("EBCellFAB::plusScale");
  CH_assert(isDefined());
  CH_assert(a_src.isDefined());
  // Dan G. feels strongly that the assert below should NOT be commented out
  // Brian feels that a weaker version of the CH_assert (if possible) is needed
  // Terry is just trying to get his code to work
  //CH_assert(m_ebisBox == a_src.m_ebisBox);
  CH_assert(a_src.nComp() == nComp());

  Box locRegion = a_src.getRegion() & getRegion();
  bool sameRegBox = (a_src.m_regFAB.box() == m_regFAB.box());
  int ncomp=nComp();
  if (!locRegion.isEmpty())
    {
      int srccomp = 0;
      int destcomp = 0;
      FORT_SCALEADDTWOFAB(CHF_FRA(m_regFAB),
                          CHF_CONST_FRA(a_src.m_regFAB),
                          CHF_CONST_REAL(a_scale),
                          CHF_BOX(locRegion),
                          CHF_INT(srccomp),
                          CHF_INT(destcomp),
                          CHF_INT(ncomp));

      m_irrFAB.forall(a_src.m_irrFAB, locRegion, srccomp, destcomp, ncomp, sameRegBox, 
                      [a_scale](Real& dest, const Real& src){dest+=a_scale*src;});

    }
  return *this;
}

/**********************/
/**********************/
EBCellFAB&
EBCellFAB::axby(const EBCellFAB& a_X, const EBCellFAB& a_Y,
                const Real& a_A, const Real& a_B)
{
  CH_TIME("EBCellFAB::axby");
  CH_assert(isDefined());
  CH_assert(a_X.isDefined());
  CH_assert(a_Y.isDefined());
  // Dan G. feels strongly that the assert below should NOT be commented out
  // Brian feels that a weaker version of the CH_assert (if possible) is needed
  // Terry is just trying to get his code to work
  //CH_assert(m_ebisBox == a_X.m_ebisBox);
  CH_assert(a_X.nComp() == nComp());
  CH_assert(a_Y.nComp() == nComp());

  bool sameRegBox = (a_X.m_regFAB.box() == a_Y.m_regFAB.box()) && (a_X.m_regFAB.box() ==  m_regFAB.box());
  Box locRegion = a_X.getRegion() & getRegion() & a_Y.getRegion();
  int ncomp=nComp();
  if (!locRegion.isEmpty())
    {
      int srccomp = 0;
      int destcomp = 0;
      FORT_AXBYFAB(CHF_FRA(m_regFAB),
                   CHF_CONST_FRA(a_X.m_regFAB),
                   CHF_CONST_FRA(a_Y.m_regFAB),
                   CHF_CONST_REAL(a_A),
                   CHF_CONST_REAL(a_B),
                   CHF_BOX(locRegion),
                   CHF_INT(srccomp),
                   CHF_INT(destcomp),
                   CHF_INT(ncomp));
      bool nvofTest   = false;
      bool regionTest = false;
      if(m_irrFAB.numVoFs()>0)
        {
          //CP: we do a thorough test here to see if all VoFs are the same, just being cautious
          const Vector<VolIndex>& vofs   = m_irrFAB.getVoFs();
          const Vector<VolIndex>& vofs_x = a_X.m_irrFAB.getVoFs();
          const Vector<VolIndex>& vofs_y = a_Y.m_irrFAB.getVoFs();
          if (vofs.size() == vofs_x.size() && vofs.size()==vofs_y.size())
            {
              nvofTest = true;
              for (int i=0; i<vofs.size(); i++)
                {
                  if (vofs[i] != vofs_x[i] || vofs[i] != vofs_y[i])
                    {
                      nvofTest = false;
                      break;
                    }
                }
            }
        }

      if (locRegion == a_X.getRegion() && locRegion == a_Y.getRegion() && locRegion == getRegion())
        regionTest = true;
      bool sameRegion = sameRegBox&&regionTest && nvofTest;
      
     m_irrFAB.forall(a_X.m_irrFAB, locRegion, srccomp, destcomp, ncomp, sameRegion, 
                     [a_A](Real& dest, const Real& src){dest=a_A*src;});
     m_irrFAB.forall(a_Y.m_irrFAB, locRegion, srccomp, destcomp, ncomp, sameRegion, 
                     [a_B](Real& dest, const Real& src){dest+=a_B*src;});
     /*
      if (sameRegBox && regionTest && nvofTest)
        {
          const Real* l = a_X.m_irrFAB.dataPtr(destcomp);
          const Real* r = a_Y.m_irrFAB.dataPtr(destcomp);
          Real* c = m_irrFAB.dataPtr(destcomp);
          int nvof = a_X.m_irrFAB.numVoFs();
          for (int i=0; i<nComp()*nvof; i++)
            c[i] = l[i] * a_A + r[i] * a_B;
        }
      else
        {

          IntVectSet ivsMulti = a_X.getMultiCells();
          ivsMulti &= a_Y.getMultiCells();
          ivsMulti &= getMultiCells();  // opt might be to take this out
          ivsMulti &= locRegion;
          IVSIterator ivsit(ivsMulti);
          for (ivsit.reset(); ivsit.ok(); ++ivsit)
            {
              const IntVect& iv = ivsit();
              Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
              for (int ivof = 0; ivof < vofs.size(); ivof++)
                {
                  const VolIndex& vof = vofs[ivof];
                  for (int icomp = 0; icomp < nComp(); ++icomp)
                    {
                      m_irrFAB(vof, icomp) = a_X.m_irrFAB(vof, icomp) * a_A
                        + a_Y.m_irrFAB(vof, icomp) * a_B;
                    }
                }
            }
        }
     */
    }
  return *this;
}
/**********************/
/**********************/
/*
EBCellFAB&
EBCellFAB::axby(const EBCellFAB& a_X, const EBCellFAB& a_Y,
                const Real& a_A, const Real& a_B,
                const int& a_destComp,const int& a_xComp,const int& a_yComp)
{
  CH_assert(isDefined());
  CH_assert(a_X.isDefined());
  CH_assert(a_Y.isDefined());
  // Dan G. feels strongly that the assert below should NOT be commented out
  // Brian feels that a weaker version of the CH_assert (if possible) is needed
  // Terry is just trying to get his code to work
  //CH_assert(m_ebisBox == a_X.m_ebisBox);
  CH_assert(nComp()     > a_destComp);
  CH_assert(a_X.nComp() > a_xComp);
  CH_assert(a_Y.nComp() > a_yComp);

  Box locRegion = a_X.getRegion() & getRegion() & a_Y.getRegion();
  bool sameRegBox = (a_X.m_regFAB.box() == a_Y.m_regFAB.box()) && (a_X.m_regFAB.box() ==  m_regFAB.box());

  if (!locRegion.isEmpty())
    {
      FORT_AXBYFABCOMP(CHF_FRA(m_regFAB),
                       CHF_CONST_FRA(a_X.m_regFAB),
                       CHF_CONST_FRA(a_Y.m_regFAB),
                       CHF_CONST_REAL(a_A),
                       CHF_CONST_REAL(a_B),
                       CHF_CONST_INT(a_destComp),
                       CHF_CONST_INT(a_xComp),
                       CHF_CONST_INT(a_yComp),
                       CHF_BOX(locRegion));

      bool nvofTest   = false;
      bool regionTest = false;
      //CP: we do a thorough test here to see if all VoFs are the same, just being cautious
      const Vector<VolIndex>& vofs   = m_irrFAB.getVoFs();
      const Vector<VolIndex>& vofs_x = a_X.m_irrFAB.getVoFs();
      const Vector<VolIndex>& vofs_y = a_Y.m_irrFAB.getVoFs();
      if (vofs.size() == vofs_x.size() && vofs.size()==vofs_y.size())
        {
          nvofTest = true;
          for (int i=0; i<vofs.size(); i++)
            {
              if (vofs[i] != vofs_x[i] || vofs[i] != vofs_y[i])
                {
                  nvofTest = false;
                  break;
                }
            }

        }
      if (locRegion == a_X.getRegion() && locRegion == a_Y.getRegion() && locRegion == getRegion())
        regionTest = true;

      bool sameRegion = sameRegBox&&regionTest && nvofTest;
      
     m_irrFAB.forall(a_X.m_irrFAB, locRegion, a_srccomp, a_destcomp, ncomp, sameRegion, 
                     [a_A](Real& dest, const Real& src){dest=a_A*src;});
     m_irrFAB.forall(a_Y.m_irrFAB, locRegion, a_srccomp, a_destcomp, ncomp, sameRegion, 
                     [a_B](Real& dest, const Real& src){dest+=a_B*src;});
     
      if (sameRegBox && regionTest && nvofTest)
        {
          const Real* l = a_X.m_irrFAB.dataPtr(a_xComp);
          const Real* r = a_Y.m_irrFAB.dataPtr(a_yComp);
          Real* c = m_irrFAB.dataPtr(a_destComp);
          int nvof = a_X.m_irrFAB.numVoFs();
          int numComp = 1;
          for (int i=0; i< numComp*nvof; i++)
            c[i] = l[i] * a_A + r[i] * a_B;
        }
      else
        {
          IntVectSet ivsMulti = a_X.getMultiCells();
          ivsMulti &= a_Y.getMultiCells();
          ivsMulti &= getMultiCells();  // opt might be to take this out
          ivsMulti &= locRegion;
          IVSIterator ivsit(ivsMulti);
          for (ivsit.reset(); ivsit.ok(); ++ivsit)
            {
              const IntVect& iv = ivsit();
              Vector<VolIndex> vofs = m_ebisBox.getVoFs(iv);
              for (int ivof = 0; ivof < vofs.size(); ivof++)
                {
                  const VolIndex& vof = vofs[ivof];
                  if (locRegion.contains(vof.gridIndex()))
                    {
                      m_irrFAB(vof, a_destComp) = a_X.m_irrFAB(vof, a_xComp) * a_A
                        + a_Y.m_irrFAB(vof, a_yComp) * a_B;
                    }
                }
            }
        }
    }
  return *this;
}
*/

//-----------------------------------------------------------------------
Real
EBCellFAB::max(int a_comp) const
{
  CH_TIME("EBCellFAB::max");
  CH_assert(isDefined());
  Real val = -DBL_MAX;

  // Find the max on irregular cells.
  const EBISBox& ebbox = getEBISBox();
  const Box& box = BaseEBCellFAB<Real>::box();
  const IntVectSet validCells(box);
  for (VoFIterator vit(validCells, ebbox.getEBGraph()); vit.ok(); ++vit)
  {
    VolIndex vofi = vit();
    val = Max(val, (*this)(vofi, a_comp));
  }
  return val;
}
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
Real
EBCellFAB::min(int a_comp) const
{
  CH_TIME("EBCellFAB::min");
  CH_assert(isDefined());
  Real val = DBL_MAX;

  // Find the min on irregular cells.
  const EBISBox& ebbox = getEBISBox();
  const Box& box = BaseEBCellFAB<Real>::box();
  const IntVectSet validCells(box);
  for (VoFIterator vit(validCells, ebbox.getEBGraph()); vit.ok(); ++vit)
  {
    VolIndex vofi = vit();
    val = Min(val, (*this)(vofi, a_comp));
  }
  return val;
}
//-----------------------------------------------------------------------

// Returns the Lp-norm of this EBCellFAB
Real
EBCellFAB::norm(int a_power,
                int a_comp,
                int a_numComp) const
{

  return norm(getRegion() , a_power ,a_comp,a_numComp);

}

// Returns the Lp-norm of this EBCellFAB within a region
Real
EBCellFAB::norm(const Box& a_subbox,
                int        a_power,
                int        a_comp,
                int        a_numComp) const
{
  CH_TIME("EBCellFAB::norm");
  CH_assert(a_numComp == 1);
  return EBArith::norm(*this, a_subbox, m_ebisBox, a_comp, a_power);
}

// (Not implemented) Returns a sum of powers of a subset of this EBCellFAB
Real
EBCellFAB::sumPow(const Box& a_subbox,
                  int        a_power,
                  int        a_comp,
                  int        a_numComp) const
{
  MayDay::Error("sumPow method 2 not implemented");

  return -1.0;
}

// (Not implemented) Return the dot product of this EBCellFAB with another
Real
EBCellFAB::dotProduct(const EBCellFAB& ebfab2) const
{
  MayDay::Error("dotProduct method 2 not implemented");

  return -1.0;
}

void writeVectorLevelName(const Vector<LevelData<EBCellFAB>*>*, Vector<int>* ref, const char*)
{
  MayDay::Warning(" writeVectorLevelName(const Vector<LevelData<EBCellFAB>*>*, const char*) not implemented");
}

#include "NamespaceFooter.H"
