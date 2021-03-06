#include <cuda.h>
#include "cuerr.hpp"
#include "devstereomatcher.hpp"
#include "cpustereomatcher.hpp"
#include "wtadev.hpp"
#include "ssddev.hpp"
#include "crosscorrelationdev.hpp"
#include "dynamicprogdev.hpp"
#include "dynamicprogcpu.hpp"
#include "semiglobaldev.hpp"
#include "birchfieldcostdev.hpp"
#include "commonstereomatcherfactory.hpp"

TDV_NAMESPACE_BEGIN

CommonStereoMatcherFactory::CommonStereoMatcherFactory()
{
    m_matchMode = SSD;
    m_optMode = WTA;
    m_compMode = CPU;
    m_maxDisparity = 64;
}

StereoMatcher* CommonStereoMatcherFactory::createStereoMatcher()
{
    CUerrExp cerr;
    int cudaDevices = 0;
    try
    {
        cerr << cudaGetDeviceCount(&cudaDevices);
    }
    catch (const tdv::Exception &ex)
    {
        cudaDevices = 0;
    }
    
    if ( cudaDevices > 0 && m_compMode == Device )
    {
        DevStereoMatcher *matcher = new DevStereoMatcher;
        
        switch ( m_matchMode )
        {
        case SSD:
            matcher->setMatchingCost(boost::shared_ptr<SSDDev>(
                                         new SSDDev(m_maxDisparity)));
            break;
        case CrossCorrelationNorm:
            matcher->setMatchingCost(boost::shared_ptr<CrossCorrelationDev>(
                                         new CrossCorrelationDev(m_maxDisparity)));
            break;
        case BirchfieldTomasi:
            matcher->setMatchingCost(boost::shared_ptr<BirchfieldCostDev>(
                                         new BirchfieldCostDev(m_maxDisparity)));
            break;            
        default:
            assert(false);
        }
        
        switch ( m_optMode )
        {
        case WTA:
            matcher->setOptimizer(boost::shared_ptr<WTADev>(
                                      new WTADev));
            break;
        case DynamicProg:
            matcher->setOptimizer(boost::shared_ptr<DynamicProgDev>(
                                      new DynamicProgDev));
            break;
        case DynamicProgOnCPU:
            matcher->setOptimizer(boost::shared_ptr<DynamicProgCPU>(
                                      new DynamicProgCPU));
            break;
        case Global:
        case SemiGlobal:
            matcher->setOptimizer(boost::shared_ptr<SemiGlobalDev>(
                                      new SemiGlobalDev));
            break;
        default:
            assert(false);
        }
        return matcher;
    }
    else
    {        
        StereoCorrespondenceCV::MatchingMode cvMatchMode = 
            StereoCorrespondenceCV::LocalMatching;
        
        switch ( m_optMode )
        {
        case WTA:
        case DynamicProg:
        case DynamicProgOnCPU:
            cvMatchMode = StereoCorrespondenceCV::LocalMatching;
            break;
        case SemiGlobal:
            cvMatchMode = StereoCorrespondenceCV::SemiGlobalMatching;
            break;
        case Global:
            cvMatchMode = StereoCorrespondenceCV::GlobalMatching;
            break;
        default:            
            assert(false);
        }
        
        return new CPUStereoMatcher(cvMatchMode, m_maxDisparity, 4);        
    }
}

TDV_NAMESPACE_END
