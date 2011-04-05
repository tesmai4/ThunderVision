#ifndef TDV_TDVCONTEXT_HPP
#define TDV_TDVCONTEXT_HPP

#include <tdvbasic/common.hpp>
#include <string>
#include <cv.h>
#include "teeworkunit.hpp"
#include "workunitprocess.hpp"
#include "processrunner.hpp"
#include "exceptionreport.hpp"

TDV_NAMESPACE_BEGIN

class StereoInputSource;
class StereoMatcher;
class Reconstruction;
class Calibration;
class ThunderSpec;

class TDVContext: public ExceptionReport
{
public:    
    TDVContext();
    
    void spec(tdv::ThunderSpec *spec);

    void start(StereoInputSource *inputSrc);    
    
    void dispose();
    
    Reconstruction* runReconstruction(const std::string &profileName);    
    
    void releaseReconstruction(Reconstruction *reconst);
    
    Calibration* runCalibration();
    
    void releaseCalibration(Calibration *calib);

    void dupInputSource(ReadPipe<CvMat*> **leftSrc, 
                        ReadPipe<CvMat*> **rightSrc);
    
    void undupInputSource();
    
    void errorOcurred(const std::exception &err);
    
    void errorHandler(ExceptionReport *handler)
    {
        m_errHandler = handler;
    }    
    
private:
    tdv::ProcessRunner *m_inputRunner, 
        *m_reconstRunner,
        *m_calibRunner;

    void specChanged();    
    
    StereoInputSource *m_inputSrc;
    TWorkUnitProcess<TeeWorkUnit<CvMat*, CvMatSinkPol> > m_inputTees[2];
    
    StereoMatcher *m_matcher;
    
    ExceptionReport *m_errHandler;
        
    ThunderSpec *m_spec;
};

TDV_NAMESPACE_END

#endif /* TDV_TDVCONTEXT_HPP */
