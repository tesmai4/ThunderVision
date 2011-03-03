#ifndef TDV_WORKUNIT_HPP
#define TDV_WORKUNIT_HPP

#include <tdvbasic/common.hpp>
#include <string>

TDV_NAMESPACE_BEGIN

class WorkUnit
{
public:
    WorkUnit()
        : m_workName("Unknow")
    {
    }
    
    virtual ~WorkUnit()
    { }
        
    virtual void process() = 0;
        
    const std::string& workName() const
    {
        return m_workName;
    }
    
    void workName(const std::string &name)
    {
        m_workName = name;
    }        
    
    virtual void finish()
    { }
    
private:    
    std::string m_workName;
};

TDV_NAMESPACE_END

#endif /* TDV_WORKUNIT_HPP */
