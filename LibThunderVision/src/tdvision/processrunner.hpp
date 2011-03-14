#ifndef TDV_PROCESSRUNNER_HPP
#define TDV_PROCESSRUNNER_HPP

#include <tdvbasic/common.hpp>
#include <tdvbasic/exception.hpp>
#include <boost/thread.hpp>

TDV_NAMESPACE_BEGIN

class Process;
class ExceptionReport;

class ProcessRunner
{
public:
    ProcessRunner(Process **wus, ExceptionReport *report);
    
    void run();

    void join()
    {
        m_threads.join_all();
    }
    
    bool errorOcurred() const
    {
        m_errorOc;
    }
private:
    friend struct ProcessCaller;
    
    void reportError(const std::exception &ex);
        
    ProcessRunner(const ProcessRunner &cpy);
    
    ProcessRunner& operator=(const ProcessRunner &cpy);
    
    boost::thread_group m_threads;
    ExceptionReport *m_errReport;
    std::vector<Process*> m_workUnits;
    
    bool m_errorOc;
};

TDV_NAMESPACE_END

#endif /* TDV_PROCESSRUNNER_HPP */
