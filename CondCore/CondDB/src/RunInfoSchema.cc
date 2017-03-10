#include "CondCore/CondDB/interface/Exception.h"
#include "RunInfoSchema.h"
//
#include <openssl/sha.h>

namespace cond {

  namespace persistency {

    RUN_INFO::Table::Table( coral::ISchema& schema ):
      m_schema( schema ){
    }

    bool RUN_INFO::Table::Table::exists(){
      return existsTable( m_schema, tname );
    }
   
    void RUN_INFO::Table::create(){
      if( exists() ){
	throwException( "RUN_INFO table already exists in this schema.",
			"RUN_INFO::Table::create");
      }
      TableDescription< RUN_NUMBER, START_TIME, END_TIME > descr( tname );
      descr.setPrimaryKey<RUN_NUMBER>();
      createTable( m_schema, descr.get() );
    }

    cond::Time_t RUN_INFO::Table::getLastInserted(){
      cond::Time_t run = cond::time::MIN_VAL;
      Query< MAX_RUN_NUMBER > q0(m_schema);
      for( auto r: q0 ) {
	run = std::get<0>(r);
      }
      return run;
    }

    bool RUN_INFO::Table::getInclusiveRunRange( cond::Time_t lower, cond::Time_t upper,
						std::vector<std::tuple<cond::Time_t,boost::posix_time::ptime,boost::posix_time::ptime> >& runData ){
      // first find the lowest existing run >= upper
      Query< MIN_RUN_NUMBER > q0(m_schema);
      q0.addCondition< RUN_NUMBER >( upper,">=" );
      for( auto r: q0 ) upper = std::get<0>(r);
      // then find the inclusive range
      Query< RUN_NUMBER, START_TIME, END_TIME > q1(m_schema);
      q1.addCondition< RUN_NUMBER >( lower, ">=").addCondition< RUN_NUMBER >( upper, "<=");
      size_t prevSize = runData.size();
      for( auto r: q1 ){
	runData.push_back( r );
      }
      return runData.size()>prevSize;    
    }
    
    bool RUN_INFO::Table::getInclusiveTimeRange( const boost::posix_time::ptime& lower ,const boost::posix_time::ptime& upper, 
						 std::vector<std::tuple<cond::Time_t,boost::posix_time::ptime,boost::posix_time::ptime> >& runData ){
      boost::posix_time::ptime up = upper;
      // first find the lowest existing run >= upper
      Query< MIN_START_TIME > q0(m_schema);
      q0.addCondition< START_TIME >( upper,">=" );
      for( auto r: q0 ) up = std::get<0>(r);
      // then find the inclusive range
      Query< RUN_NUMBER, START_TIME, END_TIME> q1(m_schema);
      q1.addCondition< END_TIME >( lower, ">=").addCondition< START_TIME >( up, "<=");
      size_t prevSize = runData.size();
      for( auto r: q1 ){
	runData.push_back( r );
      }
      return runData.size()>prevSize;    

    }
    
    void RUN_INFO::Table::insert( const std::vector<std::tuple<cond::Time_t,boost::posix_time::ptime,boost::posix_time::ptime> >& runs ){
      if( runs.size()>1 ){
        BulkInserter< RUN_NUMBER, START_TIME, END_TIME > inserter( m_schema, tname );
        for( auto run : runs ) inserter.insert( run );
        inserter.flush();
      } else {
        if(runs.size()){
          const auto& inputData = runs.front(); 
          RowBuffer< RUN_NUMBER, START_TIME, END_TIME > dataToInsert( inputData );
          insertInTable( m_schema, tname, dataToInsert.get() );
        }
      } 
    }
    
    //void RUN_INFO::Table::insertNew( cond::Time_t runNumber, const boost::posix_time::ptime& start ){
    //  RowBuffer< RUN_NUMBER, START_TIME, END_TIME > dataToInsert( std::tie(runNumber, start, start ) );
    //  insertInTable( m_schema, tname, dataToInsert.get() );
    //}

    void RUN_INFO::Table::updateEnd( cond::Time_t runNumber, const boost::posix_time::ptime& end ){
      UpdateBuffer buffer;
      buffer.setColumnData< END_TIME >( std::tie( end ) );
      buffer.addWhereCondition<RUN_NUMBER>( runNumber );
      updateTable( m_schema, tname, buffer );
 
    }

    RunInfoSchema::RunInfoSchema( coral::ISchema& schema ):
      m_runInfoTable( schema ){
    }
      
    bool RunInfoSchema::exists(){
      if( !m_runInfoTable.exists() ) return false;
      return true;
    }
    
    bool RunInfoSchema::create(){
      bool created = false;
      if( !exists() ){
	m_runInfoTable.create();
	created = true;
      }
      return created;
    }

    IRunInfoTable& RunInfoSchema::runInfoTable(){
      return m_runInfoTable;
    }
      
  }
}

